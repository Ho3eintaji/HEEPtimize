// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: caesar.c
// Author: Michele Caon
// Date: 19/06/2023
// Description: Driver for NM-Caesar near-memory computing peripheral.

#include "caesar.h"
#include "heepatia.h"
#include "heepatia_ctrl_reg.h"

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/
// Get NM-Caesar operating mode
int caesar_get_mode(const uint8_t inst, caesar_mode_t *mode) {
    uint32_t *op_mode = (uint32_t *) (HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_OP_MODE_REG_OFFSET);

    // Check instance number
    if (inst > CAESAR_NUM-1) {
        return -1;
    }

    // Get mode
    if (*op_mode & (1 << (inst + HEEPATIA_CTRL_OP_MODE_CAESAR_IMC_0_BIT))) {
        *mode = CAESAR_MODE_COMP;
    } else {
        *mode = CAESAR_MODE_MEM;
    }

    return 0;
}

// Set NM-Caesar operating mode
int caesar_set_mode(const uint8_t inst, const caesar_mode_t mode) {
    uint32_t *op_mode = (uint32_t *) (HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_OP_MODE_REG_OFFSET);

    // Check instance number
    if (inst > CAESAR_NUM-1) {
        return -1;
    }

    // Set mode
    if (mode == CAESAR_MODE_COMP) {
        *op_mode |= (1 << (inst + HEEPATIA_CTRL_OP_MODE_CAESAR_IMC_0_BIT));
        asm volatile("nop"); // Wait device configuration to be completed
    } else {
        *op_mode &= ~(1 << (inst + HEEPATIA_CTRL_OP_MODE_CAESAR_IMC_0_BIT));
        asm volatile("nop"); // Wait device configuration to be completed
    }

    return 0;
}
