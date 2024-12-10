//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include <stdio.h>
#include "selfattention_nmc.h"
#include "carus.h"
#include "dma_sdk.h"
#include "dma.h"
#include "dma_carus_transfers.h"
#include "defines_transformer_nmc.h"
#include "timer_sdk.h"
#include "fast_intr_ctrl.h"

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
    self_attn->key_transposed_layer_out = qkv + 3 * self_attn->pre_seq_len * self_attn->head_hidden_size;;

    #ifdef DEBUG_PRINTS
        PRINTF("Starting self attention\n");
        int32_t *row_ptr;
    #endif
    // timer_cycles_init();
    // timer_start();
    // Move input and weights (key, query and value) to carus trough DMA
    if (carus_dma_transpose_matmul_B(self_attn->pre_seq_len, self_attn->query_layer->input_size_, input, CARUS_INSTANCE_1, 0) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for input failed.\n");
        return 1;
    }
    DMA_WAIT(0)
    if (carus_dma_transpose_matmul_A(self_attn->key_layer->input_size_ , self_attn->key_layer->output_size_,  self_attn->key_layer->weight, 0, CARUS_INSTANCE_1, 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for key layer failed.\n");
        return 1;
    }
    int16_t idx = self_attn->key_layer->input_size_ * self_attn->key_layer->output_size_;
    DMA_WAIT(1)
    if (carus_dma_transpose_matmul_A(self_attn->query_layer->input_size_ , self_attn->query_layer->output_size_,  self_attn->query_layer->weight, idx, CARUS_INSTANCE_1, 2) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for query layer failed.\n");
        return 1;
    }
    idx = idx + (self_attn->query_layer->input_size_ * self_attn->query_layer->output_size_);
    DMA_WAIT(2)
    if (carus_dma_transpose_matmul_A(self_attn->value_layer->input_size_ , self_attn->value_layer->output_size_,  self_attn->value_layer->weight, idx, CARUS_INSTANCE_1, 3) != DMA_CONFIG_OK) {
        PRINTF( "Error: DMA transaction for value layer failed.\n");
        return 1;
    }
    DMA_WAIT(3)

    // printf("Copy QKV and input to carus: %d\n", timer_stop());

    // Configure Carus matmul kernel
    carus_cfg_t cfg = CARUS_CFG_INIT(CARUS_INSTANCE_1);
    cfg.vl =(uint32_t) self_attn->pre_seq_len;
    cfg.arg0 =(uint32_t) self_attn->key_layer->output_size_*3;
    cfg.arg1 =(uint32_t) self_attn->key_layer->input_size_;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_MATMUL_FIXED_OFFSET;

    #ifdef DEBUG_PRINTS
        PRINTF("Carus configuration:\n");
        PRINTF("cfg.vl: %d\n", cfg.vl);
        PRINTF("cfg.arg0: %d\n", cfg.arg0);
        PRINTF("cfg.arg1: %d\n", cfg.arg1);
        PRINTF("cfg.vtype: %d\n", cfg.vtype);
        PRINTF("cfg.koffs: %d\n", cfg.koffs);
    #endif
    
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    #ifdef DEBUG_PRINTS
        PRINTF("\nKey layer:\n");
        for (int i = 0; i < self_attn->key_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->key_layer->weight[i * self_attn->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwk^T in Carus\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
        for (int i = 0; i < self_attn->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn->key_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\n");
        PRINTF("\nQuery layer:\n");
        for (int i = 0; i < self_attn->query_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->query_layer->weight[i * self_attn->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwq^T in Carus\n");
        for (int i = 0; i < self_attn->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn->key_layer->input_size_ + j +64]);
            }
            PRINTF("\n");
        }    
        PRINTF("\n");
        PRINTF("\nValue layer:\n");
        for (int i = 0; i < self_attn->value_layer->input_size_; i++) {
            for (int j = 0; j < self_attn->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->value_layer->weight[i * self_attn->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwv^T in Carus\n");
        for (int i = 0; i < self_attn->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn->key_layer->input_size_ + j +128]);
            }
            PRINTF("\n");
        }   
        PRINTF("\nInput :\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->value_layer->input_size_; j++) {
                PRINTF("%x ", input[i * self_attn->value_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }    
        PRINTF("\nInput (transposed) in Carus\n");
        for(int i=1; i<17; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
    #endif
    // timer_cycles_init();
    // timer_start();
    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }
    // printf("First matmul: %d\n", timer_stop());

    // prepare configuration for the custom kernel
    cfg.koffs =(uint32_t) CARUS_MOVE_K_TRANSPOSED_OFFSET;
    cfg.vl =(uint32_t) self_attn->pre_seq_len;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    // wait for kernel to finish
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }
    // timer_cycles_init();
    // timer_start();
    if (carus_dma_transpose_matmul_R(self_attn->pre_seq_len, self_attn->query_layer->output_size_, self_attn->query_layer_out, self_attn->key_layer->output_size_, CARUS_INSTANCE_1 , 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }
    DMA_WAIT(1)
    if (carus_dma_transpose_matmul_R(self_attn->pre_seq_len, self_attn->value_layer->output_size_, self_attn->value_layer_out, self_attn->key_layer->output_size_+ self_attn->query_layer->output_size_, CARUS_INSTANCE_1 , 2) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }   
    DMA_WAIT(2)
    // printf("Move output from carus: %d\n", timer_stop());
    #ifdef DEBUG_PRINTS
        PRINTF("\nKey layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->key_layer_out[i * self_attn->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nK^T in carus:\n");
        for(int i=17; i<17+self_attn->key_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQuery layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->query_layer_out[i * self_attn->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQ^T in carus:\n");
        for(int i=17+self_attn->key_layer->output_size_; i<17+self_attn->key_layer->output_size_+self_attn->query_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nValue layer output:\n");
        for (int i = 0; i < self_attn->pre_seq_len; i++) {
            for (int j = 0; j < self_attn->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn->value_layer_out[i * self_attn->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nV^T in carus:\n");
        for(int i=17+self_attn->key_layer->output_size_+self_attn->query_layer->output_size_; i<17+self_attn->key_layer->output_size_+self_attn->query_layer->output_size_+self_attn->value_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }  
        PRINTF("Carus registers before custom kernel\n");
        for(int i=17; i<21; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", (row_ptr[j]>>1));
            }
            PRINTF("\n");
        }
    #endif
    // timer_cycles_init();
    // timer_start();
    // move K^T to Carus registers [1,4]
    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }
    // printf("Custom kernel: %d\n", timer_stop());

    // prepare configuration for the matmul kernel
    // TODO calculte 15 and 8 at runtime for reconfigurability
    uint32_t no_fitting_tiles = self_attn->pre_seq_len / 15;
    uint32_t last_tile_rows = self_attn->pre_seq_len % 15;
    cfg.vl =(uint32_t) self_attn->pre_seq_len;
    cfg.arg0 =(uint32_t) 15; // max number of rows fitting in one tile
    cfg.arg1 =(uint32_t) self_attn->head_hidden_size;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_MATMUL_FIXED_OFFSET;
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    #ifdef DEBUG_PRINTS
        PRINTF("Carus configuration:\n");
        PRINTF("cfg.vl (b_cols): %d\n", cfg.vl);
        PRINTF("cfg.arg0 (a_rows): %d\n", cfg.arg0);
        PRINTF("cfg.arg1 (a_cols=b_rows): %d\n", cfg.arg1);
        PRINTF("cfg.vtype: %d\n", cfg.vtype);
        PRINTF("cfg.koffs: %d\n", cfg.koffs);
        PRINTF("Carus registers after custom kernel\n");
        for(int i=1; i<5; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
    #endif

    // perform matmul with first 8 tiles of Q
    size_t q_tile_size = self_attn->head_hidden_size*15;
    size_t out_tile_size = self_attn->pre_seq_len*15;
    
    #ifdef DEBUG_PRINTS
        PRINTF("K^T matrix in carus\n");
        for(int i=1; i<5; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
    #endif

    for(int i=0; i<no_fitting_tiles; i++){
    // timer_cycles_init();
    // timer_start();        
        if(carus_dma_move_matmul_A_flattened(q_tile_size, &self_attn->query_layer_out[q_tile_size*i], 0, CARUS_INSTANCE_1, 0) != DMA_CONFIG_OK) {
            PRINTF("Error: DMA transaction for Q failed.\n");
            return 1;
        }
        DMA_WAIT(0)
    // printf("move Q to carus: %d\n", timer_stop());
    // timer_cycles_init();
    // timer_start(); 
        if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
            PRINTF("Error running kernel\n");
            return 1;
        }
        if (carus_wait_done(CARUS_INSTANCE_1) != 0){
            PRINTF("Error waiting for kernel\n");   
            // return 1;
        }
    //  printf("Q*K^T matmul: %d\n", timer_stop());  
        #ifdef DEBUG_PRINTS
            PRINTF("Q matrix in carus, tile %d\n", i);
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
            for(int ii=0; ii<15; ii++){     
                for(int j=0; j<4; j++){
                    PRINTF("%x ", row_ptr[4*ii+j]);
                }
                PRINTF("\n");
            }
            PRINTF("Q*K^T matrix in carus, tile %d\n", i);
            for(int ii=0; ii<15; ii++){     
                row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, ii+17);
                for(int j=0; j<121; j++){
                    PRINTF("%x ", row_ptr[j]);
                }
                PRINTF("\n");
            }
        #endif
    // timer_cycles_init();
    // timer_start();  
        if(carus_dma_move_matmul_R(15, self_attn->pre_seq_len, &intermediate[out_tile_size*i], CARUS_INSTANCE_1, 1) != DMA_CONFIG_OK) {
            PRINTF("Error: DMA transaction for output Q*K^T failed.\n");
            return 1;
        }
        DMA_WAIT(1)
    // printf("move Q*K^T from carus: %d\n", timer_stop());
    }
    // timer_cycles_init();
    // timer_start();  
    // perform matmul with last tile of Q (just one row)
    carus_dma_move_matmul_A_flattened(self_attn->head_hidden_size*last_tile_rows, &self_attn->query_layer_out[q_tile_size*no_fitting_tiles], 0, CARUS_INSTANCE_1, 0);
    cfg.arg0 =(uint32_t) last_tile_rows;
    DMA_WAIT(0)
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }

    #ifdef DEBUG_PRINTS
        PRINTF("Q matrix in carus, last tile\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
        for(int j=0; j<4; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
        
        PRINTF("Q*K^T matrix in carus, last tile %d\n");    
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 17);
        for(int j=0; j<121; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
    #endif

    carus_dma_move_matmul_R(last_tile_rows, self_attn->pre_seq_len, &intermediate[out_tile_size*no_fitting_tiles], CARUS_INSTANCE_1, 1);
    DMA_WAIT(1)
    // printf("Last tile total cycles: %d\n", timer_stop());

    // scale from zero to 1 the output of the matrix multiplication
    // timer_cycles_init();
    // timer_start();   
    computeSoftmax(intermediate, self_attn->pre_seq_len);
    // printf("Softmax: %d\n", timer_stop());
    // softMax(Q x K^T) x V
    // timer_cycles_init();
    // timer_start();
    MatMul_multiply(self_attn->pre_seq_len, intermediate, self_attn->value_layer_out, output, self_attn->pre_seq_len, self_attn->head_hidden_size);
    // printf("Matmul: %d\n", timer_stop());
}

// last matmul with CPU
void compute_DoubleHeadSelfAttn(SingleHeadSelfAttn* self_attn_1, SingleHeadSelfAttn* self_attn_2, int16_t* input, int16_t* output_1, int16_t* output_2, int16_t* qkv, int16_t* qkv_2, int16_t* intermediate_1) {
    
    // DMA channels 0 and 1, carus instance 0 
    self_attn_1->query_layer_out = qkv;
    self_attn_1->key_layer_out = qkv + self_attn_1->pre_seq_len * self_attn_1->head_hidden_size;
    self_attn_1->value_layer_out = qkv + 2 * self_attn_1->pre_seq_len * self_attn_1->head_hidden_size;
    self_attn_1->key_transposed_layer_out = qkv + 3 * self_attn_1->pre_seq_len * self_attn_1->head_hidden_size;
    // DMA channels 2 and 3, carus instance 1
    self_attn_2->query_layer_out = qkv_2;
    self_attn_2->key_layer_out = qkv_2 + self_attn_2->pre_seq_len * self_attn_2->head_hidden_size;
    self_attn_2->value_layer_out = qkv_2 + 2 * self_attn_2->pre_seq_len * self_attn_2->head_hidden_size;
    self_attn_2->key_transposed_layer_out = qkv_2 + 3 * self_attn_2->pre_seq_len * self_attn_2->head_hidden_size;
    // qkv : // 3872 bytes
    #ifdef DEBUG_PRINTS
        PRINTF("Starting self attention\n");
        int32_t *row_ptr;
    #endif

    // Move input and weights (key, query and value) to carus trough DMA
    if (carus_dma_transpose_matmul_B(self_attn_1->pre_seq_len, self_attn_1->query_layer->input_size_, input, CARUS_INSTANCE_0, 0) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for input failed.\n");
        return 1;
    }
    // Move input and weights (key, query and value) to carus trough DMA
    if (carus_dma_transpose_matmul_B(self_attn_2->pre_seq_len, self_attn_2->query_layer->input_size_, input, CARUS_INSTANCE_1, 2) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for input failed.\n");
        return 1;
    }
    if (carus_dma_transpose_matmul_A(self_attn_1->key_layer->input_size_ , self_attn_1->key_layer->output_size_,  self_attn_1->key_layer->weight, 0, CARUS_INSTANCE_0, 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for key layer failed.\n");
        return 1;
    }
    if (carus_dma_transpose_matmul_A(self_attn_2->key_layer->input_size_ , self_attn_2->key_layer->output_size_,  self_attn_2->key_layer->weight, 0, CARUS_INSTANCE_1, 3) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for key layer failed.\n");
        return 1;
    }
    int16_t idx = self_attn_1->key_layer->input_size_ * self_attn_1->key_layer->output_size_;
    DMA_WAIT(1)
    if (carus_dma_transpose_matmul_A(self_attn_1->query_layer->input_size_ , self_attn_1->query_layer->output_size_,  self_attn_1->query_layer->weight, idx, CARUS_INSTANCE_0, 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for query layer failed.\n");
        return 1;
    }
    DMA_WAIT(3)
    if (carus_dma_transpose_matmul_A(self_attn_2->query_layer->input_size_ , self_attn_2->query_layer->output_size_,  self_attn_2->query_layer->weight, idx, CARUS_INSTANCE_1, 3) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for query layer failed.\n");
        return 1;
    }
    idx = idx + (self_attn_1->query_layer->input_size_ * self_attn_1->query_layer->output_size_);
    DMA_WAIT(1)
    if (carus_dma_transpose_matmul_A(self_attn_1->value_layer->input_size_ , self_attn_1->value_layer->output_size_,  self_attn_1->value_layer->weight, idx, CARUS_INSTANCE_0, 1) != DMA_CONFIG_OK) {
        PRINTF( "Error: DMA transaction for value layer failed.\n");
        return 1;
    }
    DMA_WAIT(3)
    if (carus_dma_transpose_matmul_A(self_attn_2->value_layer->input_size_ , self_attn_2->value_layer->output_size_,  self_attn_2->value_layer->weight, idx, CARUS_INSTANCE_1, 3) != DMA_CONFIG_OK) {
        PRINTF( "Error: DMA transaction for value layer failed.\n");
        return 1;
    }

    // Configure Carus matmul kernel
    carus_cfg_t cfg = CARUS_CFG_INIT(CARUS_INSTANCE_0);
    cfg.vl =(uint32_t) self_attn_1->pre_seq_len;
    cfg.arg0 =(uint32_t) self_attn_1->key_layer->output_size_*3;
    cfg.arg1 =(uint32_t) self_attn_1->key_layer->input_size_;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_MATMUL_FIXED_OFFSET;
    DMA_WAIT(0)
    DMA_WAIT(1)
    DMA_WAIT(2)
    DMA_WAIT(3)

    #ifdef DEBUG_PRINTS
        PRINTF("\nHEAD 1 INPUT:\n");
        PRINTF("\nKey layer:\n");
        for (int i = 0; i < self_attn_1->key_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_1->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->key_layer->weight[i * self_attn_1->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwk^T in Carus\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, 0);
        for (int i = 0; i < self_attn_1->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_1->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_1->key_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\n");
        PRINTF("\nQuery layer:\n");
        for (int i = 0; i < self_attn_1->query_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_1->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->query_layer->weight[i * self_attn_1->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwq^T in Carus\n");
        for (int i = 0; i < self_attn_1->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_1->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_1->key_layer->input_size_ + j +64]);
            }
            PRINTF("\n");
        }    
        PRINTF("\n");
        PRINTF("\nValue layer:\n");
        for (int i = 0; i < self_attn_1->value_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_1->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->value_layer->weight[i * self_attn_1->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwv^T in Carus\n");
        for (int i = 0; i < self_attn_1->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_1->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_1->key_layer->input_size_ + j +128]);
            }
            PRINTF("\n");
        }   
        PRINTF("\nInput :\n");
        for (int i = 0; i < self_attn_1->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_1->value_layer->input_size_; j++) {
                PRINTF("%x ", input[i * self_attn_1->value_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }    
        PRINTF("\nInput (transposed) in Carus\n");
        for(int i=1; i<17; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nHEAD 2 INPUT:\n");
        PRINTF("\nKey layer:\n");
        for (int i = 0; i < self_attn_2->key_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_2->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->key_layer->weight[i * self_attn_2->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwk^T in Carus\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
        for (int i = 0; i < self_attn_2->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_2->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_2->key_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\n");
        PRINTF("\nQuery layer:\n");
        for (int i = 0; i < self_attn_2->query_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_2->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->query_layer->weight[i * self_attn_2->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwq^T in Carus\n");
        for (int i = 0; i < self_attn_2->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_2->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_2->key_layer->input_size_ + j +64]);
            }
            PRINTF("\n");
        }    
        PRINTF("\n");
        PRINTF("\nValue layer:\n");
        for (int i = 0; i < self_attn_2->value_layer->input_size_; i++) {
            for (int j = 0; j < self_attn_2->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->value_layer->weight[i * self_attn_2->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nwv^T in Carus\n");
        for (int i = 0; i < self_attn_2->key_layer->output_size_; i++) {
            for (int j = 0; j < self_attn_2->key_layer->input_size_; j++) {
                PRINTF("%x ", row_ptr[i * self_attn_2->key_layer->input_size_ + j +128]);
            }
            PRINTF("\n");
        }   
        PRINTF("\nInput :\n");
        for (int i = 0; i < self_attn_2->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_2->value_layer->input_size_; j++) {
                PRINTF("%x ", input[i * self_attn_2->value_layer->input_size_ + j]);
            }
            PRINTF("\n");
        }    
        PRINTF("\nInput (transposed) in Carus\n");
        for(int i=1; i<17; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
    #endif
    if (carus_set_cfg(CARUS_INSTANCE_0, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_run_kernel(CARUS_INSTANCE_0) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    
    // prepare configuration for the custom kernel
    cfg.koffs =(uint32_t) CARUS_MOVE_K_TRANSPOSED_OFFSET;
    cfg.vl =(uint32_t) self_attn_1->pre_seq_len;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;  

    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }    

    // wait for kernel to finish
    if (carus_wait_done(CARUS_INSTANCE_0) != 0){
        PRINTF("Error waiting for kernel\n");   
        return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_0, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    if (carus_dma_transpose_matmul_R(self_attn_1->pre_seq_len, self_attn_1->query_layer->output_size_, self_attn_1->query_layer_out, self_attn_1->key_layer->output_size_, CARUS_INSTANCE_0 , 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }
    if (carus_dma_transpose_matmul_R(self_attn_2->pre_seq_len, self_attn_2->query_layer->output_size_, self_attn_2->query_layer_out, self_attn_2->key_layer->output_size_, CARUS_INSTANCE_1 , 3) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }
    DMA_WAIT(1)
    DMA_WAIT(3)
    if (carus_dma_transpose_matmul_R(self_attn_1->pre_seq_len, self_attn_1->value_layer->output_size_, self_attn_1->value_layer_out, self_attn_1->key_layer->output_size_+ self_attn_1->query_layer->output_size_, CARUS_INSTANCE_0 , 1) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }   
    if (carus_dma_transpose_matmul_R(self_attn_2->pre_seq_len, self_attn_2->value_layer->output_size_, self_attn_2->value_layer_out, self_attn_2->key_layer->output_size_+ self_attn_2->query_layer->output_size_, CARUS_INSTANCE_1 , 3) != DMA_CONFIG_OK) {
        PRINTF("Error: DMA transaction for output failed.\n");
        return 1;
    }   
    DMA_WAIT(1)
    DMA_WAIT(3)

    #ifdef DEBUG_PRINTS
        PRINTF("\nHEAD 1 OUTPUT:\n");
        PRINTF("\nKey layer output:\n");
        for (int i = 0; i < self_attn_1->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_1->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->key_layer_out[i * self_attn_1->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nK^T in carus:\n");
        for(int i=17; i<17+self_attn_1->key_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, i);
            for(int j=0; j<self_attn_1->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQuery layer output:\n");
        for (int i = 0; i < self_attn_1->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_1->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->query_layer_out[i * self_attn_1->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQ^T in carus:\n");
        for(int i=17+self_attn_1->key_layer->output_size_; i<17+self_attn_1->key_layer->output_size_+self_attn_1->query_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, i);
            for(int j=0; j<self_attn_1->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nValue layer output:\n");
        for (int i = 0; i < self_attn_1->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_1->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_1->value_layer_out[i * self_attn_1->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nV^T in carus:\n");
        for(int i=17+self_attn_1->key_layer->output_size_+self_attn_1->query_layer->output_size_; i<17+self_attn_1->key_layer->output_size_+self_attn_1->query_layer->output_size_+self_attn_1->value_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, i);
            for(int j=0; j<self_attn_1->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }  
        PRINTF("Carus registers before custom kernel\n");
        for(int i=17; i<21; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", (row_ptr[j]>>1));
            }
            PRINTF("\n");
        }
        PRINTF("\nHEAD 2 OUTPUT:\n");
        PRINTF("\nKey layer output:\n");
        for (int i = 0; i < self_attn_2->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_2->key_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->key_layer_out[i * self_attn_2->key_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nK^T in carus:\n");
        for(int i=17; i<17+self_attn_2->key_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn_2->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQuery layer output:\n");
        for (int i = 0; i < self_attn_2->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_2->query_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->query_layer_out[i * self_attn_2->query_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nQ^T in carus:\n");
        for(int i=17+self_attn_2->key_layer->output_size_; i<17+self_attn_2->key_layer->output_size_+self_attn_2->query_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn_2->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nValue layer output:\n");
        for (int i = 0; i < self_attn_2->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_2->value_layer->output_size_; j++) {
                PRINTF("%x ", self_attn_2->value_layer_out[i * self_attn_2->value_layer->output_size_ + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nV^T in carus:\n");
        for(int i=17+self_attn_2->key_layer->output_size_+self_attn_2->query_layer->output_size_; i<17+self_attn_2->key_layer->output_size_+self_attn_2->query_layer->output_size_+self_attn_2->value_layer->output_size_; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<self_attn_2->pre_seq_len; j++){
                PRINTF("%x ", row_ptr[j]);
            }
            PRINTF("\n");
        }  
        PRINTF("Carus registers before custom kernel\n");
        for(int i=17; i<21; i++){
            row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, i);
            for(int j=0; j<121; j++){
                PRINTF("%x ", (row_ptr[j]>>1));
            }
            PRINTF("\n");
        }
    #endif
    // move K^T to Carus registers [1,4]
    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_run_kernel(CARUS_INSTANCE_0) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }

    // prepare configuration for the matmul kernel
    uint32_t no_fitting_tiles = self_attn_1->pre_seq_len / 30;
    uint32_t last_tile_rows = self_attn_1->pre_seq_len % 30;
    cfg.vl =(uint32_t) self_attn_1->pre_seq_len;
    cfg.arg0 =(uint32_t) 15; // max number of rows fitting in one tile
    cfg.arg1 =(uint32_t) self_attn_1->head_hidden_size;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_MATMUL_FIXED_OFFSET;
    // perform matmul with first 8 tiles of Q
    size_t q_tile_size = self_attn_1->head_hidden_size*15;
    size_t out_tile_size = self_attn_1->pre_seq_len*15;  
    uint32_t n_rows =self_attn_1->pre_seq_len/2;

    #ifdef DEBUG_PRINTS
        PRINTF("no_fitting_tiles: %d\n", no_fitting_tiles);
        PRINTF("last_tile_rows: %d\n", last_tile_rows);
        PRINTF("q_tile_size: %d\n", q_tile_size);
        PRINTF("out_tile_size: %d\n", out_tile_size);
        PRINTF("n_rows: %d\n", n_rows);
    #endif

    int16_t *intermediate_2 = intermediate_1 + self_attn_1->pre_seq_len * n_rows + 1; // intermediate occupies 29282 bytes
    int16_t *intermediate_last_1 = &qkv_2[2000];
    int16_t *intermediate_last_2 = &qkv_2[3000];

    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }       
    if (carus_wait_done(CARUS_INSTANCE_0) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_0, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }

    for(int k = 0; k<2 ; k++){ // divide in 2 parts to reduce need of intermediate memory
        for(int i=0; i<no_fitting_tiles; i++){  
            if(carus_dma_move_matmul_A_flattened(q_tile_size, &self_attn_1->query_layer_out[q_tile_size*(i+k*no_fitting_tiles)], 0, CARUS_INSTANCE_0, 0) != DMA_CONFIG_OK) {
                PRINTF("Error: DMA transaction for Q failed.\n");
                return 1;
            }
            if(carus_dma_move_matmul_A_flattened(q_tile_size, &self_attn_2->query_layer_out[q_tile_size*(i+k*no_fitting_tiles)], 0, CARUS_INSTANCE_1, 2) != DMA_CONFIG_OK) {
                PRINTF("Error: DMA transaction for Q failed.\n");
                return 1;
            }
            DMA_WAIT(0)
            DMA_WAIT(2)
            if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
                PRINTF("Error running kernel\n");
                return 1;
            }
            if (carus_run_kernel(CARUS_INSTANCE_0) != 0){
                PRINTF("Error running kernel\n");
                return 1;
            }
            if (carus_wait_done(CARUS_INSTANCE_1) != 0){
                PRINTF("Error waiting for kernel\n");   
                // return 1;
            }
            if(carus_dma_move_matmul_R(15, self_attn_2->pre_seq_len, &intermediate_2[out_tile_size*i], CARUS_INSTANCE_1, 3) != DMA_CONFIG_OK) {
                PRINTF("Error: DMA transaction for output Q*K^T failed.\n");
                return 1;
            }
            if (carus_wait_done(CARUS_INSTANCE_0) != 0){
                PRINTF("Error waiting for kernel\n");   
                // return 1;
            }
            if(carus_dma_move_matmul_R(15, self_attn_1->pre_seq_len, &intermediate_1[out_tile_size*i], CARUS_INSTANCE_0, 1) != DMA_CONFIG_OK) {
                PRINTF("Error: DMA transaction for output Q*K^T failed.\n");
                return 1;
            }    
            #ifdef DEBUG_PRINTS
                PRINTF("Q matrix in carus, HEAD 1, tile %d\n", i);
                row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, 0);
                for(int ii=0; ii<15; ii++){     
                    for(int j=0; j<4; j++){
                        PRINTF("%x ", row_ptr[4*ii+j]);
                    }
                    PRINTF("\n");
                }
                PRINTF("Q*K^T matrix in carus, HEAD 1, tile %d\n", i);
                for(int ii=0; ii<15; ii++){     
                    row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, ii+17);
                    for(int j=0; j<121; j++){
                        PRINTF("%x ", row_ptr[j]);
                    }
                    PRINTF("\n");
                }
                PRINTF("Q matrix in carus, HEAD 2, tile %d\n", i);
                row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
                for(int ii=0; ii<15; ii++){     
                    for(int j=0; j<4; j++){
                        PRINTF("%x ", row_ptr[4*ii+j]);
                    }
                    PRINTF("\n");
                }
                PRINTF("Q*K^T matrix in carus, HEAD 2, tile %d\n", i);
                for(int ii=0; ii<15; ii++){     
                    row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, ii+17);
                    for(int j=0; j<121; j++){
                        PRINTF("%x ", row_ptr[j]);
                    }
                    PRINTF("\n");
                }
            #endif
            DMA_WAIT(1)
            DMA_WAIT(3)
        }
        // scale from zero to 1 the output of the matrix multiplication
        computeSoftmax_nonsquare(intermediate_2, n_rows,  self_attn_2->pre_seq_len);
        computeSoftmax_nonsquare(intermediate_1, n_rows, self_attn_2->pre_seq_len);
        // softMax(Q x K^T) x V
        MatMul_multiply(n_rows, intermediate_1, self_attn_1->value_layer_out, &output_1[k* n_rows* self_attn_1->head_hidden_size], self_attn_1->pre_seq_len, self_attn_1->head_hidden_size);
        MatMul_multiply(n_rows, intermediate_2, self_attn_2->value_layer_out, &output_2[k* n_rows* self_attn_1->head_hidden_size], self_attn_2->pre_seq_len, self_attn_2->head_hidden_size);
    }
 // perform matmul with last tile of Q (just one row) TODO do it trough CPU while Carus is working
    carus_dma_move_matmul_A_flattened(self_attn_1->head_hidden_size*last_tile_rows, &self_attn_1->query_layer_out[q_tile_size*no_fitting_tiles*2], 0, CARUS_INSTANCE_0, 0);
    carus_dma_move_matmul_A_flattened(self_attn_2->head_hidden_size*last_tile_rows, &self_attn_2->query_layer_out[q_tile_size*no_fitting_tiles*2], 0, CARUS_INSTANCE_1, 2);
    cfg.arg0 =(uint32_t) last_tile_rows;
    DMA_WAIT(0)
    DMA_WAIT(2)
    if (carus_set_cfg(CARUS_INSTANCE_0, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }
    if (carus_set_cfg(CARUS_INSTANCE_1, &cfg) != 0){
        PRINTF("Error configuring kernel\n");
        return 1;
    }
    if (carus_run_kernel(CARUS_INSTANCE_1) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_run_kernel(CARUS_INSTANCE_0) != 0){
        PRINTF("Error running kernel\n");
        return 1;
    }
    if (carus_wait_done(CARUS_INSTANCE_1) != 0){
        PRINTF("Error waiting for kernel\n");   
        // return 1;
    }
    carus_dma_move_matmul_R(last_tile_rows, self_attn_2->pre_seq_len, intermediate_last_2, CARUS_INSTANCE_1, 3);
    if (carus_wait_done(CARUS_INSTANCE_0) != 0){  
        // return 1;
    }
    #ifdef DEBUG_PRINTS
        PRINTF("Q matrix in carus, HEAD 1, last tile\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, 0);
        for(int j=0; j<4; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
        
        PRINTF("Q*K^T matrix in carus, HEAD 1, last tile %d\n");    
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_0, 17);
        for(int j=0; j<121; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
        PRINTF("Q matrix in carus, HEAD 2, last tile\n");
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 0);
        for(int j=0; j<4; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
        
        PRINTF("Q*K^T matrix in carus, HEAD 2, last tile %d\n");    
        row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE_1, 17);
        for(int j=0; j<121; j++){
            PRINTF("%x ", row_ptr[j]);
        }
        PRINTF("\n");
    #endif
    carus_dma_move_matmul_R(last_tile_rows, self_attn_1->pre_seq_len, intermediate_last_1, CARUS_INSTANCE_0, 1);
    DMA_WAIT(3)
    computeSoftmax_nonsquare(intermediate_last_2, last_tile_rows,  self_attn_2->pre_seq_len);
    DMA_WAIT(1)
    computeSoftmax_nonsquare(intermediate_last_1, last_tile_rows, self_attn_2->pre_seq_len); 
    MatMul_multiply(last_tile_rows, intermediate_last_1, self_attn_1->value_layer_out, &output_1[2*n_rows* self_attn_1->head_hidden_size], self_attn_1->pre_seq_len, self_attn_1->head_hidden_size);
    MatMul_multiply(last_tile_rows, intermediate_last_2, self_attn_2->value_layer_out, &output_2[2*n_rows* self_attn_1->head_hidden_size], self_attn_2->pre_seq_len, self_attn_2->head_hidden_size);
    #ifdef DEBUG_PRINTS
        PRINTF("Output of the first head:\n");
        for (int i = 0; i < self_attn_1->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_1->head_hidden_size; j++) {
                PRINTF("%x ", output_1[i * self_attn_1->head_hidden_size + j]);
            }
            PRINTF("\n");
        }
        PRINTF("\nOutput of the second head:\n");
        for (int i = 0; i < self_attn_2->pre_seq_len; i++) {
            for (int j = 0; j < self_attn_2->head_hidden_size; j++) {
                PRINTF("%x ", output_2[i * self_attn_2->head_hidden_size + j]);
            }
            PRINTF("\n");
        }
    #endif
}

