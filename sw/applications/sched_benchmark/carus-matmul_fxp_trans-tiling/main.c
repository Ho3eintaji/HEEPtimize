// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein Taji
// Date: 22/06/2023
// Description: Main file for running matmul on multi-accels platform

// Description: Main file for the matrix multiplication with transposition using DMA application
// Prform A*B = R using transpose operation, by using the property:
// (B^T * A^T)^T = A * B = R
// note: this is useful for Carus' bigger size limits on matrix A than on matrix B in kernel "carus-matmul_fixed.S"


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "heepatia.h"
#include "csr.h"
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "vcd_util.h"
#include "ext_irq.h"
#include "timer_sdk.h"
#include "x-heep.h"
#include "w25q128jw.h"
#include "defines.h"
#include "core_v_mini_mcu.h"
#include "data.h"
#include "carus.h"
#include "carus_matmul_fixed.h"

/****************************************************************************/
/**                        DEFINITIONS AND MACROS                          **/
/****************************************************************************/

// #define DEBUG
// #define DEBUG_DMA
#define CARUS_MEM_SIZE (64 * 1024 / sizeof(data_t))

#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define DMA_CHANNEL_A 2
#define DMA_CHANNEL_B 3

/***************************************************************4*************/
/**                      PROTOTYPES OF LOCAL FUNCTIONS                     **/
/****************************************************************************/

// Launch carus matmul
void carusMatmul(data_t *A, data_t *B, data_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols);

// CGRA matmul tiled
void carusMatMulTransTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, int32_t *cgra_buffer, uint32_t cgra_buffer_size);

dma_config_flags_t run_dma_2d_transp_trans(dma_trans_t *trans);

/****************************************************************************/
/**                           GLOBAL VARIABLES                             **/
/****************************************************************************/

// cache: whole data is cached
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS] = {0};
data_t *A_ram = cache;
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;

data_t R_carus[R_ROWS * R_COLS] = {0}; // Result computed by NM-Carus

/****************************************************************************/

int main(void)
{

    uint32_t t1, t2, t_pe, t_cpu;
    
    /* ===========================================      
    * ========== Initialization ==================
    * ============================================ */

    // Initialize the DMA
    dma_sdk_init();
    // Pick the correct spi device based on simulation type
    spi_host_t* spi = spi_flash;
    // Init SPI host and SPI<->Flash bridge parameters
    if (w25q128jw_init(spi) != FLASH_OK){
        PRINTF("Error initializing SPI flash\n");
        return 1;
    } 

    // init_system();
    if (vcd_init() != 0) return 1;
    timer_cycles_init();
    timer_start();

    // ----- System initialization -----
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1

    // Initialize PLIC for external NM-Carus interrupt
    if (ext_irq_init() != 0) return 1;

    // ------ Initialize NM-Carus ------
    // Initialize NM-Carus
    if (carus_init(CARUS_INSTANCE) != 0)
        return 1;

    // Load kernel
    if (carus_load_kernel(CARUS_INSTANCE, carus_matmul_fixed, CARUS_MATMUL_FIXED_SIZE, 0) != 0){
        printf("Error loading kernel\n");
        return 1;
    }


    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);

    /* =======================================
    * ====== Runing on CGRA ======
    * ======================================== */
   t1 = timer_get_cycles();
//    cgraMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cgra_buffer, CGRA_BUFFER_SIZE);
    carusMatmul(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS);
    t_pe = timer_get_cycles() - t1;

    PRINTF("Carus MatMul completed in %u cycles.\n", t_pe);

    for (size_t i = 0; i < 4; i++)
    {
        PRINTF("R[%d]: %x\n", i, (uint16_t) R_ram[i]);
    }
    for (size_t i = 0; i < 4; i++)
    {
        //last 4 value
        PRINTF("R[%d]: %x\n", R_ROWS*R_COLS - 4 + i, (uint16_t) R_ram[(1)*R_COLS - 4 + i]);
    }
    

    return 0;
}

void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols)
{
    for (unsigned int i = 0; i < A_rows; i++) {
        for (unsigned int j = 0; j < B_cols; j++) {
            C[i*B_cols+j] = 0;
            for (unsigned int k = 0; k < A_cols; k++) {
                C[i*B_cols+j] += A[i*A_cols+k] * B[k*B_cols+j];
            }
        }
    }
}


