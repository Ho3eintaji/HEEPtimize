// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "rv_timer.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "fast_intr_ctrl.h"
#include "gpio.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "heepocrates.h"

#define VCD_TRIGGER_GPIO 0

static gpio_t gpio;

static rv_timer_t timer_0_1;
static const uint64_t kTickFreqHz = 1000 * 1000;
int8_t intr_flag;

void dump_on(void);
void dump_off(void);

void handler_irq_timer(void)
{
    rv_timer_irq_enable(&timer_0_1, 0, 0, kRvTimerDisabled);
    intr_flag = 1;
}

int main(int argc, char *argv[])
{
    fast_intr_ctrl_t fast_intr_ctrl;
    fast_intr_ctrl.base_addr = mmio_region_from_addr((uintptr_t)FAST_INTR_CTRL_START_ADDRESS);

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    uint32_t freq_hz = soc_ctrl_get_frequency(&soc_ctrl);

    mmio_region_t timer_0_1_reg = mmio_region_from_addr(RV_TIMER_AO_START_ADDRESS);
    rv_timer_init(timer_0_1_reg, (rv_timer_config_t){.hart_count = 2, .comparator_count = 1}, &timer_0_1);
    rv_timer_tick_params_t tick_params;
    rv_timer_approximate_tick_params(freq_hz, kTickFreqHz, &tick_params);

    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    uint32_t mask = 1 << 7;
    CSR_SET_BITS(CSR_REG_MIE, mask);

#if CLK_FREQ != 100000000
    uint32_t fll_freq, fll_freq_real;

    fll_t fll;
    fll.base_addr = mmio_region_from_addr((uintptr_t)FLL_START_ADDRESS);

    fll_freq = fll_set_freq(&fll, CLK_FREQ);
    fll_freq_real = fll_get_freq(&fll);
    soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);
#endif

    rv_timer_set_tick_params(&timer_0_1, 0, tick_params);
    rv_timer_irq_enable(&timer_0_1, 0, 0, kRvTimerEnabled);
    rv_timer_arm(&timer_0_1, 0, 0, 1024);

    intr_flag = 0;

    dump_on();
    rv_timer_counter_set_enabled(&timer_0_1, 0, kRvTimerEnabled);
    while(intr_flag==0) {
        wait_for_interrupt();
    }
    dump_off();

    printf("Success.\n");
    return EXIT_SUCCESS;
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
