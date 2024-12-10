// Author: Francesco Poluzzi

#include "dma_carus_transfers.h"
#include "carus_matmul_fixed.h"
#include "carus_move_shift_k_T_transformer.h"
#include "carus_matmul_fixed_accumulate.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "ext_irq.h"
#include "vcd_util.h"
#include "defines_transformer_nmc.h"
#include "carus.h"
#include "w25q128jw.h"
#include "core_v_mini_mcu.h"

int system_initialization(){
    // System initialization
    // ---------------------
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
    if (carus_init(0) != 0){
        PRINTF("Error initializing Carus 0\n");
        return 1;
    }
    if (carus_init(1) != 0){
        PRINTF("Error initializing Carus 1\n");
        return 1;
    }

    load_carus_matmul_fixed(0, CARUS_MATMUL_FIXED_OFFSET);
    load_carus_matmul_fixed(1, CARUS_MATMUL_FIXED_OFFSET);
    load_carus_custom_kernel_selfatt(0, CARUS_MOVE_K_TRANSPOSED_OFFSET);
    load_carus_custom_kernel_selfatt(1, CARUS_MOVE_K_TRANSPOSED_OFFSET);
    load_carus_matmul_fixed_accumulate(0, CARUS_MATMUL_FIXED_ACCUMULATE_OFFSET);
    load_carus_matmul_fixed_accumulate(1, CARUS_MATMUL_FIXED_ACCUMULATE_OFFSET);
    carus_cfg_t cfg_0 = CARUS_CFG_INIT(0);    
    carus_cfg_t cfg_1 = CARUS_CFG_INIT(1);
    if (carus_set_cfg(0, &cfg_0) != 0){
        PRINTF("Error configuring carus 0\n");
        return 1;
    }
    if (carus_set_cfg(1, &cfg_1) != 0){
        PRINTF("Error configuring carus 1\n");
        return 1;
    }
}

void __attribute__((noinline)) load_carus_matmul_fixed(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_matmul_fixed, CARUS_MATMUL_FIXED_SIZE, offset) != 0){
        PRINTF("Error loading matmul kernel\n");
        return;
    }
}

void __attribute__((noinline)) load_carus_matmul_fixed_accumulate(uint8_t carus_instance, uint32_t offset){
    // Load fixed point matmul kernel in both carus instances
    if (carus_load_kernel(carus_instance, carus_matmul_fixed_accumulate, CARUS_MATMUL_FIXED_ACCUMULATE_SIZE, offset) != 0){
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

dma_config_flags_t carus_dma_transpose_matmul_A(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel){
    // PRINTF("carus_dma_transpose_matmul_A   rows: %d, cols: %d, ptr: %d, starting_index: %d, carus_instance: %d, dma_channel: %d\n", rows, cols, (void *)ptr, starting_index, carus_instance, dma_channel);
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_A_VREG);
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
        .size_d1_du = rows, // w rows
        .size_d2_du = cols, // w columns
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };

    return run_dma_trans(&trans);
}

dma_config_flags_t carus_dma_move_matmul_A_flattened(size_t matrix_size, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_A_VREG);
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
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };
    return run_dma_trans(&trans);
}


dma_config_flags_t carus_dma_transpose_matmul_B(size_t rows, size_t cols, int16_t *ptr, int8_t carus_instance, int8_t dma_channel){
    // PRINTF("carus_dma_transpose_matmul_B   rows: %d, cols: %d, ptr: %d, carus_instance: %d, dma_channel: %d\n", rows, cols, (void *)ptr, carus_instance, dma_channel);
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_B_VREG);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = cols, 
        .type = DMA_DATA_TYPE_HALF_WORD,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst = {
        .ptr = (uint8_t *) &row_ptr[0],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/4) - rows +1,
        .type = DMA_DATA_TYPE_WORD,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans = {
        .src = &tgt_src,
        .dst = &tgt_dst,
        .size_d1_du = rows, // w rows
        .size_d2_du = cols, // w columns
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };    

    return run_dma_trans(&trans);
}

dma_config_flags_t carus_dma_transpose_matmul_R(size_t rows, size_t cols, int16_t *ptr, int8_t vreg_offset, int8_t carus_instance, int8_t dma_channel){
    // PRINTF("carus_dma_transpose_matmul_R   rows: %d, cols: %d, ptr: %d, vreg_offset: %d, carus_instance: %d, dma_channel: %d\n", rows, cols, (void *)ptr, vreg_offset, carus_instance, dma_channel);
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_R_VREG + vreg_offset);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[0],
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
        .dim_inv = 1, // This is the transposition flag!
        .end = DMA_TRANS_END_INTR, 
        .channel = dma_channel,
    };    
    
    return run_dma_trans(&trans);
}

dma_config_flags_t carus_dma_move_matmul_R(size_t rows, size_t cols, int16_t *ptr, int8_t carus_instance, int8_t dma_channel){
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_R_VREG);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) &row_ptr[0],
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

dma_config_flags_t carus_dma_move_matmul_A_vertical_tile(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel){
    // PRINTF("carus_dma_transpose_matmul_A   rows: %d, cols: %d, ptr: %d, starting_index: %d, carus_instance: %d, dma_channel: %d\n", rows, cols, (void *)ptr, starting_index, carus_instance, dma_channel);
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_A_VREG);
    dma_target_t tgt_src = {
        .ptr = (uint8_t *) ptr,
        .inc_d1_du = 1,
        .inc_d2_du = cols - MAX_A_COLS + 1, 
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
        .size_d1_du = MAX_A_COLS, // w rows
        .size_d2_du = rows, // w columns
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };

    return run_dma_trans(&trans);
}

dma_config_flags_t carus_dma_move_matmul_B_horizontal_tile(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel){
    // PRINTF("carus_dma_transpose_matmul_A   rows: %d, cols: %d, ptr: %d, starting_index: %d, carus_instance: %d, dma_channel: %d\n", rows, cols, (void *)ptr, starting_index, carus_instance, dma_channel);
    int32_t *row_ptr = (int32_t *) carus_vrf(carus_instance, CARUS_MATMUL_B_VREG);
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
        .size_d1_du = cols, // w rows
        .size_d2_du = MAX_B_ROWS, // w columns
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR, // block until the transaction is completed
        .channel = dma_channel,
    };

    return run_dma_trans(&trans);
}
