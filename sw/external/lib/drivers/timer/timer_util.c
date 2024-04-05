// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: timer_util.c
// Author: Michele Caon
// Date: 31/07/2023
// Description: Timer functions

#include <stdint.h>

#include "timer_util.h"
#include "csr.h"

/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// Timer value
uint32_t timer_value = 0;

/*************************************/
/* ---- FUNCTION IMPLEMENTATION ---- */
/*************************************/

// Initialize the timer
void timer_init() {
    // Enable MCYCLE counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
}

// Initialize NM-Caesar timer (GPIO trigger)
int timer_caesar_init() {
    // Configure GPIO pin as output with push and pull
    gpio_cfg_t pin_cfg = {
        .pin = TIMER_CAESAR_GPIO,
        .mode = GpioModeOutPushPull
    };

    // Write configuration
    if (gpio_config(pin_cfg) != GpioOk)
        return -1;

    // Return success
    return 0;
}

// Start NM-Caesar timer (enable GPIO trigger)
void timer_caesar_start() {
    gpio_write(TIMER_CAESAR_GPIO, true);
}

// Stop NM-Caesar timer (disable GPIO trigger)
void timer_caesar_stop() {
    gpio_write(TIMER_CAESAR_GPIO, false);
}
