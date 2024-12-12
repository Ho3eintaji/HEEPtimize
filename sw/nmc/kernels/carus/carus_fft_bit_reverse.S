# Copyright 2024 EPFL.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

# File: carus_fft_bit_reverse.S
# Author: Francesco Poluzzi
# Date: 07/08/2024
# Description: NM-Carus bit-reverse kernel for FFT radix-2

# This kernel performs bit-reversing on multiple input channels. 
# A bit-reversed sequence has to be store in v11 before calling the kernel.
# Channels to get bit-reversed have to be stored starting from v12 (START_REG_KERNEL).
# Arguments:
#     VL: number of input samples multiplied by 2 (so the  number of input values considering both 
#         real and imaginary parts). It must be a power of 2. The maximum value is 128.
#     VTYPE: data type 
#     ARG0: number of channels on which to perform FFT + 12. 
#         It is the index of the last regster that has to be bit-reversed.
#         Channels have to start from v12 and have to be in consecutive registers. 

# This kernel is designed for being performed before calling the carus_fft_iterative_radix2.S kernel.
# It already disposes the data in the right format for the FFT kernel.

# NM-Carus address map
#include "carus_addr_map.h"

.section .text
.balign 4
.global _start

#define BIT_REVERSE 11
#define START_REG_KERNEL 12 # after all the others

_start:
    lw x2, CTL_REG_VL_REG_ADDR(zero) # load VL (input length) from control registers
    lw x3, CTL_REG_VTYPE_REG_ADDR(zero) # load data type from control registers
    lw x4, CTL_REG_ARG0_REG_ADDR(zero) # number of channels
    vsetvl zero, x2, x3 # set vector length to 2*VL and data type to int32
    addi x9, zero, START_REG_KERNEL
    slli x6, x9, 16
    addi x6, x6, 3
    lui x7, (1<<4) 
    
    CHANNELS:
        xvmv.v.v x6 # move channel to v3
        add x6, x6, x7
        addi x14, zero, 0 # index for input

        REVERSE:
            xvmv.x.s x13, v11, x14 # load reversed index in x13
            xvmv.x.s x12, v3, x14 # load real value from input in x12
            xvmv.s.x v0, x12, x13 # store real value in reversed index
            C.ADDI x14, 1           
            blt x14, x2, REVERSE  
        
        xvmv.v.v x9 # move vector from v0 to channel
        C.ADDI x9, 1
        blt x9, x4, CHANNELS
        
  ret 

