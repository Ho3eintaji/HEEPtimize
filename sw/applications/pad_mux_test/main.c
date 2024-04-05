// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "pad_control.h"
#include "pad_control_regs.h"
#include "gpio.h"

#define GPIO_24 24

static pad_control_t pad_control;
static gpio_t gpio;

int main(int argc, char *argv[])
{
    // Setup pad_control
    mmio_region_t pad_control_reg = mmio_region_from_addr(PAD_CONTROL_START_ADDRESS);
    pad_control.base_addr = pad_control_reg;

    // Setup gpio
    gpio_params_t gpio_params;
    gpio_params.base_addr = mmio_region_from_addr((uintptr_t)GPIO_START_ADDRESS);
    gpio_init(gpio_params, &gpio);
    gpio_output_set_enabled(&gpio, GPIO_24, true);

    // Switch pad mux to GPIO_24
    pad_control_set_mux(&pad_control, (ptrdiff_t)PAD_CONTROL_PAD_MUX_SPI_CS_1_REG_OFFSET, 1);

    // Generate output PWM
    for(int i=0;i<100;i++) {
      gpio_write(&gpio, GPIO_24, true);
      for(int i=0;i<10;i++) asm volatile("nop");
      gpio_write(&gpio, GPIO_24, false);
      for(int i=0;i<10;i++) asm volatile("nop");
    }

    /* write something to stdout */
    printf("Success.\n");
    return EXIT_SUCCESS;
}
