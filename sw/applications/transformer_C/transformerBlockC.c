//
// Created by alireza on 10/6/23.
//

#include <stdio.h>
#include "transformerBlockC.h"
#include "timer_sdk.h"
#include "defines.h"

SingleHeadSelfAttn global_selfatten[NUM_LAYERS * NUM_HEAD];
Dense global_query_layer[NUM_LAYERS * NUM_HEAD];
Dense global_key_layer[NUM_LAYERS * NUM_HEAD];
Dense global_value_layer[NUM_LAYERS * NUM_HEAD];

Dense global_condense[NUM_LAYERS];
Dense global_patch;
Dense global_FF[NUM_LAYERS * 2];
Dense global_mlp;

TransformerBlock global_transformer_block;
TokenPosEmbedding global_token_embedding;

// function to create a transformer block struct that contains everything needed for inference
TransformerBlock *createTransformerBlock(size_t pre_seq_len, size_t input_dim, size_t head_hidden_size, size_t num_heads, size_t ff_size, int16_t **weightVector, int16_t **biasVector, int16_t *clsTokenVector, int16_t *posMatrix)
{
    // create the transformer block struct from function inputs
    TransformerBlock *transformerBlock = &global_transformer_block;
    transformerBlock->num_heads_ = num_heads;
    transformerBlock->head_hidden_size_ = head_hidden_size;
    transformerBlock->input_dim_ = input_dim;
    transformerBlock->ff_size_ = ff_size;

    transformerBlock->addNorm = createAddNormalize(pre_seq_len, D_EMBEDDING, weightVector[0], biasVector[0]);
    transformerBlock->patchEmbedding = &global_patch;
    createDense(transformerBlock->patchEmbedding, D_EMBEDDING, D_MODEL, weightVector[1], biasVector[1]);
    transformerBlock->addNorm2 = createAddNormalize(pre_seq_len, D_MODEL, weightVector[2], biasVector[2]);
    transformerBlock->token = &global_token_embedding;
    createTokenPosEmbedding(transformerBlock->token, posMatrix, clsTokenVector, pre_seq_len, input_dim, D_SEQ + 1);

    for (int l = 0; l < 4; l++)
    {
        transformerBlock->transformer_layer_0_addNorm[l] = createAddNormalize((pre_seq_len + 1), D_MODEL, weightVector[l * 17 + 3], biasVector[l * 17 + 3]);

        for (int n = 0; n < num_heads; n++)
        {
            transformerBlock->selfatten[l * num_heads + n] = &global_selfatten[l * num_heads + n];
            transformerBlock->selfatten[l * num_heads + n]->query_layer = &global_query_layer[l * num_heads + n];
            transformerBlock->selfatten[l * num_heads + n]->key_layer = &global_key_layer[l * num_heads + n];
            transformerBlock->selfatten[l * num_heads + n]->value_layer = &global_value_layer[l * num_heads + n];

            create_SingleHeadSelfAttn(transformerBlock->selfatten[l * num_heads + n], (pre_seq_len + 1), input_dim, head_hidden_size, weightVector + l * 17 + 4 + n * 3);
            #ifdef DEBUG_PRINTS
                printf("self attention %ld, len: %ld\n", l * num_heads + n, transformerBlock->selfatten[l * num_heads + n]->query_layer->input_size_);
            #endif
        }

        transformerBlock->condense[l] = &global_condense[l];
        createDense(transformerBlock->condense[l], num_heads * head_hidden_size, input_dim, weightVector[l * 17 + num_heads * 3 + 4], biasVector[l * 17 + num_heads * 3 + 4]);

        transformerBlock->transformer_layer_1_addNorm[l] = createAddNormalize((pre_seq_len + 1), input_dim, weightVector[l * 17 + num_heads * 3 + 5], biasVector[l * 17 + num_heads * 3 + 5]);

        transformerBlock->feedForward0[l] = &global_FF[2 * l];
        createDense(transformerBlock->feedForward0[l], input_dim, ff_size, weightVector[l * 17 + num_heads * 3 + 6], biasVector[l * 17 + num_heads * 3 + 6]);

        transformerBlock->feedForward1[l] = &global_FF[2 * l + 1];
        createDense(transformerBlock->feedForward1[l], ff_size, input_dim, weightVector[l * 17 + num_heads * 3 + 7], biasVector[l * 17 + num_heads * 3 + 7]);
    }

    transformerBlock->mlp_head_norm = createAddNormalize(1, D_MODEL, weightVector[(NUM_LAYERS - 1) * 17 + NUM_HEAD * 3 + 8], biasVector[(NUM_LAYERS - 1) * 17 + NUM_HEAD * 3 + 8]);

    transformerBlock->mlp_head_linear = &global_mlp;
    createDense(transformerBlock->mlp_head_linear, D_MODEL, D_MODEL, weightVector[(NUM_LAYERS - 1) * 17 + NUM_HEAD * 3 + 9], biasVector[(NUM_LAYERS - 1) * 17 + NUM_HEAD * 3 + 9]);

    return transformerBlock;
}

void destroyTransformerBlock(TransformerBlock *transformerBlock)
{
    // Free dynamically allocated memory

    free(transformerBlock);
}

