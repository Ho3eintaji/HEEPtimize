// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: caesar.h
// Author: Michele Caon
// Date: 19/06/2023
// Description: Header file for NM-Caesar driver

#ifndef CAESAR_H_
#define CAESAR_H_

#include <stdint.h>

/************************/
/* ---- DATA TYPES ---- */
/************************/

typedef enum {
    CAESAR_MODE_MEM, // Memory mode
    CAESAR_MODE_COMP, // Computation mode
} caesar_mode_t;

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/

/**
 * @brief Get the specified NM-Caesar instance operating mode.
 * @param inst NM-Caesar instance number.
 * @param mode Pointer to the variable where the operating mode will be stored.
 * @return 0 if success, -1 otherwise.
 */
int caesar_get_mode(const uint8_t inst, caesar_mode_t *mode);

/**
 * @brief Set the specified NM-Caesar instance operating mode.
 * @param inst NM-Caesar instance number.
 * @param mode Operating mode (memory or computing).
 * @return 0 if success, -1 otherwise.
 */
int caesar_set_mode(const uint8_t inst, const caesar_mode_t mode);

#endif // CAESAR_H_
