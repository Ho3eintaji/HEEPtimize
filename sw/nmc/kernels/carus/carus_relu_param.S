# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: carus_relu.S
# Author: Michele Caon
# Date: 28/10/2023
# Description: NM-Carus kernel for ReLU and Leaky ReLU

# This kernel computed the rectified linear unit (ReLU) and its leaky version
# (Leaky ReLU) on NM-Carus:
#       shamt = 0:   R = max(0, A)
#       shamt > 0:   R = max(A >>> shamt, A)
# where:
# - A is an input matrix
# - R is the output matrix
# - d is an integer number by which the input is divided to implement the leaky
#   ReLU
# The calling software shall provide the following arguments:
# - ARG0: number of rows of the input matrix A
# - VL: number of columns of the input matrix A
# - ARG1: shift amount (=0 for ReLU, >0 for Leaky ReLU)
# - ARG2: R | 00 | A 


# NM-Carus address map
#include "carus_addr_map.h"

.section .text
.balign 4
.global _start

_start:
    # Configure vector type and length
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from control registers
    vsetvl t0, t0, t1 # set VL (cols(A)) and SEW
    sw t0, CTL_REG_VL_REG_ADDR(zero) # write HW-returned VL to control registers

    # Load kernel parameters
    lw a0, CTL_REG_ARG0_REG_ADDR(zero) # load rows(A) from control registers
    li t0, 0 # initialize index of rows of A
    lw a1, CTL_REG_ARG1_REG_ADDR(zero) # load shamt from control registers
    lw t1, CTL_REG_ARG2_REG_ADDR(zero) # load vreg addresses from control registers

    # If leaky ReLU is requested, jump to its address
    bnez a1, leaky_relu

relu: # iterate over rows of A and compute y = max(0, x) for every element
    xvmax.vx t1, zero # vd[i] = max(0, vs2[i])
    addi t0, t0, 1 # increment index of rows of A
    addi t1, t1, 0x000101 # increment pointer to source and destination vectors
    blt t0, a0, relu # repeat if index of rows of A < rows(A)
    ret # return when done

leaky_relu: # divide rows of A by 2**shamt, then compute y = max(x >> shamt, y)
    li t2, 0x010101 # vector registers increment
L0:
    xvsra.vx t1, a1 # vd[i] = vs2[i] >> shamt
    xvmax.vv t1 # vd[i] = max(vs1[i], vs2[i])
    addi t0, t0, 1 # increment index of rows of A
    add t1, t1, t2 # increment pointer to source and destination vectors
    blt t0, a0, L0 # repeat if index of rows of A < rows(A)
    ret # return when done
