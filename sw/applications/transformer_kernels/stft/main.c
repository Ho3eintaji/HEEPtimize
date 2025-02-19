#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "SYLT-FFT/fft.h"
#include "defines.h"
#include "param.h"

#define PRINT

#define PATCH_HEIGHT 80
#define PATCH_WIDTH 5
#define OVERLAP 64
#define NUM_CHANNELS 1 //20
#define NUM_TIME_STEPS 5 //15

#define FFT_SIZE 512 
#define HANNING_SIZE 256

// Sizes in TSD
/*
    quant_bit_width *stftVec = raw_signal;
    quant_bit_width *rawInputSignal = raw_signal + 160 * 15;
    quant_bit_width *intermediate = raw_signal + 16 * 1024;
    quant_bit_width *out = raw_signal + 160 * 15 * 20;
    quant_bit_width *qkv = out + 2048;
    quant_bit_width *input_normalized = out + 4096;

    SO

    stftVec = 160 * 15 
    rawInputSignal = 160 * 15 * 20 - 16 * 1024 = 16 * 1976
    out = 2048
    qkv = 2048
    intermediate = &out - &intermediate = 160 * 15 * 20 - 16 * 1024 = 16 * 1976
    input_normalized = TOTAL - (&out + 4096) = 63840 - (160 * 15 * 20 + 4096) = 11744


*/

quant_bit_width __attribute__((section(".xheep_data_interleaved"))) hanning[HANNING_SIZE] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 32, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 30, 29, 29, 29, 29, 29, 28, 28, 28, 28, 27, 27, 27, 27, 26, 26, 26, 25, 25, 25, 24, 24, 24, 23, 23, 23, 22, 22, 22, 21, 21, 21, 20, 20, 19, 19, 19, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15, 14, 14, 14, 13, 13, 12, 12, 12, 11, 11, 10, 10, 10, 9, 9, 9, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) rawInputSignal[NUM_CHANNELS * 3072] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) stftVec[NUM_CHANNELS * NUM_TIME_STEPS * PATCH_HEIGHT * PATCH_WIDTH] = {0};
fft_complex_t __attribute__((section(".xheep_data_interleaved"))) data[FFT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) ram_buffer[2284] = {0};

void initialize_stft(fft_complex_t *data, const quant_bit_width *raw_input_signal);
quant_bit_width compute_log_amp(int32_t real, int32_t imag);
void stft_rearrange(quant_bit_width *rawInputSignal, quant_bit_width *stftVec, size_t patchHeight, size_t patchWidth);
void accuracy_test(int num_samples);
quant_bit_width compute_log_amp_optimized(int32_t real, int32_t imag);
quant_bit_width compute_log_amp_approx(int32_t real, int32_t imag) ;
void accuracy_test(int num_samples);



int main() {
    // // Initialize rawInputSignal with some values
    // for (int i = 0; i < NUM_CHANNELS * 3072; i++) {
    //     rawInputSignal[i] = i; // Example initialization
    // }

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    stft_rearrange(rawInputSignal, stftVec, PATCH_HEIGHT, PATCH_WIDTH);
    accuracy_test(1000);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("STFT time: %d\n", time);
    #endif

    return 0;
}

void initialize_stft(fft_complex_t *data, const quant_bit_width *raw_input_signal) {
    uint32_t t1 = timer_get_cycles();
    for (int i = 0; i < HANNING_SIZE; i++) {
        data[i].r = (MUL_HQ(raw_input_signal[i], hanning[i]));
        data[i].i = 0;
    }
    uint32_t t_hanning = timer_get_cycles() - t1;
    t1 = timer_get_cycles();
    for (int i = HANNING_SIZE; i < FFT_SIZE; i++) {
        data[i].r = 0;
        data[i].i = 0;
    }
    uint32_t t_zero = timer_get_cycles() - t1;
    PRINTF("Time to apply hanning: %d\n", t_hanning);
    PRINTF("Time to zero pad: %d\n", t_zero);
}

