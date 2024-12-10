//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include "softmaxC.h"

// softmax scales a matrix into values between 0 and 1 => turns into a probability distribution
void computeSoftmax(int16_t* input, size_t seq_len) {
    size_t width = seq_len;
    float input_float = 0.0f;
    for (int i = 0; i < seq_len; i++) {
        int16_t max_val = input[i * seq_len];
        for (int j = 1; j < width; j++) {
            if (input[i * seq_len + j] > max_val) {
                max_val = input[i * seq_len + j];
            }
        }
        for (int j = 0; j < width; j++) {
            input[i * seq_len + j] =  FIXED_MAX(input[i * seq_len + j] - max_val, -32767);
        }
        int32_t sum = 0;
        for (int j = 0; j < width; j++) {
            input_float = (float) input[i * seq_len + j] / (float) (1 << NUM_FRACTION_BITS);
            input_float = expf(input_float);
            input[i * seq_len + j] = (int16_t) (input_float * (1 << NUM_FRACTION_BITS));
            sum += input[i * seq_len + j];
        }
        float sum_float = (float) sum / (float) (1 << NUM_FRACTION_BITS);
        float sum_inv = (float) (1 / (sum_float + 0.00001)); // prevent zero divide!
        int16_t sum_inv_int = (int16_t) (sum_inv * (1 << NUM_FRACTION_BITS));
        for (int j = 0; j < width; j++) {
            input[i * seq_len + j] = (int16_t) MUL(input[i * seq_len + j], sum_inv_int);
        }
    }
}

// alternative implementation of softmax that works with non-square matrices
void computeSoftmax_nonsquare(int16_t* input, size_t num_rows, size_t num_cols) {
    float input_float = 0.0f;
    for (int i = 0; i < num_rows; i++) {
        int16_t max_val = input[i * num_cols];
        for (int j = 1; j < num_cols; j++) {
            if (input[i * num_cols + j] > max_val) {
                max_val = input[i * num_cols + j];
            }
        }
        for (int j = 0; j < num_cols; j++) {
            input[i * num_cols + j] =  FIXED_MAX(input[i * num_cols + j] - max_val, -32767);
        }
        int32_t sum = 0;
        for (int j = 0; j < num_cols; j++) {
            input_float = (float) input[i * num_cols + j] / (float) (1 << NUM_FRACTION_BITS);
            input_float = expf(input_float);
            input[i * num_cols + j] = (int16_t) (input_float * (1 << NUM_FRACTION_BITS));
            sum += input[i * num_cols + j];
        }
        float sum_float = (float) sum / (float) (1 << NUM_FRACTION_BITS);
        float sum_inv = (float) (1 / (sum_float + 0.00001f)); // prevent zero divide!
        int16_t sum_inv_int = (int16_t) (sum_inv * (1 << NUM_FRACTION_BITS));
        for (int j = 0; j < num_cols; j++) {
            input[i * num_cols + j] = (int16_t) MUL(input[i * num_cols + j], sum_inv_int);
        }
    }
}
