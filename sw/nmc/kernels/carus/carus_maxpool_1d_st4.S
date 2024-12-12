# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_maxpool_1d_st4.S
# Author: Francesco Poluzzi
# Date: 08/05/2024
# Description: NM-Carus 1d max pooling kernel with size=4 and stride=4, for application SeizDetnm
# there is no possibily of choosing the pooling size and stride to make the kernel smaller.

# This kernel computes the max pooling of a 1D vector, with stride=4.
# 
# - ARG0: starting register of input vectors 
# - VL: cols(A), equal to the vector length
# - ARG1: last register of input vectors +1
# notes: registers v30 and v31 are used as temporary registers, hence they should not be used by the calling software. 
#       ouxput registers will be disposed starting from vector register 0. Hence, it should be ensured that the oux1ut registers do not overlap with the input registers
#       (always guaranteed if the first input register is >= 15).
#       input vectors should have a length multiple of 4, otherwise the kernel will not work correctly.


# NM-Carus address map
#include "carus_addr_map.h"

#Expected registers
#define TMP_VREG0 30 // v30
#define TMP_VREG1 29 // v31

.section .text
.balign 4
.global _start

_start:
    # Read kernel parameters and return effective VL for stripmining
    lw x12, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers -
    lw x13, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers 
    lw x3, CTL_REG_ARG0_REG_ADDR(zero) # starting register of input vectors
    lw x4, CTL_REG_ARG1_REG_ADDR(zero) # last register of input vectors
    li x10, 4 # set pooling size
    li x14, (TMP_VREG0 << 16) # out_vregs
    srli x5, x12, 2 # compute number of output elements
    ROW:
        # move vector to v31
        vsetvl zero, x12, x13 # set VL of the input
        slli x15, x3, 16
        addi x15, x15, TMP_VREG1
        xvmv.v.v x15 # move input vector to v31
        addi x6, zero, 0 # initialize pointer to elements of input vectors
        BATCH:
            li x7, 0x80000000 # initialize max value to -inf
            addi x8, zero, 0 # initialize counter to 4
            ELEMENT:
                add x15, x8, x6 # compute index of element
                xvmv.x.s x9, v29, x15 # load element in x9
                blt x9, x7, SKIP # if element < max, do nothing
                mv x7, x9 # update max
                SKIP:
                addi x8, x8, 1 # increment counter
                bltu x8, x10, ELEMENT
            srli x15, x6, 2 # compute index of output vector
            xvmv.s.x v30, x7, x15 # store max in TMP_VREG0
            addi x6, x6, 4 # increment pointer to elements
            bltu x6, x12, BATCH
        vsetvl zero, x5, x13 # set VL of the output
        xvmv.v.v x14 # move TMP_VREG0 to output vector
        addi x14, x14, 1
        addi x3, x3, 1 # increment pointer to input vector
        bltu x3, x4, ROW
    ret

