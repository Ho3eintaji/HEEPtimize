#ifndef DEFINES_H_
#define DEFINES_H_

// #define PRINT_INTERMEDIATE_CYCLES  // to use this comment out the PRINT_TOTAL_CYCLES
#define PRINT_TOTAL_CYCLES // to use this comment out the PRINT_INTERMEDIATE_CYCLES
// #define PRINT_MATRICES_SIZES
// #define DEBUG_PRINTS
#define PRINT_RESULTS // does not interfere with cycle count

#define PRINT_KERNELS_CYCLES
extern uint32_t t_tmp;
extern uint32_t t_logamp;
extern uint32_t t_gelu;
extern uint32_t t_softmax;
extern uint32_t t_hanning;
extern uint32_t t_fft;
extern uint32_t t_norm;
extern uint32_t t_matmul_add;
extern uint32_t t_matmul;
extern uint32_t t_add;
extern uint32_t t_clsconcat;
extern uint32_t t_transpose;
extern uint32_t t_mh_transpose;
extern uint32_t t_mm_scale; 
extern uint32_t t_euc_dist;

// Softmax implementation
#define SM_FP 0
#define SM_SOFTERMAX 1
#define SM_FIXED 2
#define SM_ConSmax 3
#define SM_IMPL SM_ConSmax

// GeLU implementation (two versions: FP and PWL)
#define GELU_FP 0
#define GELU_PWL 1
#define GELU_IMPL GELU_PWL

// Logarithm of amplitude implementation
#define LOG_AMP_FP 0
#define LOG_AMP_FXP_LUT 1
#define LOG_AMP_FXP_APPROX 2
#define MAGNITUDE_FXP 3
#define MAGNITUDE_FXP_OPT 4
#define LOG_AMP_IMPL MAGNITUDE_FXP_OPT

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