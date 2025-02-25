#include "stft_amp.h"
#include <math.h>
#include <stdio.h>
#include "defines.h"
#include "../param.h"
#include <stdint.h>
#include <limits.h>

// last one
#define LUT_SIZE 1024  // Power of 2, larger is more accurate
#define LUT_SHIFT 10   // log2(LUT_SIZE), for indexing
#define SQRT_ITERATIONS 5 // Number of Newton-Raphson iterations
static int32_t log2_lut[LUT_SIZE];
// ---  log(2) for base conversion (Q(NUM_FRACTION_BITS)) ---
static int32_t LOG2_TO_LOG_SCALE; // Will be initialized in init_log2_lut


static const int32_t log_lut[256] = {
    0, 454, 907, 1358, 1808, 2256, 2703, 3148, // ... fill all 256 entries
};



// Function to compute the logarithm of the amplitude
quant_bit_width compute_log_amp(int32_t real, int32_t imag)
{
    #if LOG_AMP_IMPL == LOG_AMP_FP
    compute_log_amp_fp(real, imag);
    #elif LOG_AMP_IMPL == LOG_AMP_FXP_LUT
    compute_log_amp_fxp_lut(real, imag);
    #elif LOG_AMP_IMPL == LOG_AMP_FXP_APPROX
    compute_log_amp_fxp_approx(real, imag);
    #elif LOG_AMP_IMPL == LOG_AMP_FXP
    compute_log_amp_fxp(real, imag);
    #endif
}

// Function to compute the logarithm of the amplitude
quant_bit_width compute_log_amp_fp(int32_t real, int32_t imag)
{
    real = MUL_HQ(real, 25) >> (NUM_FRACTION_BITS - 9);
    imag = MUL_HQ(imag, 25) >> (NUM_FRACTION_BITS - 9);
    int32_t real2 = MUL_LONG(real, real) >> NUM_FRACTION_BITS;
    int32_t imag2 = MUL_LONG(imag, imag) >> NUM_FRACTION_BITS;
    float pow2 = (float)(real2 + imag2) / (float)(1 << NUM_FRACTION_BITS);
    float amp = sqrtf(pow2);
    float stft = logf(amp + 1e-10f);
    quant_bit_width stft_int = (quant_bit_width)(stft * (1 << NUM_FRACTION_BITS));
    return stft_int;
}

// an alternative impl, using fixed-point arithmetic and a lookup table
quant_bit_width compute_log_amp_fxp_lut(int32_t real, int32_t imag) {
    // Scale real and imag
    real = MUL_HQ(real, 25) >> (NUM_FRACTION_BITS - 9);
    imag = MUL_HQ(imag, 25) >> (NUM_FRACTION_BITS - 9);
    
    // Compute squared magnitude
    int32_t real2 = (MUL_LONG(real, real) >> NUM_FRACTION_BITS);
    int32_t imag2 = (MUL_LONG(imag, imag) >> NUM_FRACTION_BITS);
    int32_t sqmag = real2 + imag2;

    // Handle near-zero magnitude to avoid log(0)
    if (sqmag == 0) {
        return (quant_bit_width)(logf(1e-10f) * (1 << NUM_FRACTION_BITS));
    }

    // Fixed-point square root of sqmag (Q30.16 -> Q15.16)
    uint32_t sqrt_val = 0;
    uint32_t bit = 1UL << 30; // Start with the highest possible bit
    while (bit > sqmag) bit >>= 2;
    while (bit != 0) {
        if (sqmag >= sqrt_val + bit) {
            sqmag -= sqrt_val + bit;
            sqrt_val = (sqrt_val >> 1) + bit;
        } else {
            sqrt_val >>= 1;
        }
        bit >>= 2;
    }
    uint32_t magnitude = sqrt_val;

    // Normalize magnitude to [1, 2) range and compute log
    int clz = __builtin_clz(magnitude);
    int exponent = 31 - clz;
    uint32_t mantissa = (magnitude << clz) >> (24 - exponent); // Adjust for 8-bit LUT index
    uint8_t lut_index = (mantissa >> 16) & 0xFF; // Use upper 8 bits

    // Calculate log(amp) using LUT and exponent
    int32_t log_amp = (exponent - 16) * 45426; // 45426 ≈ log(2) in Q15.16
    log_amp += log_lut[lut_index];

    return (quant_bit_width)log_amp;
}

// an alternative impl, using fixed-point arithmetic and approximations (Newton-Raphson for square root, bit manipulation for logarithm)
quant_bit_width compute_log_amp_fxp_approx(int32_t real, int32_t imag) {
    real = MUL_HQ(real, 25) >> (NUM_FRACTION_BITS - 9);
    imag = MUL_HQ(imag, 25) >> (NUM_FRACTION_BITS - 9);
    int32_t real2 = MUL_LONG(real, real) >> NUM_FRACTION_BITS;
    int32_t imag2 = MUL_LONG(imag, imag) >> NUM_FRACTION_BITS;
    int32_t sum_sq = (real2 + imag2) >> NUM_FRACTION_BITS;

    //  Approximation for sqrt and log.
    if (sum_sq == 0) return 0; // Handle zero case

    // 1. Fast Square Root Approximation (Newton-Raphson single iteration)
    int32_t approx_sqrt = (sum_sq + (1 << NUM_FRACTION_BITS)) >> 1; // Initial guess: average
    approx_sqrt = (approx_sqrt + (sum_sq / approx_sqrt)) >> 1;       // One Newton-Raphson iteration
    //Could improve this with another iteration or a lookup table for refinement

    // 2. Fast Log2 Approximation (using bit manipulation)
    //    This part assumes NUM_FRACTION_BITS is set for your fixed-point representation.
    int leading_zeros = __builtin_clz(approx_sqrt);
    int exponent = (31 - leading_zeros) - NUM_FRACTION_BITS; // Integer part
    int mantissa = (approx_sqrt << leading_zeros) & 0x7FFFFFFF;   // Fractional part

    // You can further refine mantissa using a small lookup table if higher precision is needed.
    // For a very rough approximation, we can use the top bits of the mantissa directly.

    // Combine exponent and mantissa (adjust scaling as needed)
    quant_bit_width log_approx = (exponent << NUM_FRACTION_BITS) + (mantissa >> (31 - NUM_FRACTION_BITS));

    return log_approx;
}