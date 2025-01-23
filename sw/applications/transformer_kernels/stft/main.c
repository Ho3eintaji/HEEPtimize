#include <stdio.h>
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
#define NUM_CHANNELS 20 //20
#define NUM_TIME_STEPS 15

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

    #ifdef PRINT
        time = timer_stop();
        PRINTF("STFT time: %d\n", time);
    #endif

    return 0;
}

void initialize_stft(fft_complex_t *data, const quant_bit_width *raw_input_signal) {
    for (int i = 0; i < HANNING_SIZE; i++) {
        data[i].r = (MUL_HQ(raw_input_signal[i], hanning[i]));
        data[i].i = 0;
    }
    for (int i = HANNING_SIZE; i < FFT_SIZE; i++) {
        data[i].r = 0;
        data[i].i = 0;
    }
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

void stft_rearrange(quant_bit_width *rawInputSignal, quant_bit_width *stftVec, size_t patchHeight, size_t patchWidth) {
    fft_complex_t *data = (fft_complex_t*) &ram_buffer[0];
    int overlap = OVERLAP;
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        for (int time_step = 0; time_step < NUM_TIME_STEPS; time_step++) {
            quant_bit_width *rawSignalPtr = rawInputSignal + ch * 3072 + (HANNING_SIZE - overlap) * time_step;
            initialize_stft(data, rawSignalPtr);
            fft_fft(data, 9);
            quant_bit_width *stftVecPtr = stftVec + ch * NUM_TIME_STEPS * PATCH_HEIGHT * PATCH_WIDTH + (time_step / patchWidth) * patchWidth * patchHeight + (time_step % patchWidth);
            for (int index = 0; index < patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            stftVecPtr += patchHeight * patchWidth * 2;
            for (int index = patchHeight; index < 2 * patchHeight; index++) {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
        }
    }
}