//carus Matmul
void carusMatmul(data_t *A, data_t *B, data_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols)
{

    data_t *row_ptr;
    data_t_double *row_ptr_double;

    // ----- Carus configuration -----
    carus_cfg_t cfg = CARUS_CFG_INIT(0);
    cfg.vl    = (uint32_t)A_rows;
    cfg.arg0  = (uint32_t)B_cols;         
    cfg.arg1  = (uint32_t)A_cols; 
    cfg.vtype = (uint32_t)VTYPE_VSEW_32;
    if (carus_set_cfg(CARUS_INSTANCE, &cfg) != 0){
        printf("Error configuring kernel\n");
        return 1;
    }

    dma_data_type_t dma_type = DMA_DATA_TYPE_HALF_WORD;
    dma_data_type_t dma_type_double = DMA_DATA_TYPE_WORD;

    // Transfer data to NM-Carus
    // -------------------------
    // Copy flattened transposed matrix B
    row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_A_VREG);

    dma_target_t tgt_src_B = {
        .ptr = (uint8_t *) B,
        .inc_d1_du = 1,
        .inc_d2_du = B_cols,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_B = {
        .ptr = (uint8_t *) &row_ptr[0],
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_B = {
        .src = &tgt_src_B,
        .dst = &tgt_dst_B,
        .size_d1_du = A_cols,
        .size_d2_du = B_cols,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR_WAIT, // block until the transaction is completed
        .channel = DMA_CHANNEL_B,
    };

    // Copy matrix A transposed
    row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_B_VREG );
    dma_target_t tgt_src_A = {
        .ptr = (uint8_t *)  A,
        .inc_d1_du = 1,
        .inc_d2_du = A_cols,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_A = {
        .ptr = (uint8_t *) &row_ptr[0],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/(2*ELEM_SIZE)) - A_rows +1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_A = {
        .src = &tgt_src_A,
        .dst = &tgt_dst_A,
        .size_d1_du = A_rows ,
        .size_d2_du = A_cols,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR_WAIT, // block until the transaction is completed
        .channel = DMA_CHANNEL_A,
    };

    if (run_dma_2d_transp_trans(&trans_B) != 0)
    {
        printf("Error! DMA B^T transaction failed\n");
        return 1;
    }
    dma_config_flags_t res_A=run_dma_2d_transp_trans(&trans_A);
    if (res_A != 0)
    {
        printf("Error! DMA A^T transaction failed with error 0x%x\n",res_A);
        return 1;
    }

    DMA_WAIT(DMA_CHANNEL_B)
    DMA_WAIT(DMA_CHANNEL_A)

    // ----- Run the kernel -----
    if (carus_run_kernel(CARUS_INSTANCE) != 0){
        printf("Error running kernel\n");
        return 1;
    }
    // Wait for the kernel to complete
    if (carus_wait_done(CARUS_INSTANCE) != 0) return 1;


    // ----- copy data from carus ------
    // move back the result from NM-Carus (transposed)
    row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_R_VREG );
    dma_target_t tgt_src_R = {
        .ptr = (uint8_t *) &row_ptr[0] ,
        .inc_d1_du = 1,
        .inc_d2_du = CARUS_VREG_SIZE/(2*ELEM_SIZE),
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_R = {
        .ptr = (uint8_t *) C,
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_R = {
        .src = &tgt_src_R,
        .dst = &tgt_dst_R,
        .size_d1_du = R_COLS ,
        .size_d2_du = R_ROWS,
        .src_addr = NULL,
        .src_type = dma_type_double,
        .dst_type = dma_type,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .end = DMA_TRANS_END_INTR_WAIT, // block until the transaction is completed
        .channel = DMA_CHANNEL_A,
    }; 


    if (run_dma_2d_transp_trans(&trans_R) != 0)
    {
        printf("Error! DMA R transaction failed\n");
        return 1;
    }

    DMA_WAIT(DMA_CHANNEL_A)


    // row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_R_VREG + 0);


}

// void cgraMatMulTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram,
//     uint32_t A_rows, uint32_t A_cols, uint32_t B_cols,
//     int32_t *cgra_buffer, uint32_t cgra_buffer_size) {

//     unsigned int K = A_cols;
//     unsigned int max_elements = cgra_buffer_size;
//     dma_sdk_init();

//     // Check if the entire matrices fit in the CGRA buffer
//     if (A_rows * A_cols + A_cols * B_cols + A_rows * B_cols <= max_elements) {
//         // Direct execution without tiling
//         dma_copy((uint32_t)cgra_buffer, (uint32_t)A_ram, A_rows * A_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
//         dma_copy((uint32_t)(cgra_buffer + A_rows * A_cols), (uint32_t)B_ram, A_cols * B_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
//         cgraMatMulRun(cgra_buffer, cgra_buffer + A_rows * A_cols, R_ram, A_rows, A_cols, B_cols);
//         dma_copy((uint32_t)R_ram, (uint32_t)(cgra_buffer + A_rows * A_cols + A_cols * B_cols), A_rows * B_cols, 0, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
//         return; // Exit the function
//     }

//     unsigned int M = 0, N = 0;
//     unsigned int A_rows_div_4 = A_rows >> 2; // A_rows / 4
//     unsigned int B_cols_div_4 = B_cols >> 2; // B_cols / 4

//     // Find maximum M and N that fit in CGRA memory and are multiples of 4
//     for (M = A_rows_div_4 << 2; M >= 4; M -= 4) {
//         unsigned int a_tile = M * K;
//         if (a_tile >= max_elements) continue;

//         unsigned int remaining = max_elements - a_tile;
//         unsigned int denom = K + M;
//         if (denom == 0) break;

//         unsigned int max_N = remaining / denom;
//         N = (max_N >> 2) << 2; // max_N / 4 * 4
//         if (N >= 4) {
//             if (N > B_cols) {
//                 N = B_cols_div_4 << 2;
//                 if (N < 4) continue;
//             }
//             if ((K * N + M * N) <= remaining) break;
//         }
//     }

//     if (M < 4) {
//         PRINTF("Error: Cannot find valid tile sizes.\n");
//         return;
//     }

//     /* Process each tile */
//     for (unsigned int i = 0; i < A_rows; i += M) {
//         unsigned int current_M = (i + M <= A_rows) ? M : A_rows - i;
//         current_M &= ~0x3; // current_M = (current_M / 4) * 4;
//         current_M = current_M < 4 ? 4 : current_M;

//         for (unsigned int j = 0; j < B_cols; j += N) {
//             unsigned int current_N = (j + N <= B_cols) ? N : B_cols - j;
//             current_N &= ~0x3; // current_N = (current_N / 4) * 4;
//             current_N = current_N < 4 ? 4 : current_N;

//             /* Calculate buffer pointers */
//             size_t a_size = current_M * K;
//             size_t b_size = K * current_N;
//             size_t r_size = current_M * current_N;

//             if (a_size + b_size + r_size > CGRA_BUFFER_SIZE) {
//                 PRINTF("Tile exceeds CGRA memory.\n");
//                 return -1;
//             }

//         #ifdef DEBUG
//             PRINTF("tile size: %d x %d x %d\n", current_M, K, current_N);
//         #endif

//             int32_t *A_cgra = cgra_buffer;
//             int32_t *B_cgra = A_cgra + a_size;
//             int32_t *R_cgra = B_cgra + b_size;

//             /* Copy B tile (row-wise slices) */
//             for (unsigned int k = 0; k < K; ++k) {
//                 dma_copy((uint32_t)(B_cgra + k * current_N), (uint32_t)(B_ram + k * B_cols + j), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
//             }
//             /* Copy A tile */
//             dma_copy((uint32_t)(A_cgra), (uint32_t)(A_ram + i * K), current_M * K, 2, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);

//             /* Run CGRA MatMul */
//             cgra_intr_flag = 0;
//             cgraMatMulRun(A_cgra, B_cgra, R_cgra, current_M, K, current_N);
//             while (!cgra_intr_flag) { wait_for_interrupt(); }

//             /* Copy result back to R_ram */
//             for (unsigned int row = 0; row < current_M; ++row) {
//                 dma_copy((uint32_t)(R_ram + (i + row) * B_cols + j), (uint32_t)(R_cgra + row * current_N), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
//             }
//         }
//     }
// }

dma_config_flags_t run_dma_2d_transp_trans(dma_trans_t *trans)
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


void cpuMatMulFixedPoint(data_t *A, data_t *B, data_t *R, 
    unsigned int a_rows, unsigned int a_cols, unsigned int b_cols, 
    unsigned int q)
{
    for (unsigned int i = 0; i < a_rows; i++)
    {
        for (unsigned int j = 0; j < b_cols; j++)
        {
            int32_t sum = 0;
            for (unsigned int k = 0; k < a_cols; k++)
            {
                sum += (int32_t) ( (int32_t)A[i * a_cols + k] * (int32_t)B[k * b_cols + j] );
                // int32_t partial = (int32_t)A[i * a_cols + k] * (int32_t)B[k * b_cols + j];
                // printf("partial: %x\n", partial);
            }
            R[i * b_cols + j] = (data_t)(sum >> q);
        }
    }
}



/****************************************************************************/
/**                                 EOF                                    **/
/****************************************************************************/

