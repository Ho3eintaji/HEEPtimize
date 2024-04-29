# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_batchnorm.S
# Author: Luigi Giuffrida
# Date: 08/03/2024
# Description: NM-Carus kernel for batch normalization

# This kernel computes the batch normalization of a vector with mean var gamma and beta
# A = [(A - mean) * var * gamma] + beta
# The calling software shall provide the following arguments:
# - VL: length of the vector
# - ARG0: 00 | A | A 
# - ARG1: mean
# - ARG2: var
# - ARG3: gamma
# - ARG4: beta

# NM-Carus address map
#include "carus_addr_map.h"

.section .text
.balign 4
.global _start

_start:

    lw t0, CTL_REG_VL_REG_ADDR(zero) # get vl value
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    vsetvl t0, t0, t1 # set vector length to P (columns of B, C, and R)
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    # Read kernel parameters
    lw x3, CTL_REG_ARG0_REG_ADDR(zero) # load A | A
    lw x4, CTL_REG_ARG1_REG_ADDR(zero) # load - mean
    lw x5, CTL_REG_ARG2_REG_ADDR(zero) # load var * gamma
    lw x6, CTL_REG_ARG3_REG_ADDR(zero) # load beta
    
    xvadd.vx x3, x4  # A - mean
    xvmul.vx x3, x5  # (A - mean) * var * gamma
    xvadd.vx x3, x6  # (A - mean) * var * gamma + beta
    
    ret
