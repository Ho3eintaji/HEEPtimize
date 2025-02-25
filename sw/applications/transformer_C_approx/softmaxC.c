//
// Created by alireza on 10/6/23.
// Approx version by Hossein Taji and Francesco Poluzzi on 25/2/25.
//

#include "softmaxC.h"
#include "defines.h"

#define N_TAYLOR_COEFF 6
int32_t factorials[N_TAYLOR_COEFF-1] = {1<<NUM_FRACTION_BITS, 2<<NUM_FRACTION_BITS, 6<<NUM_FRACTION_BITS, 24<<NUM_FRACTION_BITS, 120<<NUM_FRACTION_BITS};

#define FP_ONE (1 << NUM_FRACTION_BITS)
#define EPSILON 1



void computeSoftmax(int16_t* input, size_t seq_len) {
    #if SM_IMPL == SM_FP
    computeSoftmax_fp(input, seq_len);
    #elif SM_IMPL == SM_SOFTERMAX
    softermax(input, seq_len, seq_len);
    #elif SM_IMPL == SM_FIXED
    computeSoftmax_nonsquare_fixed(input, seq_len, seq_len);
    #endif
}

// softmax scales a matrix into values between 0 and 1 => turns into a probability distribution
// consider for square matrices
void computeSoftmax_fp(int16_t* input, size_t seq_len) {
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
            input[i * seq_len + j] = FIXED_MAX(input[i * seq_len + j] - max_val, -32767);
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

int32_t fixed_div(int32_t numerator, int32_t denominator) {
    int64_t scaled_numerator = (int64_t)numerator << NUM_FRACTION_BITS;
    int32_t result = (int32_t)(scaled_numerator / denominator);
    return result;
}

int32_t exp_fixed_point_taylor(int32_t input){
    int32_t result = (1 << NUM_FRACTION_BITS); ;
    int32_t x_pow_n = input;
    for (int i = 0; i < N_TAYLOR_COEFF-1; i++) {
        result += fixed_div(x_pow_n, factorials[i]);
        x_pow_n = MUL(x_pow_n, input);
    }
    return result;
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
            input[i * num_cols + j] = (int16_t) FIXED_MAX(input[i * num_cols + j] - max_val, -32767);
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


void computeSoftmax_nonsquare_fixed(int16_t* input, size_t num_rows, size_t num_cols) {
    for (size_t i = 0; i < num_rows; i++) {
        int16_t max_val = input[i * num_cols];
        for (size_t j = 1; j < num_cols; j++) {
            int16_t val = input[i * num_cols + j];
            if (val > max_val) {
                max_val = val;
            }
        }
        for (size_t j = 0; j < num_cols; j++) {
            int32_t diff = (int32_t)input[i * num_cols + j] - (int32_t)max_val;
            if (diff > 32767) diff = 32767;
            if (diff < -32768) diff = -32768;
            input[i * num_cols + j] = (int16_t)diff;
        }
        // Compute exponent and sum
        int32_t sum = 0;
        for (size_t j = 0; j < num_cols; j++) {
            int32_t val = (int32_t)input[i * num_cols + j];
            int32_t exp_val = exp_fixed_point_taylor(val); 
            if (exp_val > 32767) exp_val = 32767;
            if (exp_val < -32768) exp_val = -32768;
            input[i * num_cols + j] = (int16_t)exp_val;
            sum += exp_val; 
        }
        int32_t sum_inv = fixed_div(FP_ONE, sum + EPSILON);
        // Multiply each element by sum_inv to normalize
        for (size_t j = 0; j < num_cols; j++) {
            int32_t val = (int32_t)input[i * num_cols + j];
            int32_t normalized = MUL(val, sum_inv); 
            if (normalized > 32767) normalized = 32767;
            if (normalized < -32768) normalized = -32768;
            input[i * num_cols + j] = (int16_t)normalized;
        }
    }
}

// alternative implementation of softmax that works with non-square matrices
void softermax(int16_t* input, size_t num_rows, size_t num_cols) {
    int16_t max_values[num_cols];
    max_values[0] = -32767;
    for (int i = 0; i < num_rows; i++) {
        int32_t d = 0;
        for (int j = 1; j < num_cols; j++) {
            int16_t current_val = input[i * num_cols + j];
            int16_t max_val= FIXED_MAX(current_val, max_values[j-1]);
            max_values[j] = max_val;
            int32_t exp_term = exp_fixed_point_taylor(current_val - max_val);
            d = (d>>(max_val-max_values[j-1]))+exp_term;
            input[i * num_cols + j] = exp_term;
        }
        int16_t absolute_max = max_values[num_cols-1];
        for (int j = 0; j < num_cols; j++) {
            input[i * num_cols + j] = (int16_t)  fixed_div( (input[i * num_cols + j]>>(absolute_max-max_values[j])), d);
        }
    }
}