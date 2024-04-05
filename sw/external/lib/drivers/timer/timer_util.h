// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: timer_util.h
// Author: Michele Caon
// Date: 31/07/2023
// Description: Execution time measurements utilities

#ifndef TIMER_UTIL_H_
#define TIMER_UTIL_H_

#include <stdint.h>

#include "csr.h"
#include "gpio.h"

#define TIMER_CAESAR_GPIO 1

/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// Timer value
extern uint32_t timer_value;

/********************************/
/* ---- EXPORTED FUNCTIONS ---- */
/********************************/

/**
 * @brief Initialize the timer
 * 
 */
void timer_init();

/**
 * @brief Get the current value of the MCYCLE CSR
 * 
 * @return int64_t Current value of the MCYCLE CSR
 */
inline uint32_t timer_get_cycles() {
    uint32_t cycle_count;
    CSR_READ(CSR_REG_MCYCLE, &cycle_count);
    return cycle_count;
}

/**
 * @brief Start the timer
 * 
 */
inline void timer_start() {
    timer_value = -timer_get_cycles();
}

/**
 * @brief Stop the timer
 * 
 * @return int64_t Elapsed time in clock cycles
 */
inline uint32_t timer_stop() {
    timer_value += timer_get_cycles();
    return timer_value;
}

/**
 * @brief Initialize NM-Caesar timer (GPIO trigger)
 * 
 * @return int Initialization outcome
 */
int timer_caesar_init();

/**
 * @brief Start the NM-Caesar timer
 * 
 */
void timer_caesar_start();

/**
 * @brief Stop the NM-Caesar timer
 * 
 */
void timer_caesar_stop();

#endif /* TIMER_H_ */
