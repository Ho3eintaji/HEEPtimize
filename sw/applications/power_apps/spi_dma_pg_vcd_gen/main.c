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
#include "dma.h"
#include "fast_intr_ctrl.h"
#include "fast_intr_ctrl_regs.h"
#include "gpio.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "heepocrates.h"
#include "power_manager.h"

#define COPY_DATA_NUM 16
#define FLASH_CLK_MAX_HZ (133*1000*1000)
#define REVERT_24b_ADDR(addr) ((((uint32_t)(addr) & 0xff0000) >> 16) | ((uint32_t)(addr) & 0xff00) | (((uint32_t)(addr) & 0xff) << 16))
#define VCD_TRIGGER_GPIO 0

spi_host_t spi_host;

static power_manager_t power_manager;

static gpio_t gpio;

void dump_on(void);
void dump_off(void);

uint32_t flash_data[COPY_DATA_NUM] __attribute__ ((aligned (4))) = {0x76543210,0xfedcba98,0x579a6f90,0x657d5bee,0x758ee41f,0x01234567,0xfedbca98,0x89abcdef,0x679852fe,0xff8252bb,0x763b4521,0x6875adaa,0x09ac65bb,0x666ba334,0x44556677,0x0000ba98};
uint32_t copy_data[COPY_DATA_NUM] __attribute__ ((aligned (4)))  = { 0 };

int main(int argc, char *argv[])
{
    mmio_region_t power_manager_reg = mmio_region_from_addr(POWER_MANAGER_START_ADDRESS);
    power_manager.base_addr = power_manager_reg;
    power_manager_counters_t power_manager_counters;
    power_manager_counters_t power_manager_counters_cpu;

    spi_host.base_addr = mmio_region_from_addr((uintptr_t)SPI_START_ADDRESS);

    dma_t dma;
    dma.base_addr = mmio_region_from_addr((uintptr_t)DMA_START_ADDRESS);

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    uint32_t core_clk = soc_ctrl_get_frequency(&soc_ctrl);

#if CLK_FREQ != 100000000
    uint32_t fll_freq, fll_freq_real;

    fll_t fll;
    fll.base_addr = mmio_region_from_addr((uintptr_t)FLL_START_ADDRESS);

    fll_freq = fll_set_freq(&fll, CLK_FREQ);
    fll_freq_real = fll_get_freq(&fll);
    soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);
#endif

    spi_set_enable(&spi_host, true);
    spi_output_enable(&spi_host, true);

    uint32_t *fifo_ptr_rx = spi_host.base_addr.base + SPI_HOST_RXDATA_REG_OFFSET;

    dma_set_read_ptr_inc(&dma, (uint32_t) 0);
    dma_set_write_ptr_inc(&dma, (uint32_t) 4);
    dma_set_read_ptr(&dma, (uint32_t) fifo_ptr_rx);
    dma_set_write_ptr(&dma, (uint32_t) copy_data);
    dma_set_spi_mode(&dma, (uint32_t) 1);
    dma_set_data_type(&dma, (uint32_t) 0);

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
    const uint32_t reset_cmd = 0xFFFFFFFF;
    spi_write_word(&spi_host, reset_cmd);

    const uint32_t cmd_reset = spi_create_command((spi_command_t){
        .len        = 3,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });

    spi_set_command(&spi_host, cmd_reset);
    spi_wait_for_ready(&spi_host);
    const uint32_t powerup_byte_cmd = 0xab;
    spi_write_word(&spi_host, powerup_byte_cmd);

    const uint32_t cmd_powerup = spi_create_command((spi_command_t){
        .len        = 0,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });

    spi_set_command(&spi_host, cmd_powerup);
    spi_wait_for_ready(&spi_host);

    const uint32_t cmd_read = spi_create_command((spi_command_t){
        .len        = 3,
        .csaat      = true,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });

    uint32_t read_byte_cmd;
    read_byte_cmd = ((REVERT_24b_ADDR(flash_data) << 8) | 0x03);

    dma_set_cnt_start(&dma, (uint32_t) (COPY_DATA_NUM*sizeof(*copy_data)));

    const uint32_t cmd_read_rx = spi_create_command((spi_command_t){
        .len        = COPY_DATA_NUM*sizeof(*copy_data) - 1,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirRxOnly
    });

    spi_write_word(&spi_host, read_byte_cmd);
    spi_wait_for_ready(&spi_host);
    spi_set_command(&spi_host, cmd_read);
    spi_wait_for_ready(&spi_host);
    spi_set_command(&spi_host, cmd_read_rx);

    power_gate_counters_init(&power_manager_counters, 0, 0, 0, 0, 0, 0, 0, 0);
    power_gate_counters_init(&power_manager_counters_cpu, 40, 40, 30, 30, 20, 20, 0, 0);

    power_gate_periph(&power_manager, kOff_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 3, kOff_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 4, kOff_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 5, kOff_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 6, kOff_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 7, kOff_e, &power_manager_counters);
    power_gate_external(&power_manager, 0, kOff_e, &power_manager_counters);
    power_gate_external(&power_manager, 1, kOff_e, &power_manager_counters);
    power_gate_external(&power_manager, 2, kOff_e, &power_manager_counters);

    dump_on();
    spi_wait_for_ready(&spi_host);
    CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
    power_gate_core(&power_manager, kDma_pm_e, &power_manager_counters_cpu);
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    dump_off();

    power_gate_periph(&power_manager, kOn_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 3, kOn_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 4, kOn_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 5, kOn_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 6, kOn_e, &power_manager_counters);
    power_gate_ram_block(&power_manager, 7, kOn_e, &power_manager_counters);
    power_gate_external(&power_manager, 0, kOn_e, &power_manager_counters);
    power_gate_external(&power_manager, 1, kOn_e, &power_manager_counters);
    power_gate_external(&power_manager, 2, kOn_e, &power_manager_counters);

    const uint32_t powerdown_byte_cmd = 0xb9;
    spi_write_word(&spi_host, powerdown_byte_cmd);

    const uint32_t cmd_powerdown = spi_create_command((spi_command_t){
        .len        = 0,
        .csaat      = false,
        .speed      = kSpiSpeedStandard,
        .direction  = kSpiDirTxOnly
    });

    spi_set_command(&spi_host, cmd_powerdown);
    spi_wait_for_ready(&spi_host);

    printf("Flash vs ram...\n");

    uint32_t errors = 0;
    uint32_t count = 0;

    for (int i = 0; i<COPY_DATA_NUM; i++) {
        if(flash_data[i] != copy_data[i]) {
            printf("@%08x-@%08x : %02x != %02x.\n" , &flash_data[i] , &copy_data[i], flash_data[i], copy_data[i]);
            errors++;
        }
        count++;
    }

    if (errors == 0) {
        printf("Success (bytes checked: %d).\n", count*sizeof(*copy_data));
        return EXIT_SUCCESS;
    } else {
        printf("Failure, %d errors (out of %d).\n", errors, count);
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