quant_bit_width compute_log_amp(int32_t real, int32_t imag) {
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

static const int32_t log_lut[256] = {
    0, 454, 907, 1358, 1808, 2256, 2703, 3148, // ... fill all 256 entries
};
quant_bit_width compute_log_amp_optimized(int32_t real, int32_t imag) {
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

// Fast Log Approximation using bitwise operations and a lookup table (optional)
quant_bit_width compute_log_amp_approx(int32_t real, int32_t imag) {
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


void stft_rearrange(quant_bit_width *rawInputSignal, quant_bit_width *stftVec, size_t patchHeight, size_t patchWidth) {
    fft_complex_t *data = (fft_complex_t*) &ram_buffer[0];
    uint32_t t1, t2;
    int overlap = OVERLAP;
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        for (int time_step = 0; time_step < NUM_TIME_STEPS; time_step++) {
            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            quant_bit_width *rawSignalPtr = rawInputSignal + ch * 3072 + (HANNING_SIZE - overlap) * time_step;
            t2 = timer_get_cycles();
            PRINTF("Time to calculate rawSignalPtr: %d\n", t2 - t1);
            
            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            initialize_stft(data, rawSignalPtr);
            t2 = timer_get_cycles();
            PRINTF("Time to initialize stft: %d\n", t2 - t1);
            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            fft_fft(data, 9);
            t2 = timer_get_cycles();
            PRINTF("Time to perform FFT: %d\n", t2 - t1);
            t1 = timer_get_cycles();
            quant_bit_width *stftVecPtr = stftVec + ch * NUM_TIME_STEPS * PATCH_HEIGHT * PATCH_WIDTH + (time_step / patchWidth) * patchWidth * patchHeight + (time_step % patchWidth);
            t2 = timer_get_cycles();
            PRINTF("Time to calculate stftVecPtr: %d\n", t2 - t1);
            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            for (int index = 0; index < patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            t2 = timer_get_cycles();
            PRINTF("Time to calculate first half of compute_log_amp original: %lu\n", t2 - t1);

            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            for (int index = 0; index < patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp_approx(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            t2 = timer_get_cycles();
            PRINTF("Time to calculate first half of compute_log_amp approx: %lu\n", t2 - t1);


            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            for (int index = 0; index < patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp_optimized(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            t2 = timer_get_cycles();
            PRINTF("Time to calculate first half of compute_log_amp optimized: %lu\n", t2 - t1);

            timer_cycles_init();
            timer_start();
            t1 = timer_get_cycles();
            stftVecPtr += patchHeight * patchWidth * 2;
            for (int index = patchHeight; index < 2 * patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            t2 = timer_get_cycles();
            PRINTF("Time to calculate second half of compute_log_amp: %d\n", t2 - t1);
        }
    }
}

void accuracy_test(int num_samples)
{
    int64_t sumSqError_approx = 0;    // Sum of squared errors for compute_log_amp_approx
    int64_t sumSqError_opt    = 0;    // Sum of squared errors for compute_log_amp_optimized
    int32_t maxAbsError_approx = 0;
    int32_t maxAbsError_opt    = 0;

    for (int i = 0; i < num_samples; i++) {
        // Generate random test real/imag in a reasonable range
        int32_t real = (int32_t)((rand() & 0xFFFF) - 32768);
        int32_t imag = (int32_t)((rand() & 0xFFFF) - 32768);

        // Compute "reference" value using the original compute_log_amp
        quant_bit_width ref = compute_log_amp(real, imag);

        // Compute approximate versions
        quant_bit_width val_approx = compute_log_amp_approx(real, imag);
        quant_bit_width val_opt    = compute_log_amp_optimized(real, imag);

        // Compute integer error
        int32_t diff_approx = val_approx - ref;
        int32_t diff_opt    = val_opt    - ref;

        // Accumulate sum of squared errors (in integer domain)
        sumSqError_approx += (int64_t)diff_approx * (int64_t)diff_approx;
        sumSqError_opt    += (int64_t)diff_opt * (int64_t)diff_opt;

        // Track max absolute error
        if (abs(diff_approx) > maxAbsError_approx) {
            maxAbsError_approx = abs(diff_approx);
        }
        if (abs(diff_opt) > maxAbsError_opt) {
            maxAbsError_opt = abs(diff_opt);
        }
    }

    // Compute integer-based MSE (without floating-point operations)
    int32_t mse_approx = (int32_t)(sumSqError_approx / num_samples);
    int32_t mse_opt    = (int32_t)(sumSqError_opt / num_samples);

    // Print as integers
    printf("\nAccuracy Test Results (N=%d):\n", num_samples);
    printf("Approx vs. Reference:\n");
    printf("    MSE  = %d\n", mse_approx);
    printf("    MaxAbsErr = %d\n", maxAbsError_approx);

    printf("Optimized vs. Reference:\n");
    printf("    MSE  = %d\n", mse_opt);
    printf("    MaxAbsErr = %d\n", maxAbsError_opt);
}
