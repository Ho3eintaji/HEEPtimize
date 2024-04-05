# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_gemm.S
# Author: Michele Caon
# Date: 28/10/2023
# Description: NM-Carus general matrix multiplication kernel

# This kernel computes the general matrix multiplication:
#       R = alpha x A x B + beta x C
# where:
# - A is an MxN matrix (10x10 at most) whose elements are stored in v0
# - B is an NxP matrix (10xVLMAX at most) whose elements are stored in v2-v11
# - C is an MxP matrix (10xVLMAX at most) whose elements are stored in v12-v21
# - R is an MxP matrix (10xVLMAX at most) whose elements are stored in v22-v31
# The calling software shall provide the following arguments
# - ARG0: number of rows of C and R (M)
# - ARG1: number of rows of B (N)
# - ARG2: alpha coefficient (AxB multiplier)
# - ARG3: beta coefficient (C multiplier)
# - ARG4 (in SCRATCH register): number of elements of A (MxN)
# - VL: number of columns of C and R (P)
# Registers are assigned to maximize the number of rows of B, C, and R that can fit
# in the vector register file, that is 10 and 11 respectively.

# NM-Carus address map
#include "carus_addr_map.h"

# Expected registers
#define CARUS_GEMM_A_VREG 0  // v0 (flattened)
#define CARUS_GEMM_B_VREG 2  // v2-v11 (10 rows max)
#define CARUS_GEMM_C_VREG 12 // v12-v21 (10 rows max)
#define CARUS_GEMM_R_VREG 22 // v22-v31 (10 rows max)

.section .text
.balign 4
.global _start

_start:
    # Load kernel parameters
    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load rows(R) from kernel params
    lw a1, CTL_REG_ARG1_REG_ADDR(zero) # load rows(B) from kernel params
    lw a2, CTL_REG_ARG2_REG_ADDR(zero) # load alpha coefficient from kernel params
    lw a3, CTL_REG_ARG3_REG_ADDR(zero) # load beta coefficient from kernel params
    
    # Compute alpha x A
    lw t0, CTL_REG_SCRATCH_REG_ADDR(zero) # get length of flattened A (M x N)
    li s0, 0 # initialize pointer to row vectors of R (m < M)
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # get vtype value
    li s1, 0 # initialize pointer to row vectors of B (n < N)
    vsetvl t0, t0, t1 # set vector length to M x N
    vmul.vx v1, v0, a2 # v1 = alpha x A (A is flattened)

    # Set the vector length to P (columns of B, C, and R) and return effective VL
    lw t0, CTL_REG_VL_REG_ADDR(zero) # get vl value
    vsetvl t0, t0, t1 # set vector length to P (columns of B, C, and R)
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    li t0, 0 # initialize pointer to the elements of A
    li t1, 0x000c16 # initialize pointers to C (vs2=v12) and R (vd=v22)

L0: # iterate over rows of C
    xvmul.vx t1, a3 # R[m,:] = beta x C[m,:]
    addi t1, t1, 0x0101 # increment pointer to C (vs2++) and R (vd++)
    addi s0, s0, 1 # increment pointer to row vectors of C and R
    bltu s0, a0, L0 # iterate over m < M
    li s0, 0 # reset pointer to row vectors of C and R
    li t1, 0x000216 # reset pointers to B (vs2=v2) and R (vd=v22)
L1: # iterate over rows of B
    xvmv.x.s tp, v1, t0 # load alpha*A[t0] into tp
    addi t0, t0, 1 # increment pointer to the elements of A
    xvmacc.vx t1, tp # R[m,:] += alpha * A[m,n] * B[n,:]
    addi s1, s1, 1 # increment pointer to row vectors of B
    addi t1, t1, 0x0100 # increment pointer to B (vs2) while leaving vd as it is
    bltu s1, a1, L1 # iterate over n < N
    andi t1, t1, 0x00ff # reset pointer to B (vs2)
    addi t1, t1, 0x0201 # set pointer to B (vs2=v1) and increment pointer to R (vd++)
    addi s0, s0, 1 # increment pointer to row vectors of R
    li s1, 0 # reset pointer to row vectors of B
    bltu s0, a0, L1 # iterate over m < M

    # Return from the kernel
    ret
