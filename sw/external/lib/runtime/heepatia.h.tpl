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

// NM-Carus
#define CARUS_NUM ${carus_num}
%for inst in range(carus_num):
%if inst == 0:
#define CARUS${inst}_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${carus_start_address[inst]})
%else:
#define CARUS${inst}_START_ADDRESS (CARUS${inst-1}_END_ADDRESS)
%endif
#define CARUS${inst}_SIZE 0x${carus_size[inst]}
#define CARUS${inst}_END_ADDRESS (CARUS${inst}_START_ADDRESS + CARUS${inst}_SIZE)
%endfor

// OECGRA
#define OECGRA_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_start_address})
#define OECGRA_SIZE 0x${oecgra_size}
#define OECGRA_END_ADDRESS (OECGRA_START_ADDRESS + OECGRA_SIZE)

#define OECGRA_CONTEXT_MEM_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_context_mem_start_address})
#define OECGRA_CONTEXT_MEM_SIZE 0x${oecgra_size}
#define OECGRA_CONTEXT_MEM_END_ADDRESS (OECGRA_CONTEXT_MEM_START_ADDRESS + OECGRA_CONTEXT_MEM_SIZE)

#define OECGRA_CONFIG_REGS_START_ADDRESS (EXT_SLAVE_START_ADDRESS + 0x${oecgra_config_regs_start_address})
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

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* HEEPATIA_H_ */