void computeFixedPoint(TransformerBlock *transformerBlock, size_t seq_len, quant_bit_width *input,
                       quant_bit_width *input_normalized, quant_bit_width *output,
                       quant_bit_width *intermediate, quant_bit_width *qkv)
{
    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_cycles_init();
        int normalize_time = 0;
        timer_start();
    #endif
    printf("\nNormalize weights:\n");
    int16_t *ptr = transformerBlock->addNorm.weight_;
    for (int i = 0; i < 400; i++)
    {
        printf("%d ", *ptr);
        ptr++;
    }
    printf("\nNormalize biases:\n");
    ptr = transformerBlock->addNorm.bias_;
    for (int i = 0; i < 400; i++)
    {
        printf("%d ", *ptr);
        ptr++;
    }
    normalize(&transformerBlock->addNorm, input, input);
    printf("\nNormalize output:\n");
    ptr = input;
    for (int i = 0; i < transformerBlock->addNorm.seq_len_*transformerBlock->addNorm.input_dim_; i++)
    {
        printf("%d ", *ptr);
        ptr++;
    }
    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        printf("normalize time (1st): %d\n", normalize_time);

        timer_start();
    #endif
    // patch embedding layer : matmul to turn the input image into something the transformer can process (tokens)
    computeDense(transformerBlock->patchEmbedding, seq_len, input, output);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int dense_time = timer_stop();
        printf("patch embedding (dense) time: %d\n", dense_time);

        timer_start();
    #endif
    normalize(&transformerBlock->addNorm2, output, output);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        printf("normalize time(2nd): %d\n", normalize_time);

        timer_start();
    #endif
    //insert the cls token at the beginning of the sequence
    clsConcatenate(transformerBlock->token, output, input);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int clsconc_time = timer_stop();
        printf("clsConcatenate time: %d\n", clsconc_time);
    #endif
    seq_len++;
    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_start();
    #endif
    // add the position embedding to the input sequence
    posEmbedding(transformerBlock->token, input);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int posemb_time = timer_stop();
        printf("insert posEmbedding time: %d\n", posemb_time);
    #endif
    for (int l = 0; l < 4; l++) // loop through the 4 transformer layers
    {
        #ifdef PRINT_INTERMEDIATE_CYCLES
            timer_start();
        #endif
        normalize(&transformerBlock->transformer_layer_0_addNorm[l], input, input_normalized);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            normalize_time = timer_stop();
            printf("layer 1st normalize time[%d]: %d\n", l, normalize_time);
        #endif
        for (int n = 0; n < NUM_HEAD; n++)   // loop through the 4 heads in the multi-head attention mechanism
        {
            #ifdef PRINT_INTERMEDIATE_CYCLES
                timer_start();
            #endif
            compute_SingleHeadSelfAttn(transformerBlock->selfatten[l * NUM_HEAD + n], input_normalized,
                                       output + n * (seq_len * transformerBlock->head_hidden_size_), qkv, intermediate);
            #ifdef PRINT_INTERMEDIATE_CYCLES
                int selfattn_time = timer_stop();
                printf("single head self attention time[%d][%d]: %d\n", l, n, selfattn_time);
            #endif
            // destroy_SingleHeadSelfAttn(transformerBlock->selfatten[l * NUM_HEAD + n]);
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            timer_start();
        #endif
        multihead_transpose(output, intermediate, seq_len, transformerBlock->head_hidden_size_, transformerBlock->num_heads_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int transpose_time = timer_stop();
            printf("transpose after self attention time[%d]: %d\n", l, transpose_time);

            timer_start();
        #endif
        computeDense(transformerBlock->condense[l], seq_len, intermediate, output);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int condense_time = timer_stop();
            printf("feed forward 1st dense time[%d]: %d\n", l, condense_time);

            timer_start();
        #endif
        add(input, output, seq_len, transformerBlock->input_dim_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int add_time = timer_stop();
            printf("1st add time[%d]: %d\n", l, add_time);

            timer_start();
        #endif
        normalize(&transformerBlock->transformer_layer_1_addNorm[l], input, input_normalized);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            normalize_time = timer_stop();
            printf("layer 2nd normalize time[%d]: %d\n", l, normalize_time);

            timer_start();
        #endif
        computeDense(transformerBlock->feedForward0[l], seq_len, input_normalized, intermediate);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int dense_time = timer_stop();
            printf("feed forward 2nd dense time[%d]: %d\n", l, dense_time);

            timer_start();
        #endif
        activation(transformerBlock->feedForward0[l], seq_len * transformerBlock->ff_size_, intermediate, intermediate);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int activation_time = timer_stop();
            printf("activation (GeLu) time[%d]: %d\n", l, activation_time);

            timer_start();
        #endif
        computeDense(transformerBlock->feedForward1[l], seq_len, intermediate, output);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            dense_time = timer_stop();
            printf("feed forward 3rd dense time[%d]: %d\n", l, dense_time);

            timer_start();
        #endif
        add(input, output, seq_len, transformerBlock->input_dim_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            add_time = timer_stop();
            printf("2nd add time[%d]: %d\n", l, add_time);
        #endif
    }
    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_start();
    #endif
    normalize(&transformerBlock->mlp_head_norm, input, input_normalized);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        printf("final layer normalization time: %d\n", normalize_time);

        timer_start();
    #endif
    computeDense(transformerBlock->mlp_head_linear, 1, input_normalized, output);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        dense_time = timer_stop();   
        printf("final MLP dense time: %d\n", dense_time);
    #endif
}
