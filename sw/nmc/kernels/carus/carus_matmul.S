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
# - ARG0: number of rows of R (M)
# - ARG1: number of rows of B (N)
# - VL: number of columns of R (P)
# Registers are assigned to maximize the number of rows of B and R that can fit
# in the vector register file, that is 15 and 16 respectively.

# NM-Carus address map
#include "carus_addr_map.h"

# Expected registers
#define CARUS_MATMUL_A_VREG 0  // v0 (flattened)
#define CARUS_MATMUL_B_VREG 1  // v1-v15 (15 rows max)
#define CARUS_MATMUL_R_VREG 16 // v16-v31 (16 rows max)

.section .text
.balign 4
.global _start

_start:
    # Read kernel parameters and return effective VL for stripmining
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    li a2, 0 # initialize pointer to row vectors of R (m < M)
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    li a3, 0 # initialize pointer to row vectors of B (n < N)
    vsetvl t0, t0, t1 # set VL and SEW
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    li t0, 0 # initialize pointer to the elements of A
    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load rows(R) from control registers
    li t1, 0x000110 # initialize pointers to B (vs2=v1) and R (vd=v16)
    lw a1, CTL_REG_ARG1_REG_ADDR(zero) # load rows(B) from control registers

L0: # iterate over rows of B
    xvmv.x.s s1, v0, t0 # load A[t0] into s1
    addi t0, t0, 1 # increment pointer to the elements of A
    bnez a3, B0 # if n > 0, skip initialization of R[m,:]
    xvmul.vx t1, s1 # R[m,:] = A[m,n] * B[n,:] (faster than MACC)
    j B1 # skip MACC
B0:
    xvmacc.vx t1, s1 # R[m,:] += A[m,n] * B[n,:]
B1:
    addi a3, a3, 1 # increment pointer to row vectors of B
    addi t1, t1, 0x0100 # increment pointer to B (vs2) while leaving vd as it is
    bltu a3, a1, L0 # iterate over n < N
    andi t1, t1, 0x00ff # reset pointer to B (vs2)
    addi t1, t1, 0x0101 # set pointer to B (vs2=v1) and increment pointer to R (vd++)
    addi a2, a2, 1 # increment pointer to row vectors of R
    li a3, 0 # reset pointer to row vectors of B
    bltu a2, a0, L0 # iterate over m < M

    # Return from the kernel
    ret
