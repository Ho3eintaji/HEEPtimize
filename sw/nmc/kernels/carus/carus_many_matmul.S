# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_matmul.S
# Author: Michele Caon
# Date: 28/10/2023
# Description: NM-Carus matrix multiplication kernel

# This kernel computes the matrix multiplication R = A x B, where:
# - A is an MxN matrix (15x16 at most) whose elements are stored in v0
# - B is an NxP matrix (16xVLMAX at most) whose elements are stored in v1-v15
# - R is an MxP matrix (15xVLMAX at most) whose elements are stored in v16-v31
# The calling software shall provide the following arguments
# - ARG0: number of rows of B (N) [31:16] | number of rows of R (M) [15:0]
# - ARG1: number of channels to accumulate
# - VL: number of columns of R (P)
# - ARG2: vA | vB | vR (indices in VRF)
# - ARG3: starting point in the A VR
# Registers are assigned to maximize the number of rows of B and R that can fit
# in the vector register file, that is 15 and 16 respectively.

# NM-Carus address map
#include "carus_addr_map.h"


.section .text
.balign 4
.global _start

_start:
    # Read kernel parameters and return effective VL for stripmining
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    li a2, 0 # Initialize pointer to channels to accumulate
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    li a3, 0 # initialize pointer to row vectors of B (n < N)
    vsetvl t0, t0, t1 # set VL and SEW
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers

    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load rows(R) | rows(B) from control registers
    srai a1, a0, 16 # extract rows(R) from ARG0
    li t2, 0x0000ffff # mask for extracting rows(B) from ARG0
    and a0, a0, t2 # extract rows(B) from ARG0
    
    lw t1, CTL_REG_ARG2_REG_ADDR(zero) # load vA | vB | vR from control registers
    lw t2, CTL_REG_ARG1_REG_ADDR(zero) # load the number of channels to accumulate
    lw t0, CTL_REG_ARG3_REG_ADDR(zero) # load starting point in the A VR

L0: # iterate over rows of B
    xvmv.x.s s1, v0, t0 # load A[t0] into s1
    addi t0, t0, 1 # increment pointer to the elements of A
    xvmacc.vx t1, s1 # R[m,:] += A[m,n] * B[n,:]
    addi a3, a3, 1 # increment pointer to row vectors of B
    addi t1, t1, 0x0100 # increment pointer to B (vs2) while leaving vd as it is
    bltu a3, a1, L0 # iterate over n < N
    li a3, 0 # initialize pointer to row vectors of B (n < N)
    addi a2, a2, 1; # a2 = a2 + 1
    bltu a2, t2, L0 # iterate over the number of channels to accumulate

RET:
    ret
