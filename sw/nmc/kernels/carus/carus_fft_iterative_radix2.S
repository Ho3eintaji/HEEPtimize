# Copyright 2024 EPFL.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

# File: carus_fft_iterative_radix2.S
# Author: Francesco Poluzzi
# Date: 04/09/2024
# Description: NM-Carus iterative fft radix-2

# This kernel performs FFT radix 2 on multiple input channels. 
# Data is expected to be in interleaved format: 
# [real[0], imag[0], real[1], imag[1], real[2], imag[2], ..... ]
# Inputs are expected to be in fixed point format, with 8 decimal bits.
# To change the number of decimal bits, modify the immediate value in the instructions:
# vsra.vi v3, v3, 8
# vsra.vi v4, v4, 8

# Arguments:
#     VL: number of input samples multiplied by 2 (so the  number of input values considering both 
#         real and imaginary parts). It must be a power of 2. The maximum value is 128.
#     VTYPE: data type 
#     ARG0: number of channels over which to perform FFT + START_REG. Channels have to start from START_REG and have to
#         be in consecutive registers. The value to pass in ARG0 is the position of the final channel. Numeric value of
#         START_REG is in sw/external/carus/kernels/carus_fft_iterative_radix2.h
#     ARG1: number of stages. This is the log2 of the number of input samples.

# Twiddle factors are expected to be precomputed and ordered for each stage and stored in the register TWIDDLE_REAL_REG for the
# real part and TWIDDLE_IMAG_REG for the imaginary part before running the kernel. An example on how to do it is avaliable in
# sw/applications/carus-fft_radix_2_iterative/main.c.

# Also, the kernel expects the data to be in bit-reversed order. An example for bit-reversing an input
# sequence is available in the file sw/applications/carus-fft_radix_2_iterative/fft_iterative_radix_2.h , in the function:
#     void reverse_array(int16_t* x, int16_t* xrev, uint32_t N);
# Alternatively, inputs can be bit-reversed by using the kernel sw/nmc/kernels/carus/carus_fft_bit_reverse.S

#include "carus_addr_map.h"

.section .text
.balign 4
.global _start

# Expected registers
#define TWIDDLE_REAL_REG 1  # do not change 
#define TWIDDLE_IMAG_REG 2  # do not change
#define TEMP_REG_0 0 
#define TEMP_REG_1 3 # temp_real
#define TEMP_REG_2 4 # temp_imag
#define A_REAL 5
#define A_IMAG 6
#define B_REAL 7
#define B_IMAG 8
#define TWIDDLE_TEMP_REAL 9
#define TWIDDLE_TEMP_IMAG 10
#define BIT_REVERSE_REG 11
#define START_REG 12 # after all the others

