// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Michele Caon
// Date: 19/06/2023
// Description: Test application for near-memory computing peripherals.

#include "caesar.h"
#include "carus.h"

int main(int argc, char const *argv[])
{
    // Set NM-Caesar in computing mode
    if (caesar_set_mode(0, CAESAR_MODE_COMP) != 0) return 1;
    if (caesar_set_mode(0, CAESAR_MODE_MEM) != 0) return 1;

    // Set NM-Carus in configuration mode
    if (carus_set_mode(0, CARUS_MODE_CFG) != 0) return 1;
    if (carus_set_mode(0, CARUS_MODE_MEM) != 0) return 1;

    return 0;
}
