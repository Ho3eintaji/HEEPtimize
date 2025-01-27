//
// Created by alireza on 10/5/23.
// NMC version by Francesco Poluzzi
//

#ifndef FVLLMONTITRANSFORMER_DENSE_LAYERC_H
#define FVLLMONTITRANSFORMER_DENSE_LAYERC_H


#include <stdlib.h>
// #include <stddef.h>
#include <math.h>
#include "param.h"
#include "defines.h"
#include "carus.h"

#define MAX_A_ROWS_TILE_SIZE MAX_A_ROWS
#define MAX_A_COLS_TILE_SIZE MAX_A_COLS

// Define the struct
typedef struct {
    size_t input_size_;
    size_t output_size_;
    int16_t* weight; // quant_bit_width is a typedef for int16_t
    int16_t* bias; // quant_bit_width is a typedef for int16_t
} Dense;

void createDense(Dense* dense, size_t input_dim, size_t output_dim, quant_bit_width *weight, quant_bit_width* bias);

void destroyDense(Dense* dense);

/**
 * @brief Perform matrix multiplication using CPU
 *
*/
void multiplyweight(Dense* dense, size_t seq_len, int16_t* input, int16_t* output);

/**
 * @brief Perform matrix multiplication using Carus and DMA.
 *
 * Maximux sizes for calling this function are:
 * - seq_len = carus vector length (2048 Bytes)
 * - dense->input_size_ = 16
 * - dense->output_size_ can be anything, as we tile in this sense
*/
void multiplyweight_carus_regular_tiling(Dense *dense, size_t seq_len, int16_t *input, int16_t *output);

void addbias(Dense* dense, size_t seq_len, int16_t* output);

/**
 * @brief Perform dense layer operation using CPU.
 *
 * This function computes the dense layer operation on the given input
 * and stores the result in the output. The dense layer is defined by
 * the provided Dense structure.
 *
 * @param dense Pointer to the Dense structure defining the layer.
 * @param seq_len Length of the input sequence.
 * @param input Pointer to the input data.
 * @param output Pointer to the output data.
 */
void computeDense(Dense* dense, size_t seq_len, int16_t* input, int16_t* output);

/**
 * @brief Perform dense layer operation using either Carus and DMA or CPU, depending on the input and layer sizes.
 *
 * This function computes the dense layer operation on the given input
 * using Carus and DMA, and stores the result in the output. The dense
 * layer is defined by the provided Dense structure.
 *
 * @param dense Pointer to the Dense structure defining the layer.
 * @param seq_len Length of the input sequence.
 * @param input Pointer to the input data.
 * @param output Pointer to the output data.
 */
void computeDense_carus(Dense *dense, size_t seq_len, int16_t *input, int16_t *output);

void activation(Dense* dense, size_t length, int16_t* input, int16_t* output);

#endif //FVLLMONTITRANSFORMER_DENSE_LAYERC_H
