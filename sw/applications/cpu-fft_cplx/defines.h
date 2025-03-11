
#ifndef _DEFINES_H_
#define _DEFINES_H_

/* --------------------------------------------------------------------------
 *                     Application defines
 * --------------------------------------------------------------------------*/

// Possible FFT size range for complex-valued input: 128, 256, 512, 1024, 2048
// Possible FFT size range for real-valued input: 256, 512, 1024, 2048
// Possible value for this application: 512, 1024, 2048 (data for the other sizes have to be generated and put inside data.h)
// Smaller FFT is possible but mapping as to be adapted because the input does not fill a complete VWR
// Larger  FFT is possible but mapping as to be adapted because scratchpad memory cannot hold all inputs at the same time

#define FFT_SIZE (512)

// Choose complex or real values FFT
#define CPLX_FFT
// #define REAL_FFT

#define CHECK_ERRORS

#define DEBUG

// Use PRINTF instead of PRINTF to remove print by default
#ifdef DEBUG
  #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
  #define PRINTF(...)
#endif

#endif // _DEFINES_H_
