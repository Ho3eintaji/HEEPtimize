
#ifndef _DEFINES_H_
#define _DEFINES_H_

/* --------------------------------------------------------------------------
 *                     Application defines
 * --------------------------------------------------------------------------*/

#define FFT_SIZE (512)

// Choose complex or real values FFT
#define CPLX_FFT
// #define REAL_FFT --> DEPRECATED (KERNEL NEEDS TO BE ADAPTED TO CURRENT CGRA)

// Define this to use completely the CGRA (2 FFTS running in parallel)
// #define CGRA_100_PERCENT
// Define this to run the FFT kernel forever for power measurement
// #define CGRA_FFT_FOREVER

// Check for error and print results
#define CHECK_ERRORS

// Define this to print more information
#define DEBUG

// PRINT_TIME prints the time of the execution
#define PRINT_TIME

// Define this to use the CPU FFT implementation
#define CPU_FFT

// Use PRINTF instead of PRINTF to remove print by default
#ifdef DEBUG
  #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
  #define PRINTF(...)
#endif


#endif // _DEFINES_H_
