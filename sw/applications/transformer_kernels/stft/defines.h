#ifndef DEFINES_H_
#define DEFINES_H_

#define PRINT_INTERMEDIATE_CYCLES  // to use this comment out the PRINT_TOTAL_CYCLES
// #define PRINT_TOTAL_CYCLES // to use this comment out the PRINT_INTERMEDIATE_CYCLES
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

#endif /* DEFINES_H_ */