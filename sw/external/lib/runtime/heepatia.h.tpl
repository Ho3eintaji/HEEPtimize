// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia.h
// Author: Michele Caon, Hossein Taji
// Date: 13/05/2023
// Description: Address map for heepatia external peripherals.

#ifndef HEEPATIA_H_
#define HEEPATIA_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "core_v_mini_mcu.h"

// Number of masters and slaves on the external crossbar
#define EXT_XBAR_NMASTER ${xbar_nmasters}
#define EXT_XBAR_NSLAVE ${xbar_nslaves}

// Peripherals address map
// -----------------------
// NM-Caesar
#define CAESAR_NUM ${caesar_num}
%for inst in range(caesar_num):
#define CAESAR${inst}_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${caesar_start_address + inst * caesar_size})
#define CAESAR${inst}_SIZE 0x${caesar_size}
#define CAESAR${inst}_END_ADDRESS (CAESAR${inst}_START_ADDRESS + CAESAR{inst}_SIZE)
%endfor

// NM-Carus
#define CARUS_NUM ${carus_num}
%for inst in range(carus_num):
#define CARUS${inst}_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${carus_start_address + inst * carus_size})
#define CARUS${inst}_SIZE 0x${carus_size}
#define CARUS${inst}_END_ADDRESS (CARUS${inst}_START_ADDRESS + CARUS_SIZE)
%endfor

// OECGRA
#define OECGRA_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_start_address}) //Previously CGRA_START_ADDRESS
#define OECGRA_SIZE 0x${oecgra_size}
#define OECGRA_END_ADDRESS (OECGRA_START_ADDRESS + OECGRA_SIZE)

#define OECGRA_CONTEXT_MEM_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_context_mem_start_address}) 
#define OECGRA_CONTEXT_MEM_SIZE 0x${oecgra_context_mem_size}
#define OECGRA_CONTEXT_MEM_END_ADDRESS (OECGRA_CONTEXT_MEM_START_ADDRESS + OECGRA_CONTEXT_MEM_SIZE)

#define OECGRA_CONFIG_REGS_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_config_regs_start_address}) // previously was CGRA_PERIPH_START_ADDRESS which one it is
#define OECGRA_CONFIG_REGS_SIZE 0x${oecgra_config_regs_size}
#define OECGRA_CONFIG_REGS_END_ADDRESS (OECGRA_CONFIG_REGS_START_ADDRESS + OECGRA_CONFIG_REGS_SIZE)

// FLL
#define FLL_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x${fll_start_address})
#define FLL_SIZE 0x${fll_size}
#define FLL_END_ADDRESS (FLL_START_ADDRESS + FLL_SIZE)

// IMC control register
#define HEEPATIA_CTRL_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x${heepatia_ctrl_start_address})
#define HEEPATIA_CTRL_SIZE 0x${heepatia_ctrl_size}
#define HEEPATIA_CTRL_END_ADDRESS (NMC_CTRL_START_ADDRESS + NMC_CTRL_SIZE)
// im2col registers
#define HEEPATIA_IM2COL_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x${im2col_start_address})
#define HEEPATIA_IM2COL_SIZE 0x${im2col_size}
#define HEEPATIA_IM2COL_END_ADDRESS (HEEPATIA_IM2COL_START_ADDRESS + HEEPATIA_IM2COL_SIZE)


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* HEEPATIA_H_ */
