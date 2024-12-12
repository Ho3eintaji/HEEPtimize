// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_conv1d.c
// Author: Francesco Poluzzi
// Date: 20/03/2024
// Description: 1D convolution kernel for NM-Carus with stride=1 and padding=0

// This kernel computes the 1D convolution with an f-long filter:
//       R[l]=(A∗F)[l]=  ∑     A[l+k]×F[k]
//                    k=0...f−1
// where:
// - A is an L-long input vector 
// - R is an (L-f+1)-long output vector 
// - F is a f-long filter
// The calling software shall provide the following arguments
// - ARG0: number of elments of F (f)
// - ARG1: vA | vR (indices in rs2:  vA: ARG1[12:8], vR: ARG1[4:0])
// - VL: number of elements of A (L)
// Note: filter has to be stored in v31
// Note: register 30 is used by the kernel as a temporary register, hence it should not be used by the calling software
// Note: this implementation only considers stride=1 and padding=0 with a single inpput channel

# NM-Carus address map
#include "carus_addr_map.h"

#Expected registers
#define TMP_VREG0 30 // v30
#define CARUS_CONV1D_F_VREG 31 // v31    register for filter

.section .text
.balign 4
.global _start

_start:
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    vsetvl t0, t0, t1 # set Length of vector A
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load sizeof(f) from control registers
    lw a1, CTL_REG_ARG1_REG_ADDR(zero) # load  vA | vR from control registers
    li tp , TMP_VREG0  # load temporary register

    # Load the mask 0xFFFFFFE0 into a temporary register 
    lui  t2, 0xFFFFF    # Load the upper 20 bits of the mask (0xFFFFFFE0)
    addi t2, t2, -32    # Adjust the lower 12 bits, -32 is 0xFFE0 in two's complement
    and  a2, a1, t2      # Perform bitwise AND with the mask

    # #TODO: check if it works with this instead of the ppreviouus block
    # li t2, 0x1F00
    # and a2, a1, t2

    sub a4, t0, a0 # L-f
    or a2, a2, tp # slide_vregs_a
    lui  t2, 0xFFFFF     # Load the upper 20 bits of the mask (0xFFFFE000)
    addi t2, t2, -257 
    slli tp, tp, 8 # TMP_VREG0 << 8
    and a3, a1, t2 # vR
    or a3, tp, a3 # vregs
    addi a4, a4, 1 # L-f+1 = sizeof(R)
    addi s0 ,zero, 0 # i=0
L0: #iterate over filter elements
    xvmv.x.s s1, v31, s0 # load filter element
    vsetvl zero, t0, t1 # set the vector length to VL
    xvslidedown.vx a2, s0 # slide input vector by i and store it in TMP_VREG0
    vsetvl zero, a4, t1 # set the vector length to sizeof(R)
    addi s0, s0, 1 # i++
    xvmacc.vx a3, s1 # multiply-accumulate for the filter element
    blt s0, a0, L0 # iterate over filter elements

    ret