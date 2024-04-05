# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_conv2d.S
# Author: Michele Caon
# Date: 28/10/2023
# Description: 2D convolution kernel for NM-Carus

# This kernel computes the 2D convolution with an [fxf] filter:
#       R[(m-f+1) x (n-f+1)] = A[m x n] * F[f x f]
# where:
# - A is an MxN matrix (10xVLMAX at most) whose elements are stored in v0-v9
# - R is an (M-2)x(VLMAX) matrix whose elements are stored in v20-v27
# - F is a 3x3 filter, flattened and stored in v31
# The calling software shall provide the following arguments
# - ARG0: number of rows of A (m)
# - ARG1: number of rows and columns of F (f)
# - VL: number of columns of A (n)
# Registers v10-v19 are used as temporary buffers for slided versions of A.

# NM-Carus address map
#include "carus_addr_map.h"

# Expected registers
#define CARUS_CONV2D_A_VREG 0 # v0-v9
#define CARUS_CONV2D_R_VREG 20 # v20-v27
#define CARUS_CONV2D_F_VREG 31 # v31 (flattened and transposed)

.section .text
.balign 4
.global _start

_start:
    # Configure vector type and length
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    li s0, 0 # initialize index of filter column
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    li s1, 0 # initialize index of output row
    vsetvl t0, t0, t1 # set VL and SEW
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    li a2, 0 # initialize index of filter row

    # Load kernel parameters
    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load rows(A) from control registers
    li t0, 0 # initialize pointer to filter element
    lw a1, CTL_REG_ARG1_REG_ADDR(zero) # load rows(F) from control registers
    li t1, 0x000014 # initialize target registers (vs1=v0, vs2=v0, vd=v20)
    sub a3, a0, a1 # a3 = m - f
    addi a3, a3, 1 # a3 = m - f + 1

L0: # iterate over the filter rows (fi)
    xvmv.x.s tp, v31, t0 # load filter element
    xvmacc.vx t1, tp # R[yi,:] += F[fi,fj] * A[yi+fi,fj+1:n]
    add t0, t0, a1 # increment pointer to filter element
    addi t1, t1, 0x000100 # increment vs2 (input row index)
    addi a2, a2, 1 # increment filter row index
    blt a2, a1, L0 # if fi < f, repeat L0

    # Iterate over the output rows
    li a2, 0 # reset filter row index
    slli t2, a1, 8 # t2 = f << 8
    sub t1, t1, t2 # reset input row pointer vs2 = vs2 - f
    addi t1, t1, 0x000101 # increment input and output rows (vs++, vd++)
    addi s1, s1, 1 # increment output row index
    mv t0, s0 # reset pointer to filter element
    blt s1, a3, L0 # if yi < m-f+1, repeat L0

    # Iterate over the filter columns
    addi s0, s0, 1 # increment filter column index
    li t2, 0x00000a # initialize target buffer register (vd=v10)
    li t0, 0 # reset pointer to input row

L1: # iterate over the input rows
    xvslidedown.vx t2, s0 # shift input row by filter column index
    addi t2, t2, 0x000101 # increment source and destination (vs++, vd++)
    addi t0, t0, 1 # increment input row index
    blt t0, a0, L1 # if xi < m, repeat L1

    mv t0, s0 # t0 = s0
    li t1, 0x000a14 # reset target registers (vs1=v0, vs2=v10, vd=v20)
    li s1, 0 # reset output row index
    blt s0, a1, L0 # if fj < f, repeat L0

    # Return from the kernel
    ret # return
