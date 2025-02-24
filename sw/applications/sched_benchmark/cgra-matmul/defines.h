#ifndef DEFINES_H_
#define DEFINES_H_

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