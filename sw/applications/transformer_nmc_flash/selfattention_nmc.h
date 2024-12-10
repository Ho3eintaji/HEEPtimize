//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#ifndef FVLLMONTITRANSFORMER_SELFATTENTIONC_H
#define FVLLMONTITRANSFORMER_SELFATTENTIONC_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "dense_layerC.h"
#include "softmaxC.h"
#include "transposeC.h"
#include "matMulC.h"
#include "../param.h"

typedef struct {
    Dense* query_layer;
    Dense* key_layer;
    Dense* value_layer;
    int16_t* query_layer_out;
    int16_t* key_layer_out;
    int16_t* key_transposed_layer_out;
    int16_t* value_layer_out;
    int16_t* attention_scores;
    size_t pre_seq_len;
    size_t head_hidden_size;
} SingleHeadSelfAttn;

void create_SingleHeadSelfAttn(SingleHeadSelfAttn*, size_t pre_seq_len, size_t input_dim, size_t head_hidden_size, int16_t** weightVector);
void destroy_SingleHeadSelfAttn(SingleHeadSelfAttn* self_attn);
void compute_SingleHeadSelfAttn(SingleHeadSelfAttn* self_attn, int16_t* input, int16_t* output, int16_t* qkv, int16_t* intermediate);
void compute_DoubleHeadSelfAttn(SingleHeadSelfAttn* self_attn_1, SingleHeadSelfAttn* self_attn_2, int16_t* input, int16_t* output_1, int16_t* output_2, int16_t* qkv, int16_t* qkv_2, int16_t* intermediate_1);
#endif //FVLLMONTITRANSFORMER_SELFATTENTIONC_H
