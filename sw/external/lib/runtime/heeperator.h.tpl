// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heeperator.h
// Author: Michele Caon
// Date: 13/05/2023
// Description: Address map for HEEPerator external peripherals.

#ifndef HEEPERATOR_H_
#define HEEPERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "core_v_mini_mcu.h"

// Number of masters and slaves on the external crossbar
#define EXT_XBAR_NMASTER ${xbar_nmasters}
#define EXT_XBAR_NSLAVE ${xbar_nslaves}

// Peripherals address map
// -----------------------

// NM-Carus
#define CARUS_NUM ${carus_num}
%for inst in range(carus_num):
#define CARUS${inst}_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${carus_start_address + inst * carus_size})
#define CARUS${inst}_SIZE 0x${carus_size}
#define CARUS${inst}_END_ADDRESS (CARUS${inst}_START_ADDRESS + CARUS_SIZE)
%endfor

// FLL
#define FLL_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x${fll_start_address})
#define FLL_SIZE 0x${fll_size}
#define FLL_END_ADDRESS (FLL_START_ADDRESS + FLL_SIZE)

// IMC control register
#define HEEPERATOR_CTRL_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x${heeperator_ctrl_start_address})
#define HEEPERATOR_CTRL_SIZE 0x${heeperator_ctrl_size}
#define HEEPERATOR_CTRL_END_ADDRESS (NMC_CTRL_START_ADDRESS + NMC_CTRL_SIZE)

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* HEEPERATOR_H_ */
