# File: carus_batchnorm.S
# Author: Francesco Poluzzi
# Date: 28/11/2024
# Description: NM-Carus kernel for batch normalization

# This kernel computes the batch normalization of multiple vectors with per-element weights and biases
# A = [(A - mean) * (1/σ) * gamma] + beta
# The calling software shall provide the following arguments:
# - VL: length of the vectors
# - ARG0: number of vectors (max 28)
# All means and std deviation inverse for each vector are stored respectively in V28 and V29
# The gamma (weight) vector has to be stored in V30 and the beta (bias) vector in V31
# Note: this kernel won't work if the number of vectors is grater than the vector length
# Note: this kernel works with int16_t values, but they need to be casted to int32_t and
#       and the data type argument has to be set to int32_t (this is need to avoid 
#       overflow in the intermediate results)

# NM-Carus address map
#include "carus_addr_map.h"

#define MEAN_VECTOR 28
#define VAR_VECTOR 29
#define GAMMA_VECTOR 30
#define BETA_VECTOR 31
#define NUM_DECIMAL_BITS 12

.section .text
.balign 4
.global _start

_start:

    lw x5, CTL_REG_VL_REG_ADDR(zero) # get vl value
    lw x6, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    vsetvl x5, x5, x6 # set vector length to vl
    lw x2, CTL_REG_ARG0_REG_ADDR(zero) # load number of vectors

    addi x4, zero, 0 // initilize vector counter
    li x3, GAMMA_VECTOR<<16 // initialize vector pointer for gamma 
    li x10, BETA_VECTOR<<16 // initialize vector pointer for beta
    VECTOR: 
        xvmv.x.s x8, v28, x4 # load mean
        xvmv.x.s x9, v29, x4 # load std deviation inverse
        xvsub.vx x3, x8  # A - mean
        xvmul.vx x3, x9  # (A - mean) * (1/σ) 
        xvsra.vi x3, NUM_DECIMAL_BITS
        xvmul.vv x3  # (A - mean) * (1/σ) * gamma
        xvsra.vi x3, NUM_DECIMAL_BITS
        xvadd.vv x10  # (A - mean) * (1/σ) * gamma + beta
        addi x4, x4, 1 # increment vector counter
        addi x3, x3, (1<<8) | 1 # increment vector pointer for gamma
        addi x10, x10, (1<<8) | 1 # increment vector pointer for beta
        blt x4, x2, VECTOR # loop over all vectors

    ret
