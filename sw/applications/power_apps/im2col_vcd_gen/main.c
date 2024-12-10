/*
    Copyright EPFL contributors.
    Licensed under the Apache License, Version 2.0, see LICENSE for details.
    SPDX-License-Identifier: Apache-2.0

    Author: Tommaso Terzano <tommaso.terzano@epfl.ch>
                            <tommaso.terzano@gmail.com>
    
    Info: Example application of im2col algorithm with configurable format, verification and performance analysis.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "x-heep.h"
#include "im2col_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "gpio.h"

#define VCD_TRIGGER_GPIO 0

static gpio_result_t gpio_res;
/* Test variables */
int errors;
unsigned int cycles;

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

int main()
{
    // for (int i=START_ID; i<3; i++)
    // {
    dump_on();
      im2col_nchw_int32(2, &cycles);
    dump_off();
    //   #if TEST_EN == 0
    //   PRINTF("im2col NCHW test %d executed\n\r", i);
    //   PRINTF_TIM("Total number of cycles: [%d]\n\r", cycles);
    //   #endif
// 
    //   errors = verify();
// 
    //   if (errors != 0)
    //   {
        //   #if TEST_EN == 0
        //   PRINTF("TEST %d FAILED: %d errors\n\r", i, errors);
        //   return EXIT_FAILURE;
        //   #else
        //   PRINTF_TIM("%d:%d:1\n\r", i, cycles);
        //   #endif
        //   
    //   } 
    //   else
    //   {
        //   #if TEST_EN == 0
        //   PRINTF("TEST PASSED!\n\r\n\r");
        //   #else
        //   PRINTF_TIM("%d:%d:0\n\r", i, cycles);                
        //   #endif
    //   } 
    // }
    // 
    // /* Print the end word for verification */
    // PRINTF("&\n\r");

    return EXIT_SUCCESS;
}
