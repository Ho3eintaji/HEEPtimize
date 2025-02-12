#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>      // added for strcmp
#include "timer_sdk.h"   // added for timing
#include "data.h"        // Generated header including A, W, B and macros like Q, A_ROWS, A_COLS

// Assuming macros and types (e.g. int16_t) are defined in data.h
// Reuse fixed-point multiplication macros
#define MUL(x, y) ((int32_t)(((int32_t)(x) * (int32_t)(y)) >> Q))
#define MUL_HQ(x, y) ((int32_t)(((int32_t)(x) * (int32_t)(y))))
#define SHIFT(x) ((x) >> Q)

data_t R_cpu[A_SIZE] __attribute__((section(".xheep_data_interleaved"))); // Result computed by the CPU

// Fixed-point normalization function
void normalize_fixed(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *output) {
    for (int i = 0; i < seq_len; i++) {
        data_t *in_ptr = input + i * input_dim;
        data_t *out_ptr = output + i * input_dim;
        int sum = 0;
        for (int j = 0; j < input_dim; j++) {
            sum += in_ptr[j];
        }
        data_t mean = (int16_t)((float)sum / input_dim);
        data_t_double variance = 0;
        for (int j = 0; j < input_dim; j++) {
            variance += MUL_HQ((in_ptr[j] - mean), (in_ptr[j] - mean));
        }
        variance = SHIFT(variance);
        float variance_float = (float)variance / input_dim;
        variance_float /= (1 << Q);
        float sd = sqrtf(variance_float);
        float sd_inv = 1.0f / (sd + 0.00001f);
        data_t sd_inv_int = (int16_t)(sd_inv * (1 << Q));
        for (int j = 0; j < input_dim; j++) {
            data_t norm = (data_t)MUL((in_ptr[j] - mean), sd_inv_int);
            out_ptr[j] = (data_t)(MUL(norm, W[j]) + B[j]);
        }
    }
}

// // Integer normalization function using pure integer arithmetic (no fixed point scaling)
// void normalize_int(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *output) {
//     for (int i = 0; i < seq_len; i++) {
//         int16_t *in_ptr = input + i * input_dim;
//         int16_t *out_ptr = output + i * input_dim;
//         int sum = 0;
//         for (int j = 0; j < input_dim; j++)
//             sum += in_ptr[j];
//         int16_t mean = sum / input_dim;
//         int variance = 0;
//         for (int j = 0; j < input_dim; j++)
//             variance += (in_ptr[j] - mean) * (in_ptr[j] - mean);
//         variance /= input_dim;
//         float sd = sqrtf((float)variance);
//         // Avoid divide by zero
//         float sd_inv = sd ? 1.0f / sd : 1.0f;
//         // Normalize and then apply scaling and bias with integer arithmetic
//         for (int j = 0; j < input_dim; j++) {
//             float normalized = ((float)in_ptr[j] - mean) * sd_inv;
//             // Here W and B are assumed to be in same scale as in data.h
//             out_ptr[j] = (int16_t)(normalized * W[j] + B[j]);
//         }
//     }
// }

// Helper: integer square root (using a simple iterative method)
static inline data_t isqrt(data_t_double num) {
    data_t_double res = 0;
    data_t_double bit = 1L << 30;
    while (bit > num)
        bit >>= 2;
    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (data_t)res;
}

// Efficient integer normalization using fixed-point arithmetic (no float in inner loop)
void normalize_int(data_t seq_len, data_t input_dim, data_t *input, data_t *output) {
    for (int i = 0; i < seq_len; i++) {
        data_t *in_ptr = input + i * input_dim;
        data_t *out_ptr = output + i * input_dim;
        int sum = 0;
        for (int j = 0; j < input_dim; j++) {
            sum += in_ptr[j];
        }
        data_t mean = sum / input_dim;
        int variance = 0;
        for (int j = 0; j < input_dim; j++) {
            data_t diff = in_ptr[j] - mean;
            variance += diff * diff;
        }
        variance /= input_dim;
        data_t sd = isqrt(variance);  // integer sqrt approximation
        data_t sd_inv_int = sd ? ((1 << Q) / sd) : (1 << Q);
        for (int j = 0; j < input_dim; j++) {
            data_t norm = MUL((in_ptr[j] - mean), sd_inv_int);
            out_ptr[j] = (data_t)(MUL(norm, W[j]) + B[j]);
        }
    }
}

int main(void) {
    
    // Start timer
    timer_cycles_init();
    timer_start();
    
    // Run the appropriate normalization based on OPERATION (set in data.h)
    if (strcmp(OPERATION, "fxp") == 0) {
        normalize_fixed(A_ROWS, A_COLS, A, R_cpu);
    } else {
        normalize_int(A_ROWS, A_COLS, A, R_cpu);
    }
    
    // Stop timer and get elapsed cycles
    uint32_t cycles = timer_stop();
    printf("Norm %s: %u\n", OPERATION, cycles);
    
    // // Print results for verification
    // for (unsigned int i = 0; i < A_ROWS; i++) {
    //     for (unsigned int j = 0; j < A_COLS; j++) {
    //         printf("%d ", result[i * A_COLS + j]);
    //     }
    //     printf("\n");
    // }
    return 0;
}


