// Author: Francesco Poluzzi

#include "dma_carus_transfers.h"
#include "carus_matmul_fixed.h"
#include "carus_move_shift_k_T_transformer.h"
#include "carus_matmul_fixed_accumulate.h"
#include "carus_matmul_set_out_to_zero.h"
#include "carus_matmul_transformer_fold.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "ext_irq.h"
#include "vcd_util.h"
#include "carus.h"
#include "w25q128jw.h"
#include "core_v_mini_mcu.h"
#include "carus_batchnorm_multivector.h"
#include "carus_add.h"

int system_initialization(){
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e)
        return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1
    // Initialize PLIC for external NM-Carus interrupt
    if (ext_irq_init() != 0)
        return 1;
    // Initialize the DMA
    dma_sdk_init();
    // Pick the correct spi device based on simulation type
    spi_host_t* spi = spi_flash;
    // Init SPI host and SPI<->Flash bridge parameters
    if (w25q128jw_init(spi) != FLASH_OK){
        PRINTF("Error initializing SPI flash\n");
        return 1;
    } 
    // Initialize the 2 NM-Carus instances
    if (carus_init(SINGLE_CARUS_INSTANCE) != 0){
        PRINTF("Error initializing Carus 0\n");
        return 1;
    }
    // Load the kernels in the Carus instances
    load_carus_matmul_fixed(SINGLE_CARUS_INSTANCE, CARUS_MATMUL_FIXED_OFFSET);
    load_carus_custom_kernel_selfatt(SINGLE_CARUS_INSTANCE, CARUS_MOVE_K_TRANSPOSED_OFFSET);
    load_carus_batchnorm(SINGLE_CARUS_INSTANCE, CARUS_BATCHNORM_MULTIVECTOR_OFFSET);
    load_carus_add(CARUS_ADD_INSTANCE, CARUS_ADD_OFFSET);

    #ifdef USE_2_CARUS_INSTANCES  // also initialize the second Carus instance
        if (carus_init(!SINGLE_CARUS_INSTANCE) != 0){
            PRINTF("Error initializing Carus 1\n");
            return 1;
        }
        load_carus_batchnorm(!SINGLE_CARUS_INSTANCE, CARUS_BATCHNORM_MULTIVECTOR_OFFSET);
        load_carus_custom_kernel_selfatt(!SINGLE_CARUS_INSTANCE, CARUS_MOVE_K_TRANSPOSED_OFFSET);
        load_carus_matmul_fixed(!SINGLE_CARUS_INSTANCE, CARUS_MATMUL_FIXED_OFFSET);
    #endif
}

void __attribute__((noinline)) load_carus_matmul_fixed(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_matmul_fixed, CARUS_MATMUL_FIXED_SIZE, offset) != 0){
        PRINTF("Error loading matmul kernel\n");
        return;
    }
}

void __attribute__((noinline)) load_carus_custom_kernel_selfatt(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_move_shift_k_T_transformer, CARUS_MOVE_SHIFT_K_T_TRANSFORMER_SIZE, offset) != 0){
        PRINTF("Error loading custom kernel\n");
        return;
    }
}

void __attribute__((noinline)) load_carus_batchnorm(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_batchnorm_multivector, CARUS_BATCHNORM_MULTIVECTOR_SIZE, offset) != 0){
        PRINTF("Error loading batchnorm kernel\n");
        return;
    }
}

void __attribute__((noinline)) load_carus_add(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_add, CARUS_ADD_SIZE, offset) != 0){
        PRINTF("Error loading batchnorm kernel\n");
        return;
    }
}

dma_config_flags_t run_dma_trans(dma_trans_t *trans)
{
    dma_config_flags_t res1, res2, res3;

    res1 = dma_validate_transaction(trans, DMA_ENABLE_REALIGN, DMA_PERFORM_CHECKS_INTEGRITY);
    if(res1 != DMA_CONFIG_OK)
    {
        PRINTF("Error in dma_validate_transaction 0x%x \n",res1);
    }

    res2 = dma_load_transaction(trans);
    if(res2 != DMA_CONFIG_OK)
    {
        PRINTF("Error in dma_load_transaction 0x%x \n",res2);
    }

    res3 |= dma_launch(trans);
    if(res3 != DMA_CONFIG_OK)
    {
        PRINTF("Error in dma_launch 0x%x \n",res3);
    }
    return res1|res2|res3;
} 

dma_config_flags_t carus_dma_move_input_flattended_no_signext(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel){
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
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = matrix_size, 
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_dma_move_output_flattended_no_signext(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance,vreg);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[starting_index],
        .inc_d1_du = 1,
        .type = DMA_DATA_TYPE_HALF_WORD,
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
        .src_type = DMA_DATA_TYPE_HALF_WORD,
        .dst_type = DMA_DATA_TYPE_HALF_WORD,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .dim = DMA_DIM_CONF_1D,
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
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

// takes a vertical tile of a matrix and writes it into Carus transposed
dma_config_flags_t carus_write_matrix_flatten_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel){
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
        .inc_d2_du = 1,
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
