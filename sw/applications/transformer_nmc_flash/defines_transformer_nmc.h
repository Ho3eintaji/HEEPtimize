// Author: Francesco Poluzzi

#ifndef DEFINES_H_
#define DEFINES_H_

#include "x-heep.h"
#include <stdio.h>

// #define PRINT_INTERMEDIATE_CYCLES  // to use this comment out the PRINT_TOTAL_CYCLES
// #define PRINT_TOTAL_CYCLES // to use this comment out the PRINT_INTERMEDIATE_CYCLES
// #define PRINT_MATRICES_SIZES // prints the sizes of the matrices in matrix multiplications
// #define DEBUG_PRINTS // enables extensive prints used for debug purposes
#define PRINT_RESULTS // does not interfere with cycle count

/* By default, PRINTFs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA  1
#define PRINTF_IN_SIM   0

#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif TARGET_IS_FPGA && PRINTF_IN_FPGA
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

#define USE_DOUBLE_HEAD_SELF_ATTN 

#endif /* DEFINES_H_ */