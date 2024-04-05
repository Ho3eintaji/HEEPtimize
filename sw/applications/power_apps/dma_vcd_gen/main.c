// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "dma.h"
#include "fast_intr_ctrl.h"
#include "fast_intr_ctrl_regs.h"
#include "gpio.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "heepocrates.h"

#define VCD_TRIGGER_GPIO 0
#define TEST_DATA_SIZE 128

static gpio_t gpio;

uint32_t test_data_4B[TEST_DATA_SIZE] __attribute__ ((aligned (4))) = {0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98, 0x76543210, 0xfedcba98, 0x579a6f90, 0x657d5bee, 0x758ee41f, 0x01234567, 0xfedbca98, 0x89abcdef, 0x679852fe, 0xff8252bb, 0x763b4521, 0x6875adaa, 0x09ac65bb, 0x666ba334, 0x55446677, 0x65ffba98};
uint32_t copied_data_4B[TEST_DATA_SIZE] __attribute__ ((aligned (4))) = { 0 };

int8_t dma_intr_flag;

void dump_on(void);
void dump_off(void);

void handler_irq_fast_dma(void)
{
    fast_intr_ctrl_t fast_intr_ctrl;
    fast_intr_ctrl.base_addr = mmio_region_from_addr((uintptr_t)FAST_INTR_CTRL_START_ADDRESS);
    clear_fast_interrupt(&fast_intr_ctrl, kDma_fic_e);

    dma_intr_flag = 1;
}

int main(int argc, char *argv[])
{
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = 1 << 19;
    CSR_SET_BITS(CSR_REG_MIE, mask);

#if CLK_FREQ != 100000000
    uint32_t fll_freq, fll_freq_real;

    fll_t fll;
    fll.base_addr = mmio_region_from_addr((uintptr_t)FLL_START_ADDRESS);

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);

    fll_freq = fll_set_freq(&fll, CLK_FREQ);
    fll_freq_real = fll_get_freq(&fll);
    soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);
#endif

    dma_t dma;
    dma.base_addr = mmio_region_from_addr((uintptr_t)DMA_START_ADDRESS);

    dma_set_read_ptr(&dma, (uint32_t) test_data_4B);
    dma_set_write_ptr(&dma, (uint32_t) copied_data_4B);
    dma_set_read_ptr_inc(&dma, (uint32_t) 4);
    dma_set_write_ptr_inc(&dma, (uint32_t) 4);
    dma_set_spi_mode(&dma, (uint32_t) 0);
    dma_set_data_type(&dma, (uint32_t) 0);

    dma_intr_flag = 0;

    dump_on();
    dma_set_cnt_start(&dma, (uint32_t) TEST_DATA_SIZE*sizeof(*copied_data_4B));
    while(dma_intr_flag==0) {
        wait_for_interrupt();
    }
    dump_off();

    int32_t errors;

    errors=0;
    for(int i=0; i<TEST_DATA_SIZE; i++) {
        if (copied_data_4B[i] != test_data_4B[i]) {
            printf("ERROR COPY [%d]: %08x != %08x : %04x != %04x.\n", i, &copied_data_4B[i], &test_data_4B[i], copied_data_4B[i], test_data_4B[i]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("Success.\n");
        return EXIT_SUCCESS;
    } else {
        printf("Failure: %d errors out of %d words checked.\n", errors, TEST_DATA_SIZE);
        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;
}

void dump_on(void)
{
    gpio_params_t gpio_params;
    gpio_params.base_addr = mmio_region_from_addr((uintptr_t)GPIO_AO_START_ADDRESS);
    gpio_init(gpio_params, &gpio);
    gpio_output_set_enabled(&gpio, VCD_TRIGGER_GPIO, true);

    gpio_write(&gpio, VCD_TRIGGER_GPIO, true);
}

void dump_off(void)
{
    gpio_write(&gpio, VCD_TRIGGER_GPIO, false);
}
