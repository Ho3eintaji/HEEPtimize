//
// Created by alireza on 10/6/23.
//

#include <stdio.h>
#include "selfattentionC.h"
#include "timer_sdk.h"
#include "defines.h"

void create_SingleHeadSelfAttn(SingleHeadSelfAttn* self_attn, size_t pre_seq_len, size_t input_dim, size_t head_hidden_size, int16_t** weightVector) {
    self_attn->pre_seq_len = pre_seq_len;
    self_attn->head_hidden_size = head_hidden_size;
    createDense(self_attn->query_layer, input_dim, head_hidden_size, weightVector[0], NULL);
    createDense(self_attn->key_layer, input_dim, head_hidden_size, weightVector[1], NULL);
    createDense(self_attn->value_layer, input_dim, head_hidden_size, weightVector[2], NULL);
}

void destroy_SingleHeadSelfAttn(SingleHeadSelfAttn* self_attn) {
    free(self_attn->query_layer_out);
    free(self_attn->key_layer_out);
    free(self_attn->key_transposed_layer_out);
    free(self_attn->value_layer_out);
    free(self_attn->attention_scores);

    destroyDense(self_attn->query_layer);
    destroyDense(self_attn->key_layer);
    destroyDense(self_attn->value_layer);

    free(self_attn);
}

void compute_SingleHeadSelfAttn(SingleHeadSelfAttn* self_attn, int16_t* input, int16_t* output, int16_t* qkv, int16_t* intermediate) {
    self_attn->query_layer_out = qkv;
    self_attn->key_layer_out = qkv + self_attn->pre_seq_len * self_attn->head_hidden_size;
    self_attn->value_layer_out = qkv + 2 * self_attn->pre_seq_len * self_attn->head_hidden_size;
    self_attn->key_transposed_layer_out = qkv + 3 * self_attn->pre_seq_len * self_attn->head_hidden_size;

    //perform the 3 dense layers for the query, key, and value
    t_tmp = timer_get_cycles();
    computeDense(self_attn->query_layer, self_attn->pre_seq_len, input, self_attn->query_layer_out);
    computeDense(self_attn->key_layer, self_attn->pre_seq_len, input, self_attn->key_layer_out);
    computeDense(self_attn->value_layer, self_attn->pre_seq_len, input, self_attn->value_layer_out);
    t_matmul_add += timer_get_cycles() - t_tmp;

    #ifdef DEBUG_PRINTS
        printf("\nKey layer:\n");
        for (int i = 0; i < self_attn->key_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->key_layer->output_size_; j++) {
                printf("%x ", self_attn->key_layer->weight[i * self_attn->key_layer->output_size_ + j]);
            }
            printf("\n");
        }
        printf("\nQuery layer:\n");
        for (int i = 0; i < self_attn->query_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->query_layer->output_size_; j++) {
                printf("%x ", self_attn->query_layer->weight[i * self_attn->query_layer->output_size_ + j]);
            }
            printf("\n");
        }
        printf("\nValue layer:\n");
        for (int i = 0; i < self_attn->value_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->value_layer->output_size_; j++) {
                printf("%x ", self_attn->value_layer->weight[i * self_attn->value_layer->output_size_ + j]);
            }
            printf("\n");
        }
        printf("\nInput :\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->value_layer->input_size_; j++) {
                printf("%x ", input[i * self_attn->value_layer->input_size_ + j]);
            }
            printf("\n");
        }
        printf("\nKey layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->key_layer->output_size_; j++) {
                printf("%x ", self_attn->key_layer_out[i * self_attn->key_layer->output_size_ + j]);
            }
            printf("\n");
        }
        printf("\nQuery layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->query_layer->output_size_; j++) {
                printf("%x ", self_attn->query_layer_out[i * self_attn->query_layer->output_size_ + j]);
            }
            printf("\n");
        }
        printf("\nValue layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->value_layer->output_size_; j++) {
                printf("%x ", self_attn->value_layer_out[i * self_attn->value_layer->output_size_ + j]);
            }
            printf("\n");
        }
    #endif

    #ifdef PRINT_INTERMEDIATE_CYCLES
        timer_cycles_init();
        timer_start();
    #endif

    // transpose the key layer
    t_tmp = timer_get_cycles();
    transpose_quant(self_attn->key_layer_out, self_attn->key_transposed_layer_out, self_attn->pre_seq_len, self_attn->head_hidden_size);
    t_transpose += timer_get_cycles() - t_tmp;
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int transpose_time = timer_stop();
        PRINTF("transpose time: %d\n", transpose_time);
        timer_cycles_init();
        timer_start();
    #endif
    // sscale the key matrix, which helps normalize the attention scores (shift right by 1)
    t_tmp = timer_get_cycles();
    MatMul_scale(self_attn->key_transposed_layer_out, 1, self_attn->pre_seq_len * self_attn->head_hidden_size);
    t_mm_scale += timer_get_cycles() - t_tmp;
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int scale_time = timer_stop();
        PRINTF("scale time: %d\n", scale_time);
        timer_cycles_init();
        timer_start();
    #endif
    
    #ifdef DEBUG_PRINTS
        printf( "K transposed and scaled:\n");
        for (int i=0; i<self_attn->head_hidden_size; i++) {
            for (int j=0; j<self_attn->pre_seq_len; j++) {
                printf("%x ", self_attn->key_transposed_layer_out[i * self_attn->pre_seq_len + j]);
            }
            printf("\n");
        }
        printf("Q matrix:\n");
        for(int i=0;i<121; i++){
            if(i%15==0){
                printf("\n");
            }
            for(int j=0;j<4; j++){
                printf("%x ", self_attn->query_layer_out[i*4+j]);
            }
            printf("\n");
        }
    #endif

    // perform Q x K^T
    t_tmp = timer_get_cycles();
    MatMul_multiply(self_attn->pre_seq_len, self_attn->query_layer_out, self_attn->key_transposed_layer_out, intermediate, self_attn->head_hidden_size, self_attn->pre_seq_len);
    t_matmul += timer_get_cycles() - t_tmp;
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int matmul_time = timer_stop();
        PRINTF("matmul time QxK^T: %d\n", matmul_time);
        timer_cycles_init();
        timer_start();
    #endif
    #ifdef DEBUG_PRINTS
        printf("Q x K^T matrix:\n");    
        for(int i=0;i<121; i++){
            if(i%15==0){
                printf("\n");
            }
            for(int j=0;j<121; j++){
                printf("%x ", intermediate[i*121+j]);
            }
            printf("\n");
        }
    #endif
    
    // scale from zero to 1 the output of the matrix multiplication
    t_tmp = timer_get_cycles();
    computeSoftmax(intermediate, self_attn->pre_seq_len);
    t_softmax += timer_get_cycles() - t_tmp;

    #ifdef PRINT_INTERMEDIATE_CYCLES
        int softmax_time = timer_stop();
        PRINTF("softmax time: %d\n", softmax_time);
        timer_cycles_init();
        timer_start();
    #endif
    // softMax(Q x K^T) x V
    t_tmp = timer_get_cycles();
    MatMul_multiply(self_attn->pre_seq_len, intermediate, self_attn->value_layer_out, output, self_attn->pre_seq_len, self_attn->head_hidden_size);
    t_matmul += timer_get_cycles() - t_tmp;
    #ifdef PRINT_INTERMEDIATE_CYCLES
        int matmul_time2 = timer_stop();
        PRINTF("matmul time softmax(QxK^T)xV: %d\n", matmul_time2);
    #endif
}
