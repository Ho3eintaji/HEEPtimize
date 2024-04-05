// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "spi_host.h"
#include "fast_intr_ctrl.h"
#include "fast_intr_ctrl_regs.h"
#include "gpio.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "heepocrates.h"

#define REVERT_24b_ADDR(addr) ((((uint32_t)(addr) & 0xff0000) >> 16) | ((uint32_t)(addr) & 0xff00) | (((uint32_t)(addr) & 0xff) << 16))
#define FLASH_CLK_MAX_HZ (133*1000*1000)

#define VCD_TRIGGER_GPIO 0

static gpio_t gpio;

int8_t spi_intr_flag;
spi_host_t spi_host;
uint32_t flash_data[8];
uint32_t flash_original[8] = {1};

void dump_on(void);
void dump_off(void);

void handler_irq_fast_spi(void)
{
    spi_enable_evt_intr(&spi_host, false);
    spi_enable_rxwm_intr(&spi_host, false);

    fast_intr_ctrl_t fast_intr_ctrl;
    fast_intr_ctrl.base_addr = mmio_region_from_addr((uintptr_t)FAST_INTR_CTRL_START_ADDRESS);
    clear_fast_interrupt(&fast_intr_ctrl, kSpi_fic_e);

    spi_intr_flag = 1;
}

int main(int argc, char *argv[])
{
    spi_host.base_addr = mmio_region_from_addr((uintptr_t)SPI_START_ADDRESS);

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    uint32_t core_clk = soc_ctrl_get_frequency(&soc_ctrl);

    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = 1 << 20;
    CSR_SET_BITS(CSR_REG_MIE, mask);

#if CLK_FREQ != 100000000
    uint32_t fll_freq, fll_freq_real;

    fll_t fll;
    fll.base_addr = mmio_region_from_addr((uintptr_t)FLL_START_ADDRESS);

    fll_freq = fll_set_freq(&fll, CLK_FREQ);
    fll_freq_real = fll_get_freq(&fll);
    soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);
#endif

    spi_set_enable(&spi_host, true);
    spi_enable_evt_intr(&spi_host, true);
    spi_enable_rxwm_intr(&spi_host, true);
    spi_output_enable(&spi_host, true);

    uint16_t clk_div = 0;
    if(FLASH_CLK_MAX_HZ < core_clk/2){
        clk_div = (core_clk/(FLASH_CLK_MAX_HZ) - 2)/2;
        if (core_clk/(2 + 2 * clk_div) > FLASH_CLK_MAX_HZ) clk_div += 1;
    }

    const uint32_t chip_cfg = spi_create_configopts((spi_configopts_t){
        .clkdiv     = clk_div,
        .csnidle    = 0xF,
        .csntrail   = 0xF,
        .csnlead    = 0xF,
        .fullcyc    = false,
        .cpha       = 0,
        .cpol       = 0
    });
    spi_set_configopts(&spi_host, 0, chip_cfg);
    spi_set_csid(&spi_host, 0);

    spi_set_rx_watermark(&spi_host, 8);

    uint32_t *flash_data_ptr = flash_data[0];

    const uint32_t powerup_byte_cmd = 0xab;
    spi_write_word(&spi_host, powerup_byte_cmd);

    const uint32_t cmd_powerup = spi_create_command((spi_command_t){
        .len        = 3,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });
    spi_set_command(&spi_host, cmd_powerup);
    spi_wait_for_ready(&spi_host);

    volatile uint32_t data_addr = flash_original;

    const uint32_t read_byte_cmd = ((REVERT_24b_ADDR(flash_original) << 8) | 0x03);

    spi_write_word(&spi_host, read_byte_cmd);
    spi_wait_for_ready(&spi_host);

    const uint32_t cmd_read = spi_create_command((spi_command_t){
        .len        = 3,
        .csaat      = true,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });
    spi_set_command(&spi_host, cmd_read);
    spi_wait_for_ready(&spi_host);

    const uint32_t cmd_read_rx = spi_create_command((spi_command_t){
        .len        = 31,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirRxOnly
    });
    spi_set_command(&spi_host, cmd_read_rx);

    spi_intr_flag = 0;

    dump_on();
    spi_wait_for_ready(&spi_host);
    while(spi_intr_flag==0) {
        wait_for_interrupt();
    }
    dump_off();

    spi_enable_evt_intr(&spi_host, true);
    spi_enable_rxwm_intr(&spi_host, true);

    for (int i=0; i<8; i++) {
        spi_read_word(&spi_host, &flash_data[i]);
    }

    uint32_t errors = 0;
    uint32_t* ram_ptr = flash_original;
    for (int i=0; i<8; i++) {
        if(flash_data[i] != *ram_ptr) {
            printf("@%x : %x != %x.\n", ram_ptr, flash_data[i], *ram_ptr);
            errors++;
        }
        ram_ptr++;
    }

    if (errors == 0) {
        printf("Success.\n");
        return EXIT_SUCCESS;
    } else {
        printf("Failure, %d errors.\n", errors);
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
