# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_xor.S
# Author: Michele Caon
# Date: 20/11/2023
# Description: NM-Carus kernel for vector addition

# This kernel computes the addition between two vectors A and B
# The calling software shall provide the following arguments:
# - VL: length of A and B

# NM-Carus address map
#include "carus_addr_map.h"

# Expected registers
#define CARUS_ADD_A_VREG 0  // v0
#define CARUS_ADD_B_VREG 16 // v10
#define CARUS_ADD_R_VREG 0  // v20

.section .text
.balign 4
.global _start

_start:
    # Read kernel parameters and return effective VL for stripmining
    lw a0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    mv s0, a0 # initialize strip-mining initial value
    lw s1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    li a1, CARUS_ADD_B_VREG # initialize vector counter

    # Stripmining loop
    li t1, ((CARUS_ADD_A_VREG << 16) | (CARUS_ADD_B_VREG << 8) | CARUS_ADD_R_VREG) # initialize vectors (vs1=v0, vs2=v10, vd=v20
    li t2, 0x010101 # vector register index increment
    li a2, 1
L0:
    vsetvl t0, s0, s1 # set VL and SEW
    sub a1, a1, a2 # decrement available registers
    sub s0, s0, t0 # update remaining elements
    xvadd.vv t1 # compute vector addition on VL elements
    add t1, t1, t2 # increment register indexes
    beqz a1, B0 # exit if out of registers
    bgtz s0, L0 # repeat if there are left elements

B0:
    sub a0, a0, s0 # processed elements
    sw a0, CTL_REG_VL_REG_ADDR(zero) # store number of processed items
    ret
