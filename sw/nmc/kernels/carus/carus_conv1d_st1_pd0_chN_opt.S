// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_conv1d.c
// Author: Francesco Poluzzi
// Date: 26/04/2024
// Description: 1D convolution multi-channel kernel for NM-Carus with stride=1 and padding=0, with an alternative
//  implementations for a most efficient use of the vector registers.

// This kernel computes the multi-channel 1D convolution where each output channel
// results from convolving a corresponding filter with the input vector across its channels:
//       R[c][l] = (A * F[c])[l] = ∑ ( ∑ A[m][l+k] × F[c][m][k] )
//                            m=0...M-1   k=0...f-1
// where:
// - A is an L-long input vector with M channels (A[M][L])
// - R is an output vector with C channels, each being (L-f+1)-long (R[C][L-f+1])
// - F[c] is a f-long filter for channel c, with M channels matching the input (F[C][M][f])
// - L is the length of the input vector
// - f is the length of each filter
// - M is the number of channels in the input vector and filters
// - C is the number of output channels, with each channel having its own filter

// This operation applies each filter to its corresponding channel in the input vector, summing
// across all input channels for each position in the output vector.

// The calling software shall provide the following arguments
// - ARG0: length of the filter (f) | nr of channels (M) | nr of filters (C) (indices in rs2: f:ARG0[20:16] , M: ARG0[12:8], C: ARG0[4:0])
// - ARG1: total elements of the filter (M*f)
// - VL: number of elements of A (L)
// Note: filters have to be stored in the expected output registers, that start from v(input channels)
// Note: the input vector A has to be stored in the vector registers starting from v0
// Note: register 30 and 31 are used by as a temporary register, hence it should not be used by the calling software
// Note: this implementation only considers the case of stride=1 and no zero-padding (padding should be handled by calling software).

// TODO implement bias addition (maybe to be put in the last element of the filter vectors)

# NM-Carus address map
#include "carus_addr_map.h"

#Expected registers
#define TMP_VREG0 30 // v30
#define TMP_VREG1 31 // v31

.section .text
.balign 4
.global _start

_start:
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    vsetvl t0, t0, t1 # set Length of vector A
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers
    lw a4, CTL_REG_ARG0_REG_ADDR(zero) # load ARG0 from control registers
    li tp, 0x0000001F
    and t2, a4, tp   # nr of filters (C)
    srli a1, a4, 8   
    and a1, a1, tp   # nr of channels (M) === input channels === starting output register
    srli a4, a4, 16 # length of the filter (f)
    li tp, 0x00001e00
    add s0, a1, tp # initialize vregs 
    
    add s1, a1, t2 # final output register + 1
    sub sp , t0, a4 # L-f
    addi sp, sp , 1 # R_COLS
    add t2, a1, zero # initialize the filter pointer

FILTER: # iterate over the filters
    
    addi a3, zero, 0 # initialize the channel counter
    addi a2, zero, TMP_VREG0     # restore original slide_vregs_a (first A register is always v0)
    lw tp, CTL_REG_ARG1_REG_ADDR(zero)
    vsetvl zero, tp, t1 # set the vector length to the number of filter elements
    slli gp, t2, 16
    addi gp, gp, TMP_VREG1 # for moving the filter to v31
    xvmv.v.v gp # move the filter to v31
    xvand.vx t2, zero # clear the output register
    addi gp, zero, 0 # initialize the filter element pointer

    CHANNEL: # iterate over the channels
        addi tp, zero, 0 # initialize the filter element counter
        
        L0: # iterate over the filter channel elements
            xvmv.x.s a5, v31, gp # load filter element in a5
            vsetvl zero, t0, t1 # set the vector length to VL
            xvslidedown.vx a2, tp   # slide input vector by i and store it in TMP_VREG0
            vsetvl zero, sp, t1 # set the vector length to sizeof(R)
            xvmacc.vx s0, a5 # multiply-accumulate for the filter element
            addi tp, tp, 1 # increment the filter element counter
            addi gp, gp, 1 # increment the filter element pointer
            bltu tp, a4, L0 # iterate over filter elements

        addi a3, a3, 1 # increment the channel counter
        addi a2, a2, 0x100 # increment input channel pointer in slide_vregs_a
        bltu a3, a1, CHANNEL # iterate over the channels

    addi s0, s0, 1 # increment the output channel pointer in vregs
    addi t2, t2, 1 # increment the filter pointer in vregs
    bltu t2, s1, FILTER # iterate over the filters

    ret