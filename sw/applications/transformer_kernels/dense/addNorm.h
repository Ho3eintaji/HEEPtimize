//
// Created by alireza on 10/5/23.
// NMC version by Francesco Poluzzi
//

#ifndef FVLLMONTITRANSFORMER_ADDNORMC_H
#define FVLLMONTITRANSFORMER_ADDNORMC_H


#include <stdint.h>
// #include <stdlib.h>
#include "math.h"
#include "param.h"
#include "dma_carus_transfers.h"
// #include "defines_transformer_nmc.h"
#include "defines.h"

#ifdef USE_2_CARUS_INSTANCES
#define NORMALIZE normalize_carus
#else
#define NORMALIZE normalize
#endif

typedef struct {
    int seq_len_;
    int input_dim_;
    quant_bit_width *weight_;
    quant_bit_width *bias_;
} AddNormalize;

AddNormalize createAddNormalize(int seq_len, int input_dim, quant_bit_width *weight, quant_bit_width *bias);

/**
 * @brief Performs batch normalization on the input data using CPU.
 *
 * @param addNorm Pointer to the AddNormalize structure containing normalization parameters.
 * @param input Pointer to the input data array to be normalized.
 * @param input_normalized Pointer to the array where the normalized data will be stored.
 */
void normalize(AddNormalize *addNorm, quant_bit_width *input, quant_bit_width *input_normalized);

/**
 * @brief Performs batch normalization on the input data using 2 carus instances and DMA
 *
 * @param addNorm Pointer to the AddNormalize structure containing normalization parameters.
 * @param input Pointer to the input data array to be normalized.
 * @param input_normalized Pointer to the array where the normalized data will be stored.
 */
void normalize_carus(AddNormalize *addNorm, quant_bit_width *input, quant_bit_width *input_normalized);

void add(quant_bit_width *input, quant_bit_width *to_be_added, int seq_len, int input_dim);

void add_carus(quant_bit_width *input, quant_bit_width *to_be_added, int seq_len, int input_dim);

#endif //FVLLMONTITRANSFORMER_ADDNORMC_H
