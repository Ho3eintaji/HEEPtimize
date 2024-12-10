// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "data.h"
#include "gpio.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "power_manager.h"
#include "x-heep.h"
#include "heepatia_coprosit_ctrl_regs.h"
#include "heepatia.h"
#include "core_v_mini_mcu_memory.h"


/* By default, printfs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA  1
#define PRINTF_IN_SIM   0

#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif PRINTF_IN_FPGA && !TARGET_SIM
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

#define VCD_TRIGGER_GPIO 0

static gpio_result_t gpio_res;

int32_t m_c[DIM*DIM];

void __attribute__ ((noinline)) matrixMul(int32_t *A, int32_t *B, int32_t *C, int N);
uint32_t check_results(int32_t *C, int N);

static power_manager_t power_manager;

int dump_on(void);
void dump_off(void);

//defined in the linker script
extern uint32_t _edata;

int main(int argc, char *argv[])
{
    uint32_t errors = 0;

    mmio_region_t power_manager_reg = mmio_region_from_addr(POWER_MANAGER_START_ADDRESS);
    power_manager.base_addr = power_manager_reg;
    power_manager_counters_t power_manager_counters;
    power_gate_counters_init(&power_manager_counters, 30, 30, 30, 30, 30, 30, 0, 0);

     //clock gate peripheral subsystem
    mmio_region_write32(power_manager.base_addr, (ptrdiff_t)(POWER_MANAGER_PERIPH_CLK_GATE_REG_OFFSET), 0x1);

    //clock and power gate external domains, i.e. Carus and CGRA
    for(uint32_t i = 0; i < EXTERNAL_DOMAINS; ++i) {
        //clock gate
        mmio_region_write32(power_manager.base_addr, (ptrdiff_t)(power_manager_external_map[i].clk_gate), 0x1);
        // Power off external domain
        power_gate_external(&power_manager, i, kOff_e, &power_manager_counters);
        // Check that the external domain is actually OFF
        while(!external_power_domain_is_off(&power_manager, i));
    }

    //clock gate coprosit-cpu
    volatile uint32_t *cpu_controller_clken = (uint32_t *)(HEEPATIA_COPROSIT_CTRL_START_ADDRESS + HEEPATIA_COPROSIT_CTRL_CLKEN_N_REG_OFFSET);
    *cpu_controller_clken = 0;


    //clock gate ununsed memory banks

    // ------------ clock gating ------------
    // do not clock gate instruction and data memory (usually first 2 banks)

    uint32_t end_data_addr = (uint32_t)(&_edata);
    PRINTF("%x\n",end_data_addr);
    for(uint32_t i = 0; i < MEMORY_BANKS; ++i) {
        if (end_data_addr < xheep_memory_regions[i].start) {
            PRINTF("off %x\n",i);
            mmio_region_write32(power_manager.base_addr, (ptrdiff_t)(power_manager_ram_map[i].clk_gate), 0x1);
            power_gate_ram_block(&power_manager, i, kOff_e, &power_manager_counters);
            while(!ram_block_power_domain_is_off(&power_manager, i));
        }
    }


    dump_on();
    matrixMul(m_a, m_b, m_c, DIM);
    dump_off();


    errors = check_results(m_c, DIM);

    if (errors == 0) {
        PRINTF("Success.\n");
        return EXIT_SUCCESS;
    } else {
        PRINTF("Failure: %d errors.\n", errors);
        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;
}

int dump_on(void)
{

    gpio_cfg_t cfg_out = {
        .pin = VCD_TRIGGER_GPIO,
        .mode = GpioModeOutPushPull
    };

    gpio_res = gpio_config(cfg_out);

    if (gpio_res != GpioOk) {
        return -1;
    }

    gpio_res = gpio_write(VCD_TRIGGER_GPIO, true);

    return 0;
}

void dump_off(void)
{
    gpio_res = gpio_write(VCD_TRIGGER_GPIO, false);
}

void __attribute__ ((noinline)) matrixMul(int32_t *A, int32_t *B, int32_t *C, int N)
{
    int i, j, k;

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            C[i*N+j] = 0;
            for (k = 0; k < N; k++) {
                C[i*N+j] += A[i*N+k] * B[k*N+j];
            }
        }
    }
}

uint32_t check_results(int32_t *C, int N)
{
    int i, j;
    uint32_t err = 0;

    for(i = 0; i < N; i++) {
        for(j = 0; j < N; j++) {
            if(C[i*N+j] != m_exp[i*N+j]) {
                err++;
                PRINTF("Error at index %d, %d, expected %d, got %d.\n", i, j, m_exp[i*N+j], C[i*N+j]);
            }
        }
    }

    return err;
}
