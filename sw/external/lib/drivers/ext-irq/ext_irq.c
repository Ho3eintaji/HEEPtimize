// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: ext_irq.c
// Author: Michele Caon
// Date: 20/06/2023
// Description: External interrupt driver

#include "ext_irq.h"
#include "core_v_mini_mcu.h"
#include "rv_plic.h"
#include "heeperator.h"
#include "carus.h"
#include "vcd_util.h"

/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// External interrupt flag
volatile int8_t carus_irq_flag = 0;
volatile uint32_t carus_irq_idx = 0;

/**********************************/
/* ---- FUNCTION DEFINITIONS ---- */
/**********************************/

int ext_irq_init(void) {
    // Initialize PLIC for external interrupts
    if (plic_Init() != kPlicOk)
        return -1;
    if (plic_irq_set_priority(EXT_INTR_0, 1) != kPlicOk)
        return -1;
    if (plic_irq_set_enabled(EXT_INTR_0, kPlicToggleEnabled) != kPlicOk)
        return -1;

    // Install external interrupt handler
    if (plic_assign_external_irq_handler(EXT_INTR_0, &handler_irq_ext) != kPlicOk)
        return -1;

    // Return success
    return 0;
}

void handler_irq_ext(uint32_t id) {
    carus_ctl_t ctl;

    // Stop VCD dump (NM-Carus power simulation)
    vcd_disable();

    // Check interrupt type
    if (id != EXT_INTR_0) {
        carus_irq_flag = -1;
        return;
    }

    // Check which NM-Carus instance triggered the interrupt
    for (unsigned int i = 0; i < CARUS_NUM; i++) {
        if (carus_get_ctl(i, &ctl) != 0) {
            carus_irq_flag = -1;
            return;
        }
        if (ctl.done == 0) continue;
        
        // Register IRQ
        carus_irq_idx = i;
        carus_irq_flag = 1;
        if (ctl.err != 0) carus_irq_flag = -1;
        
        // Clear done bit
        ctl.done = 0;
        if (carus_set_ctl(i, &ctl) != 0) {
            carus_irq_flag = -1;
            return;
        }
    }

    return;
}
