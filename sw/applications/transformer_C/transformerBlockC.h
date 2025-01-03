//
// Created by alireza on 10/6/23.
//

#ifndef FVLLMONTITRANSFORMER_TRANSFORMERBLOCK_H
#define FVLLMONTITRANSFORMER_TRANSFORMERBLOCK_H

#include <stddef.h>
#include <stdint.h>
#include "selfattentionC.h"
#include "addNormC.h"
#include "dense_layerC.h"
#include "tokenPosEmbeddingC.h"
#include "../param.h"
#include "transposeC.h"

typedef struct {
    // number of attention heads in the multi-head attention mechanism
    // Each head performs self-attention independently, and their results are combined.
    size_t num_heads_;

    // Each attention head computes a query, key, and value projection with dimensions defined by this size
    // total attention size = num_heads_ * head_hidden_size_
    size_t head_hidden_size_; 

    // dimensionality of the input sequence or embedding
    size_t input_dim_; 

    // size of the hidden layer in the feed-forward network (FFN)
    size_t ff_size_;

    // array of pointers to SingleHeadSelfAttn structures, where each element represents a single head in the multi-head attention mechanism
    SingleHeadSelfAttn* selfatten[NUM_LAYERS*NUM_HEAD];

    // pointer to the output of the multi-head attention mechanism
    int16_t* multihead_out;

    // Likely stores the output after the condense operation, which might refer to a projection or reduction of dimensions applied after
    // multi-head attention and before the feed-forward network.
    int16_t* condense_out;

    // pointer to the intermediate output from the first layer of the feed-forward network (FFN) in the transformer
    int16_t* intermediateFF;
     // same but to store the intermediate output in a block-wise manner
    int16_t* intermediateFFBlockWise;

    // 2 instances of the AddNormalize structure, which which likely encapsulates the Add & Layer Normalization (done for each sub-layer)
    AddNormalize addNorm;
    AddNormalize addNorm2;

    // array of AddNormalize objects, one for each transformer layer
    AddNormalize transformer_layer_0_addNorm[NUM_LAYERS];
    AddNormalize transformer_layer_1_addNorm[NUM_LAYERS];

    //  layer normalization applied after the final MLP head at the end of the transformer 
    AddNormalize mlp_head_norm;

    // Pointer to the token position embedding layer
    // (combination of token embeddings and positional embeddings)
    TokenPosEmbedding* token;

    // array of Dense layers, one for each transformer layer
    // A dense layer (also known as a fully connected layer) would be used to transform the output of multi-head attention or feed-forward networks.
    Dense* condense[NUM_LAYERS];

    // These arrays holds the first fully connected layer (dense layer) in the feed-forward network (FFN) for each transformer layer
    Dense* feedForward0[NUM_LAYERS];
    Dense* feedForward1[NUM_LAYERS];

    // pointer to the patch embedding layer
    // the patch embedding layer converts 2D patches of an image into a sequence of vectors that the transformer can process
    Dense* patchEmbedding;

    //  pointer to the final MLP head linear layer, which is typically used for tasks like classification
    Dense* mlp_head_linear;
    #ifndef REARRANGE
    int16_t* multihead_out_reshape;
    #endif
} TransformerBlock;

TransformerBlock* createTransformerBlock(size_t pre_seq_len, size_t input_dim, size_t head_hidden_size, size_t num_heads, size_t ff_size, int16_t** weightVector, int16_t** biasVector, int16_t* clsTokenVector, int16_t* posMatrix);
void destroyTransformerBlock(TransformerBlock* transformerBlock);
void computeFixedPoint(TransformerBlock *transformerBlock, size_t seq_len, quant_bit_width *input,
                       quant_bit_width *input_normalized, quant_bit_width *output,
                       quant_bit_width *intermediate, quant_bit_width *qkv, quant_bit_width* ram_buffer);
#endif //FVLLMONTITRANSFORMER_TRANSFORMERBLOCK_H
