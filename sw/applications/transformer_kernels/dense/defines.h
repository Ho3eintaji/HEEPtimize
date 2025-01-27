#ifndef DEFINES_H_
#define DEFINES_H_

#include "x-heep.h"
#include <stdio.h>

// #define PRINT_INTERMEDIATE_CYCLES  // to use this comment out the PRINT_TOTAL_CYCLES
#define PRINT_TOTAL_CYCLES // to use this comment out the PRINT_INTERMEDIATE_CYCLES
// #define PRINT_MATRICES_SIZES
// #define DEBUG_PRINTS
#define PRINT_RESULTS // does not interfere with cycle count

/* By default, PRINTFs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA  1
#define PRINTF_IN_SIM   1
#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif TARGET_IS_FPGA && PRINTF_IN_FPGA
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

#define CEIL_INT_DIV(a, b) (((a) + (b) - 1) / (b))

// comment out to use the standard self attention with a single carus instance
// #define USE_2_CARUS_INSTANCES 

// carus instance to use for the single carus instance version
#define SINGLE_CARUS_INSTANCE 0

#define N_INPUT_CHANNELS 20 // number of input 1d channels for FFT

#endif /* DEFINES_H_ */