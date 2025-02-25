//
// Created by alireza on 10/6/23.
//

#include "dense_layerC.h"
#include <stdio.h>
#include "defines.h"
#include "gelu.h"

void createDense(Dense *dense, size_t input_dim, size_t output_dim, quant_bit_width *weight, quant_bit_width *bias)
{
    dense->input_size_ = input_dim;
    dense->output_size_ = output_dim;
    dense->weight = weight;
    dense->bias = bias;
}

void destroyDense(Dense *dense)
{
    // Free the memory allocated for the Dense struct
    free(dense);
}

// TODO: modify kernel to do also the fixed point shifts
// matrix multiplication between the input data and the weights of the dense layer
void multiplyweight(Dense *dense, size_t seq_len, int16_t *input, int16_t *output)
{
    #ifdef PRINT_MATRICES_SIZES
        printf("multiplyweight (dense matmul)\n");
        printf("input A: %d X %d\n", seq_len, dense->input_size_);
        printf("input B: %d X %d\n", dense->input_size_, dense->output_size_);
    #endif
    for (int length = 0; length < seq_len; length++)
    {
        for (int out_idx = 0; out_idx < dense->output_size_; out_idx++)
        {
            int16_t *weight_ptr = dense->weight + out_idx;
            int16_t *output_ptr = output + (length * dense->output_size_) + out_idx;
            int16_t *input_ptr = input + (length * dense->input_size_);
            int32_t sum = 0;
            for (int i = 0; i < dense->input_size_; i++)
            {
                sum += MUL_HQ(*weight_ptr, *input_ptr); // MUL_HQ macro
                input_ptr++;
                weight_ptr += dense->output_size_;
            }
            *(output_ptr) = (int16_t)(sum >> NUM_FRACTION_BITS); // NUM_FRACTION_BITS macro
        }
    }
}

void addbias(Dense *dense, size_t seq_len, int16_t *output)
{
    for (size_t idx = 0; idx < seq_len; idx++)
    {
        for (size_t feature_idx = 0; feature_idx < dense->output_size_; feature_idx++)
        {
            output[idx * dense->output_size_ + feature_idx] += dense->bias[feature_idx];
        }
    }
}

// perform the dense layer matrix multiplication and add the bias if it exists
void computeDense(Dense *dense, size_t seq_len, int16_t *input, int16_t *output)
{
    multiplyweight(dense, seq_len, input, output);
    if (dense->bias != NULL) // if the dense layer has a bias, add it to the output
    {
        addbias(dense, seq_len, output);
    }
}

// GELU activation function
void activation(Dense *dense, size_t length, int16_t *input, int16_t *output)
{
    gelu(dense, length, input, output);
}
