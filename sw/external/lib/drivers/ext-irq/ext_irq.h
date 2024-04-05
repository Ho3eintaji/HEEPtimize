// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: ext_irq.h
// Author: Michele Caon
// Date: 20/06/2023
// Description: Header file for external IRQ driver

#include "rv_plic.h"

/********************************/
/* ---- EXPORTED VARIABLES ---- */
/********************************/

// NM-Carus interrupt flag
extern volatile int8_t carus_irq_flag;
extern volatile uint32_t carus_irq_idx;

/********************************/
/* ---- EXPORTED FUNCTIONS ---- */
/********************************/

/**
 * @brief Initialize external interrupt handler.
 * @return 0 if successful, -1 otherwise.
 */
int ext_irq_init(void);

/**
 * @brief Serve external interrupt requests.
 * @param id Interrupt ID.
 * @note This function overrides the weak definition in rv_plic.h.
 */
void handler_irq_ext(uint32_t id);
