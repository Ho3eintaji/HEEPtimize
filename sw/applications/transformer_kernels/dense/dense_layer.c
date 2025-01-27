//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include "dense_layer.h"
#include <stdio.h>
#include "defines.h"
#include "addNorm.h"
// #include "coprosit_functions.h"

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

// perform the dense layer matrix multiplication and add the bias if it exists
void computeDense_carus(Dense *dense, size_t seq_len, int16_t *input, int16_t *output)
{   
    // if((dense->input_size_<=MAX_A_COLS_TILE_SIZE) && (seq_len<=CARUS_VREG_SIZE/4) && (dense->output_size_ <= MAX_A_ROWS_TILE_SIZE)){
    if( (dense->input_size_<=MAX_A_COLS_TILE_SIZE) && (seq_len<=CARUS_VREG_SIZE/4) ){
        multiplyweight_carus_regular_tiling(dense, seq_len, input, output);
    }
    else{
        PRINTF("Size not supported\n");
    }
    if (dense->bias != NULL) // if the dense layer has a bias, add it to the output
    {   
        addbias(dense, seq_len, output);
    }
}

// this function handles matrix multiplication in Carus where a_cols=b_rows=(dense->input_size_)<=16 and a_rows=seq_len<CARUS_VREG_SIZE/4 
void multiplyweight_carus_regular_tiling(Dense *dense, size_t seq_len, int16_t *input, int16_t *output){
#ifdef USE_2_CARUS_INSTANCES
    // calculate tiling parameters
    int16_t tile_size = (dense->output_size_ <= MAX_A_ROWS_TILE_SIZE) ? dense->output_size_: MAX_A_ROWS_TILE_SIZE;
    int16_t n_tiles = CEIL_INT_DIV(dense->output_size_ , tile_size);
    int16_t last_tile_size = (n_tiles==1)? tile_size : dense->output_size_ % tile_size;
    // set Carus configuration
    carus_cfg_t cfg = CARUS_CFG_INIT(SINGLE_CARUS_INSTANCE);
    cfg.vl =(uint32_t) seq_len;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_MATMUL_FIXED_OFFSET;
    cfg.arg0 =(uint32_t) tile_size;
    cfg.arg1 =(uint32_t) dense->input_size_;
    carus_set_cfg(SINGLE_CARUS_INSTANCE, &cfg);
    if(n_tiles>1)carus_set_cfg(!SINGLE_CARUS_INSTANCE, &cfg);
    int16_t left_tiles = n_tiles;
    // copy transposed input matrix to Carus (only once)
    carus_write_matrix_transpose(seq_len, dense->input_size_, input, 0, 1, SINGLE_CARUS_INSTANCE, 1);
    if(n_tiles>1)carus_write_matrix_transpose(seq_len, dense->input_size_, input, 0, 1, !SINGLE_CARUS_INSTANCE, 2);
    DMA_WAIT(1);
    if(n_tiles>1)DMA_WAIT(2);
    for(int i = 0 ; i < n_tiles; i+= 2){ // loop over the tiles
        if(left_tiles==1 && n_tiles != 1){ // reduce the kernel size for the last tile
            tile_size = last_tile_size;
            cfg.arg0 = (uint32_t) last_tile_size;
            carus_set_cfg(SINGLE_CARUS_INSTANCE, &cfg);
        }
        int16_t tile_size_2 = tile_size;
        if(left_tiles==2){ // reduce the kernel size for the last tile
            tile_size_2 = last_tile_size;
            cfg.arg0 = (uint32_t) last_tile_size;
            carus_set_cfg(!SINGLE_CARUS_INSTANCE, &cfg);
        }
        // move weight matrix transposed to Carus for this tile
        carus_write_matrix_flatten_tiled_transpose(dense->input_size_, tile_size, dense->output_size_,dense->weight + i*MAX_A_ROWS_TILE_SIZE, 0, 0, SINGLE_CARUS_INSTANCE, 1);
        if(left_tiles>1)carus_write_matrix_flatten_tiled_transpose(dense->input_size_, tile_size_2, dense->output_size_, dense->weight + (i+1)*MAX_A_ROWS_TILE_SIZE, 0, 0, !SINGLE_CARUS_INSTANCE, 2);
        DMA_WAIT(1);
        if(left_tiles>1)DMA_WAIT(2);
        // start the matrix multiplication
        carus_run_kernel(SINGLE_CARUS_INSTANCE);
        if(left_tiles>1)carus_run_kernel(!SINGLE_CARUS_INSTANCE);
        carus_wait_done(SINGLE_CARUS_INSTANCE);
        if(left_tiles>1)carus_wait_done(!SINGLE_CARUS_INSTANCE);
        // transpose the output matrix and move it back to the CPU
        carus_read_matrix_tiled_transpose(seq_len, tile_size, dense->output_size_, output + i*MAX_A_ROWS_TILE_SIZE, 0, 17, SINGLE_CARUS_INSTANCE, 1);
        if(left_tiles>1)carus_read_matrix_tiled_transpose(seq_len, tile_size_2, dense->output_size_, output + (i+1)*MAX_A_ROWS_TILE_SIZE, 0, 17, !SINGLE_CARUS_INSTANCE, 2);
        DMA_WAIT(1);
        if(left_tiles>1)DMA_WAIT(2);
        left_tiles -= 2;
    }
#else
    int16_t tile_size = (dense->output_size_ <= MAX_A_ROWS_TILE_SIZE) 
                        ? dense->output_size_
                        : MAX_A_ROWS_TILE_SIZE;
    int16_t n_tiles = CEIL_INT_DIV(dense->output_size_, tile_size);
    int16_t last_tile_size = (n_tiles == 1) ? tile_size : (dense->output_size_ % tile_size);

    carus_cfg_t cfg = CARUS_CFG_INIT(SINGLE_CARUS_INSTANCE);
    cfg.vl    = (uint32_t)seq_len;
    cfg.vtype = (uint32_t)VTYPE_VSEW_32;
    cfg.koffs = (uint32_t)CARUS_MATMUL_FIXED_OFFSET;
    cfg.arg0  = (uint32_t)tile_size;         // rows for the kernel
    cfg.arg1  = (uint32_t)dense->input_size_; // cols for the kernel
    carus_set_cfg(SINGLE_CARUS_INSTANCE, &cfg);

    // Copy input (already shape: seq_len x input_size) to Carus once
    carus_write_matrix_transpose(seq_len, dense->input_size_, input, 
                                 0, 1, SINGLE_CARUS_INSTANCE, 1);
    DMA_WAIT(1);

    for(int i = 0; i < n_tiles; i++){
        int16_t current_tile_size = tile_size;
        if(i == n_tiles - 1 && n_tiles > 1){ // handle last tile
            current_tile_size = last_tile_size;
            cfg.arg0 = (uint32_t)last_tile_size;
            carus_set_cfg(SINGLE_CARUS_INSTANCE, &cfg);
        }

        // Write the chunk of the weight matrix (transposed)
        carus_write_matrix_flatten_tiled_transpose(dense->input_size_, current_tile_size, 
                                                   dense->output_size_, 
                                                   dense->weight + i * MAX_A_ROWS_TILE_SIZE,
                                                   0, 0, SINGLE_CARUS_INSTANCE, 1);
        DMA_WAIT(1);
        
        // Run the matrix multiplication kernel
        carus_run_kernel(SINGLE_CARUS_INSTANCE);
        carus_wait_done(SINGLE_CARUS_INSTANCE);

        // Read back the results (transposed)
        carus_read_matrix_tiled_transpose(seq_len, current_tile_size, 
                                          dense->output_size_, 
                                          output + i * MAX_A_ROWS_TILE_SIZE,
                                          0, 17, SINGLE_CARUS_INSTANCE, 1);
        DMA_WAIT(1);
    }
#endif

}

// GELU activation function
void activation(Dense *dense, size_t length, int16_t *input, int16_t *output)
{
    float in_float, in_tanh;
    int32_t x3, in_tanh_fxp;
    int32_t a, b, c;
    int32_t qb, qc;
    int32_t q_sign, q;
    int32_t q_L, S_L;
    for (int i = 0; i < length; i++)
    {
        x3 = MUL(MUL(input[i], input[i]), input[i]);
        x3 = MUL(x3, 183); // 183 = 0.044715 in fixed-point 12 bit
        x3 += input[i];
        x3 = MUL(x3, 3268); // 3268 = sqrt(2/PI) in fixed-point 12 bit
        in_float = (float)x3 / (float)(1 << NUM_FRACTION_BITS);
        in_tanh = tanhf(in_float);
        in_tanh_fxp = (int16_t)(in_tanh * (1 << NUM_FRACTION_BITS));
        in_tanh_fxp += (1 << NUM_FRACTION_BITS);
        output[i] = MUL(in_tanh_fxp, input[i] >> 1);
    }
}
