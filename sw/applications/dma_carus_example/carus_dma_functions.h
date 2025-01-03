// Author: Francesco Poluzzi

// SDK library for matrix movement between Carus and memory using DMA.
// All these functions are non-blocking: ypu have to wait for the DMA to finish before reading the results, 
// using the DMA_WAIT macro (defined in dma_sdk.h)

#include "csr.h"
#include "fast_intr_ctrl.h"
#include "ext_irq.h"
#include "vcd_util.h"
#include "carus.h"
#include "core_v_mini_mcu.h"
#include "carus_batchnorm_multivector.h"

#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes

// Note: all these functions take int16_t values and cast them to int32_t values in carus when writing to the VREGs, and vice versa when reading from the VREGs.

dma_config_flags_t run_dma_trans(dma_trans_t *trans)
{
    dma_config_flags_t res1, res2, res3;

    res1 = dma_validate_transaction(trans, DMA_ENABLE_REALIGN, DMA_PERFORM_CHECKS_INTEGRITY);
    if(res1 != DMA_CONFIG_OK)
    {
        printf("Error in dma_validate_transaction 0x%x \n",res1);
    }

    res2 = dma_load_transaction(trans);
    if(res2 != DMA_CONFIG_OK)
    {
        printf("Error in dma_load_transaction 0x%x \n",res2);
    }

    res3 |= dma_launch(trans);
    if(res3 != DMA_CONFIG_OK)
    {
        printf("Error in dma_launch 0x%x \n",res3);
    }
    return res1|res2|res3;
} 

dma_config_flags_t carus_write_flatten_matrix(size_t matrix_size, int16_t *ptr, int16_t starting_index, int8_t vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance,vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = matrix_size, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .sign_ext = 1, 
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_write_flatten_transpose_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = cols, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = rows, 
        .size_d2_du = cols, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, 
        .sign_ext = 1,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };

    return run_dma_trans(&trans);
}

dma_config_flags_t carus_write_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = 1, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/4) - cols +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = cols, 
        .size_d2_du = rows, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .sign_ext = 1,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_write_matrix_transpose(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = cols, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/4) - rows +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = rows, 
        .size_d2_du = cols, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, 
        .sign_ext = 1,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };    
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_write_matrix_tiled(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = matrix_cols - tile_cols + 1, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du =  (CARUS_VREG_SIZE/4) - tile_cols +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = tile_cols, 
        .size_d2_du = rows, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .sign_ext = 1,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

// takes a vertical tile of a matrix and writes it into Carus transposed
dma_config_flags_t carus_write_matrix_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = matrix_cols, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du =  (CARUS_VREG_SIZE/4) - rows +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = rows, 
        .size_d2_du = tile_cols, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, 
        .sign_ext = 1,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_read_flatten_matrix(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance,vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = matrix_size, 
        .src_addr = NULL,
        .src_type = DMA_DATA_TYPE_WORD,
        .dst_type = DMA_DATA_TYPE_HALF_WORD,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .dim = DMA_DIM_CONF_1D,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_read_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du =(CARUS_VREG_SIZE/4) - cols +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = cols ,
        .size_d2_du = rows, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_read_matrix_transpose(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du = CARUS_VREG_SIZE/4, 
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = cols, 
        .size_d2_du = rows, 
        .src_addr = NULL,
        .src_type = DMA_DATA_TYPE_WORD,
        .dst_type = DMA_DATA_TYPE_HALF_WORD,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, 
        .end = DMA_TRANS_END_INTR, 
        .channel = dma_channel,
    };    
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_read_matrix_tiled(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du =  (CARUS_VREG_SIZE/4) - tile_cols +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = matrix_cols - tile_cols + 1, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = tile_cols, 
        .size_d2_du = rows, 
        .src_addr = NULL,
        .src_type = DMA_DATA_TYPE_WORD,
        .dst_type = DMA_DATA_TYPE_HALF_WORD,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .end = DMA_TRANS_END_INTR,
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_read_matrix_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, starting_vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .inc_d2_du = CARUS_VREG_SIZE/4, 
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = matrix_cols - tile_cols + 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = tile_cols, 
        .size_d2_du = rows, 
        .src_addr = NULL,
        .src_type = DMA_DATA_TYPE_WORD,
        .dst_type = DMA_DATA_TYPE_HALF_WORD,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, 
        .end = DMA_TRANS_END_INTR, 
        .channel = dma_channel,
    };    
    return run_dma_trans(&trans);
}
