#ifndef DEFINES_H_
#define DEFINES_H_

// #define PRINT_INTERMEDIATE_CYCLES  // to use this comment out the PRINT_TOTAL_CYCLES
#define PRINT_TOTAL_CYCLES // to use this comment out the PRINT_INTERMEDIATE_CYCLES
// #define PRINT_MATRICES_SIZES
// #define DEBUG_PRINTS
#define PRINT_RESULTS // does not interfere with cycle count

// Softmax implementation
#define SM_FP 0
#define SM_SOFTERMAX 1
#define SM_FIXED 2
#define SM_IMPL SM_FIXED

// GeLU implementation (two versions: FP and PWL)
#define GELU_FP 0
#define GELU_PWL 1
#define GELU_IMPL GELU_PWL

// Logarithm of amplitude implementation
#define LOG_AMP_FP 0
#define LOG_AMP_FXP_LUT 1
#define LOG_AMP_FXP_APPROX 2
#define LOG_AMP_IMPL LOG_AMP_FXP_APPROX

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

#endif /* DEFINES_H_ */