//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//
// vscode-fold=1
#include <stdio.h>
#include "transformerBlockC.h"
#include "timer_sdk.h"
#include "w25q128jw.h"
#include "core_v_mini_mcu.h"
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
            //PRINTF("self attention %ld, len: %ld\n", l * num_heads + n, transformerBlock->selfatten[l * num_heads + n]->query_layer->input_size_);
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
                       quant_bit_width *intermediate, quant_bit_width *qkv, quant_bit_width* ram_buffer)
{
    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_cycles_init();
        int normalize_time = 0;
        timer_start();
    #endif

    // ram buffer is already initialized with weights of first normalization
    transformerBlock->addNorm.weight_ = ram_buffer;
    transformerBlock->addNorm.bias_ = ram_buffer+400;

    normalize(&transformerBlock->addNorm, input, input); 

    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->addNorm2.weight_), ram_buffer, 64) != FLASH_OK)return -1; // read addnorm2 in ram_buffer[0]

    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        PRINTF("normalize time: %d\n", normalize_time);
        timer_start();
    #endif

    // ram buffer is already initialized with patch emebdding weigths and biases
    transformerBlock->patchEmbedding->weight = ram_buffer+800;
    transformerBlock->patchEmbedding->bias = ram_buffer+800+6400;
    computeDense(transformerBlock->patchEmbedding, seq_len, input, output);// patch embedding layer : matmul to turn the input image into something the transformer can process (tokens)
    w25q128jw_wait_quad_dma_async(ram_buffer, 64);
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->token->pos_matrix_), ram_buffer+32, 1936*2 + 32) != FLASH_OK)return -1; // read pos_matrix and cls_token_vector

    #ifdef PRINT_INTERMEDIATE_CYCLES
        int dense_time = timer_stop();
        PRINTF("dense time: %d\n", dense_time);
        timer_start();
    #endif

    transformerBlock->addNorm2.weight_ = ram_buffer;
    transformerBlock->addNorm2.bias_ = ram_buffer+16;
    normalize(&transformerBlock->addNorm2, output, output);

    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        PRINTF("normalize time: %d\n", normalize_time);
        timer_start();
    #endif

    transformerBlock->token->pos_matrix_ = ram_buffer+32;
    transformerBlock->token->cls_token_vector_ = ram_buffer+32+1936;

    w25q128jw_wait_quad_dma_async(ram_buffer+32, 1936*2 + 32); // wait for pos embeddings and cls tokens to be loaded

    //insert the cls token at the beginning of the sequence
    clsConcatenate(transformerBlock->token, output, input);
    seq_len++;

    #ifdef PRINT_INTERMEDIATE_CYCLES
        int clsconc_time = timer_stop();
        PRINTF("clsConcatenate time: %d\n", clsconc_time);
        timer_start();
    #endif

    // add the position embedding to the input sequence
    posEmbedding(transformerBlock->token, input);

    #ifdef PRINT_INTERMEDIATE_CYCLES
        int posemb_time = timer_stop();
        PRINTF("posEmbedding time: %d\n", posemb_time);
    #endif

    for (int l = 0; l < 4; l++) // loop through the 4 transformer layers
    {
        #ifdef DEBUG_PRINTS
            PRINTF("Starting transformer layer %d\n", l);
        #endif
        #ifdef PRINT_INTERMEDIATE_CYCLES
            timer_start();
        #endif
        normalize(&transformerBlock->transformer_layer_0_addNorm[0], input, input_normalized);
        if(l>0)w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->feedForward1[0]->weight, 160); // wait for loading of ff2 of this layer to be finished
        if(l<3){ // load successive layer
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->transformer_layer_0_addNorm[l+1].weight_), (uint32_t *)transformerBlock->transformer_layer_0_addNorm[0].weight_, 64) != FLASH_OK)return 1;
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            normalize_time = timer_stop();
            PRINTF("normalize time[%d]: %d\n", l, normalize_time);
        #endif
        for (int n = 0; n < NUM_HEAD; n+=2)   // loop through the 4 heads in the multi-head attention mechanism, 2 at a time
        {
            #ifdef PRINT_INTERMEDIATE_CYCLES
                timer_cycles_init();
                timer_start();
            #endif
            compute_SingleHeadSelfAttn(transformerBlock->selfatten[ n], input_normalized,
                                    output + n * (seq_len * transformerBlock->head_hidden_size_), qkv, intermediate);
            #ifdef PRINT_INTERMEDIATE_CYCLES
                int selfattn_time = timer_stop();
                PRINTF("single head time[%d][%d]: %d\n", l, n, selfattn_time);
            #endif
            #ifdef PRINT_INTERMEDIATE_CYCLES
                timer_cycles_init();
                timer_start();
            #endif
            compute_SingleHeadSelfAttn(transformerBlock->selfatten[ n+1], input_normalized,
                                    output + (n+1) * (seq_len * transformerBlock->head_hidden_size_), qkv, intermediate);
            #ifdef PRINT_INTERMEDIATE_CYCLES
                selfattn_time = timer_stop();
                PRINTF("single head time[%d][%d]: %d\n", l, n+1, selfattn_time);
            #endif
            if(l<3){ // load 2 heads of successive layer from flash
                if(n==0)w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->transformer_layer_0_addNorm[0].weight_, 64); // wait for the first normalize weigths to be loaded
                else w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->selfatten[n]->query_layer->weight, 384*2); // wait for the previous 2 heads to be leaded for the next layer
                if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->selfatten[(l+1)*NUM_HEAD + n ]->query_layer->weight),(uint32_t *)transformerBlock->selfatten[n]->query_layer->weight, 384*2) != FLASH_OK)return 1;
            }
        }  
        #ifdef PRINT_INTERMEDIATE_CYCLES
            timer_start();
        #endif
        multihead_transpose(output, intermediate, seq_len, transformerBlock->head_hidden_size_, transformerBlock->num_heads_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int transpose_time = timer_stop();
            PRINTF("transpose time[%d]: %d\n", l, transpose_time);
            timer_start();
        #endif
    
        computeDense(transformerBlock->condense[0], seq_len, intermediate, output);

        if(l<3){ // load successive layer from flash
            w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->selfatten[2]->query_layer->weight, 384*2); // wait for the previous dma transfer to be performed
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->condense[l+1]->weight),(uint32_t *)transformerBlock->condense[0]->weight, 272*2) != FLASH_OK)return 1;
        }
        else{ // load mlp head norm if final layer
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->mlp_head_norm.weight_), ram_buffer, 64) != FLASH_OK)return -1;
            transformerBlock->mlp_head_norm.weight_ = ram_buffer;
            transformerBlock->mlp_head_norm.bias_ = ram_buffer+16;
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int condense_time = timer_stop();
            PRINTF("condense time[%d]: %d\n", l, condense_time);
            timer_start();
        #endif
        add(input, output, seq_len, transformerBlock->input_dim_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int add_time = timer_stop();
            PRINTF("add time[%d]: %d\n", l, add_time);
            timer_start();
        #endif
        normalize(&transformerBlock->transformer_layer_1_addNorm[0], input, input_normalized);
        if(l<3){ // load successive layer
            w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->condense[0]->weight, 272*2);
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->transformer_layer_1_addNorm[l+1].weight_), (uint32_t *)transformerBlock->transformer_layer_1_addNorm[0].weight_, 64) != FLASH_OK)return 1;
        }
        else{ // load mlp head linear if final layer
            w25q128jw_wait_quad_dma_async( ram_buffer, 64);
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->mlp_head_linear->weight), ram_buffer+32, 272*2) != FLASH_OK)return -1;
            transformerBlock->mlp_head_linear->weight = ram_buffer+32;
            transformerBlock->mlp_head_linear->bias = ram_buffer+32+256;
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            normalize_time = timer_stop();
            PRINTF("normalize time[%d]: %d\n", l, normalize_time);
            timer_start();
        #endif
        computeDense(transformerBlock->feedForward0[0], seq_len, input_normalized, intermediate);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int dense_time = timer_stop();
            PRINTF("dense time[%d]: %d\n", l, dense_time);
            timer_start();
        #endif
        activation(transformerBlock->feedForward0[0], seq_len * transformerBlock->ff_size_, intermediate, intermediate);
        if(l<3){ // load successive layer from flash
            w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->transformer_layer_1_addNorm[0].weight_, 64);
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->feedForward0[l+1]->weight),(uint32_t *)transformerBlock->feedForward0[0]->weight, 68*2) != FLASH_OK)return 1;
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            int activation_time = timer_stop();
            PRINTF("activation time[%d]: %d\n", l, activation_time);
            timer_start();
        #endif
        computeDense(transformerBlock->feedForward1[0], seq_len, intermediate, output);
        if(l<3){ // load successive layer from flash
            w25q128jw_wait_quad_dma_async((uint32_t *)transformerBlock->feedForward0[0]->weight, 68*2);
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)transformerBlock->feedForward1[l+1]->weight),(uint32_t *)transformerBlock->feedForward1[0]->weight, 160) != FLASH_OK)return 1;
        }
        #ifdef PRINT_INTERMEDIATE_CYCLES
            dense_time = timer_stop();
            PRINTF("dense time[%d]: %d\n", l, dense_time);
            timer_start();
        #endif
        add(input, output, seq_len, transformerBlock->input_dim_);
        #ifdef PRINT_INTERMEDIATE_CYCLES
            add_time = timer_stop();
            PRINTF("add time[%d]: %d\n", l, add_time);
        #endif
    }
    
    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_start();
    #endif

    normalize(&transformerBlock->mlp_head_norm, input, input_normalized); 

    #ifdef PRINT_INTERMEDIATE_CYCLES
        normalize_time = timer_stop();
        PRINTF("normalize time: %d\n", normalize_time);
        timer_start();
    #endif

    w25q128jw_wait_quad_dma_async(ram_buffer+32, 272*2);
    computeDense(transformerBlock->mlp_head_linear, 1, input_normalized, output);

    #ifdef PRINT_INTERMEDIATE_CYCLES
        dense_time = timer_stop();   
        PRINTF("dense time: %d\n", dense_time);
    #endif
}