_start:
    lw x2, CTL_REG_VL_REG_ADDR(zero) # load VL (input length) from control registers
    lw x3, CTL_REG_VTYPE_REG_ADDR(zero) # load data type from control registers
    lw x8, CTL_REG_ARG1_REG_ADDR(zero) # number of stages
    vsetvl zero, x2, x3 # set vector length to 2*VL and data type to int32
    C.LI x9, START_REG  # initialize channel counter

    CHANNEL:
        slli x6, x9, 16
        xvmv.v.v x6     # copy channel in v0
        addi x5 ,zero ,1  # initialize stage counter
        C.LI x10,  2  # initialize step
        addi x15, zero, 0  # initialize twiddle index

        // UNROLL THE FIRST STAGE (twiddles are only 0 and 1) => only data movement
        addi x12, zero, 0  # initialize i
        FIRST_STAGE:
            xvmv.x.s x13, v0, x12 # a_real -> x13
            addi x7, x12, 2 # 2*(i+halfstep)
            xvmv.x.s x6, v0, x7 # b_real -> x6
            add x14, x13, x6 # a_real + b_real -> x14
            sub x13, x13, x6 # a_real - b_real -> x13
            xvmv.s.x v0, x14, x12 # store a_real + b_real
            xvmv.s.x v0, x13, x7 # store a_real - b_real
            C.ADDI x12, 1 
            C.ADDI x7, 1  
            xvmv.x.s x13, v0, x12 # a_imag -> x13
            xvmv.x.s x6, v0, x7 # b_imag -> x6
            add x14, x13, x6 # a_imag + b_imag -> x14
            sub x13, x13, x6 # a_imag - b_imag -> x13
            xvmv.s.x v0, x14, x12 # store a_imag + b_imag
            xvmv.s.x v0, x13, x7 # store a_imag - b_imag
            C.ADDI x12, 3
            blt x12, x2, FIRST_STAGE

        STAGE:    
            vsetvl zero, x10, x3
            vslidedown.vx v9, v1, x15 # twiddle_temp_real
            vslidedown.vx v10, v2, x15 # twiddle_temp_imag
            vsetvl zero, x2, x3

            mv x7, x10  # initialize halfstep
            C.SLLI x10, 1  # double step
            addi x11, zero , 0  # initialize i
            
            STEP:

                addi x12, zero, 0  # initialize j
                slli x14, x11, 1 # initialize 2*(i)
                add x4, x14, x10 # initialize 2*(i+halfstep)
                
                // SCALAR DATA MOVEMENT
                ORGANIZE_DATA:
                    xvmv.x.s x13, v0, x14 # a_real -> x13
                    xvmv.s.x v5, x13, x12  # store a_real 
                    C.ADDI x14, 1 # 2*(i+j)
                    xvmv.x.s x13, v0, x14 # a_imag -> x13
                    C.ADDI x14, 1 # 2*(i+j) + 1
                    xvmv.s.x v6, x13, x12  # store a_imag
                    xvmv.x.s x13, v0, x4 # b_real -> x6
                    xvmv.s.x v7, x13, x12  # store b_real
                    C.ADDI x4, 1  # 2*(i+j+halfstep)
                    xvmv.x.s x13, v0, x4 # b_imag -> x13
                    C.ADDI x4, 1  # 2*(i+j+halfstep)+1
                    xvmv.s.x v8, x13, x12  # store b_imag
                    C.ADDI x12, 1  # increment j
                    blt x12, x7, ORGANIZE_DATA  # loop over j 

                // VECTOR OPERATIONS
                vsetvl zero, x7, x3 # set vector length to step and data type to int32
                vmul.vv v3, v9, v7 # w_real*b_real
                vmul.vv v4, v10, v8 # w_imag*b_imag
                vsub.vv v3, v3, v4 # w_real*b_real - w_imag*b_imag = temp_real -> v3
                vmul.vv v4, v9, v8 # w_imag*b_real
                vmacc.vv v4, v10, v7 # b_real*w_imag + b_imag*w_real = temp_imag -> v4
                vsra.vi v3, v3, 8
                vsra.vi v4, v4, 8
                vadd.vv v7, v5, v3 # a_real + temp_real -> v7
                vadd.vv v8, v6, v4 # a_imag + temp_imag -> v8
                vsub.vv v5, v5, v3 # a_real - temp_real -> v5
                vsub.vv v6, v6, v4 # a_imag - temp_imag -> v6
                vsetvl zero, x2, x3 # set vector length to 2*VL and data type to int32
                addi x12, zero, 0  # initialize j
                slli x14, x11, 1 # initialize 2*(i)
                add x4, x14, x10 # initialize 2*(i+halfstep)

                // SCALAR DATA MOVEMENT
                PUT_DATA_BACK:
                    xvmv.x.s x13, v7, x12 # a_real[j] + temp_real[j] -> x13
                    xvmv.s.x v0, x13, x14 
                    C.ADDI x14, 1 # 2*(i+j) 
                    xvmv.x.s x13, v8, x12 # a_imag[j] + temp_imag[j] -> x13
                    xvmv.s.x v0, x13, x14 
                    C.ADDI x14, 1 # 2*(i+j) + 1
                    xvmv.x.s x13, v5, x12 # a_real[j] - temp_real[j] -> x13
                    xvmv.s.x v0, x13, x4
                    C.ADDI x4, 1  # 2*(i+j+halfstep)
                    xvmv.x.s x13, v6, x12 # a_imag[j] - temp_imag[j] -> x13
                    xvmv.s.x v0, x13, x4
                    C.ADDI x4, 1  # 2*(i+j+halfstep)+1
                    C.ADDI x12, 1  # increment j
                    blt x12, x7, PUT_DATA_BACK  # loop over j 

                srai x6, x2, 1
                C.ADD x11, x10  # increment i by step
                blt x11, x6, STEP  # loop over i
          
            C.ADDI x5, 1  # increment stage counter
            add x15, x15, x7  # increment twiddle index by twiddle step
            blt x5, x8, STAGE  # loop over stages

        xvmv.v.v x9 # copy v0 back to its register
        lw x4, CTL_REG_ARG0_REG_ADDR(zero) # number of channels
        C.ADDI x9, 1  # increment channel counter
        blt x9, x4, CHANNEL  # loop over channels

  ret 
