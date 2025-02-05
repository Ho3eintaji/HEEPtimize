
// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Michele Caon
// Date: 22/06/2023
// Description: Main file for the matrix multiplication with transposition using DMA application
// Prform A*B = R using transpose operation, by using the property:
// (B^T * A^T)^T = A * B = R
// note: this is useful for Carus' bigger size limits on matrix A than on matrix B in kernel "carus-matmul_fixed.S"

#include <stdlib.h>
#include <stdio.h>

#include "heepatia.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "vcd_util.h"
#include "timer_sdk.h"
#include "ext_irq.h"
#include "carus.h"
#include "carus_matmul_fixed.h"
#include "data2.h"
#include "cgra_bitstream.h"
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"

#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

// #define DEBUG_DMA
#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define DMA_CHANNEL_A 2
#define DMA_CHANNEL_B 3

// CGRA variables
#define CGRA_COL_INPUT_SIZE 4
volatile bool               cgra_intr_flag;
static cgra_t               cgra;
static uint8_t              cgra_slot;
static int32_t cgra_input[CGRA_N_COLS][CGRA_COL_INPUT_SIZE]    __attribute__ ((aligned (4)));

int32_t R_cpu[R_ROWS * R_COLS] __attribute__((section(".xheep_data_interleaved"))); // Result computed by the CPU
data_t R_cpu_fixed[R_ROWS * R_COLS] __attribute__((section(".xheep_data_interleaved"))); 
data_t R_carus[R_ROWS * R_COLS] __attribute__((section(".xheep_data_interleaved"))) = {0}; // Result computed by NM-Carus
int32_t R_cgra[R_ROWS * R_COLS] __attribute__((section(".xheep_data_interleaved"))) = {0}; // Result computed by CGRA
data_t R_cgra_fixed[R_ROWS * R_COLS] __attribute__((section(".xheep_data_interleaved")));

// Software matrix multiplication
// NOTE: force alignment on 32-bit boundary to prevent the execution time from
// being affected by the amount of previous compressed instructions. This
// guarantees a stable baseline independent on the previous code.
void __attribute__((noinline, aligned(4))) cpuMatMulFixedPoint(data_t *A, data_t *B, int32_t *R, unsigned int a_rows, unsigned int a_cols, unsigned int b_cols, 
                         unsigned int q);
// Handler for the CGRA interruption
void handler_irq_cgra(uint32_t id);


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

int main(void)
{
    carus_cfg_t cfg = CARUS_CFG_INIT(0); // NM-Carus configuration
    dma_data_type_t dma_type = DMA_DATA_TYPE_WORD;
    dma_data_type_t dma_type_double;
    data_t *row_ptr;
    data_t_double *row_ptr_double;
    unsigned int a_rows = A_ROWS;
    unsigned int a_cols = A_COLS;
    unsigned int b_cols = B_COLS;
    uint32_t cpu_cycles;
    uint32_t dma_cycles;
    uint32_t carus_cycles;
    uint32_t cgra_cycles;

    // System initialization
    // ---------------------
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e)
        return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1

    // Initialize PLIC for external NM-Carus interrupt
    plic_Init();
    // carus
    if (ext_irq_init() != 0)
        return 1;
    // oe-cgra
    plic_irq_set_priority(CGRA_INTR, 1);
    plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled);
    plic_assign_external_irq_handler( CGRA_INTR, (void *) &handler_irq_cgra);
    cgra_intr_flag = 0;

    // Initialize the DMA
    dma_sdk_init();

    // Initialize the VCD trigger
    if (vcd_init() != 0) return 1;

    // --------------------------------
    // --- Carus ---
    // --------------------------------

    // Initialize NM-Carus
    // ------------------
    // Initialize NM-Carus
    if (carus_init(CARUS_INSTANCE) != 0)
        return 1;

    // Load kernel
    if (carus_load_kernel(CARUS_INSTANCE, carus_matmul_fixed, CARUS_MATMUL_FIXED_SIZE, 0) != 0){
        printf("Error loading kernel\n");
        return 1;
    }

    // Set kernel configuration configuration
    cfg.vl = VL;
    switch (ELEM_SIZE)
    {
    case 1:
        cfg.vtype = VTYPE_VSEW_16;
        dma_type = DMA_DATA_TYPE_BYTE;
        dma_type_double = DMA_DATA_TYPE_HALF_WORD;
        break;
    case 2:
        cfg.vtype = VTYPE_VSEW_32;
        dma_type = DMA_DATA_TYPE_HALF_WORD;
        dma_type_double = DMA_DATA_TYPE_WORD;
        break;
    default:
        printf("Error: unsupported element size\n");
        return 1;
    }

    cfg.arg0 = ARG0; // n. rows of A
    cfg.arg1 = ARG1; // n. columns of A

    // Write kernel configuration
    if (carus_set_cfg(CARUS_INSTANCE, &cfg) != 0){
        printf("Error configuring kernel\n");
        return 1;
    }

    // Transfer data to NM-Carus
    // -------------------------
    // Copy flattened transposed matrix B
    row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_A_VREG);
    dma_target_t tgt_src_B = {
        .ptr = (uint8_t *) B,
        .inc_d1_du = 1,
        .inc_d2_du = B_COLS,
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
        .size_d1_du = B_ROWS,
        .size_d2_du = B_COLS,
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
        .inc_d2_du = A_COLS,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_A = {
        .ptr = (uint8_t *) &row_ptr[0],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/(2*ELEM_SIZE)) - A_ROWS +1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_A = {
        .src = &tgt_src_A,
        .dst = &tgt_dst_A,
        .size_d1_du = A_ROWS ,
        .size_d2_du = A_COLS,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .dim_inv = 1, // This is the transposition flag!
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR_WAIT, // block until the transaction is completed
        .channel = DMA_CHANNEL_A,
    };

    timer_cycles_init();
    timer_start();

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

    dma_cycles = timer_stop();

    #ifdef DEBUG_DMA
        // DMA check
        row_ptr_double = (data_t_double *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_A_VREG);
        printf("B^T\n");
        for (int i = 0; i < B_COLS; i++)
        {
            for(int j = 0; j < B_ROWS; j++){
                printf("%x ", row_ptr_double[i*B_ROWS + j]);
            }
            printf("\n");
        }
        printf("\nA^T\n");
        for(int i=CARUS_MATMUL_B_VREG; i<CARUS_MATMUL_B_VREG+A_COLS; i++){
            row_ptr_double = (data_t_double *)carus_vrf(CARUS_INSTANCE, i);
            for (int j = 0; j < A_ROWS; j++)
            {
                printf("%x ", row_ptr_double[j]);
            }
            printf("\n");
        }
    #endif

    timer_cycles_init();
    timer_start();
    // Enable VCD dump
    vcd_enable();

    // Run the kernel
    // --------------
    // Run the kernel
    if (carus_run_kernel(CARUS_INSTANCE) != 0){
        printf("Error running kernel\n");
        return 1;
    }
    // Wait for the kernel to complete
    if (carus_wait_done(CARUS_INSTANCE) != 0) return 1;
        
    // Disable VCD dump
    vcd_disable();
    carus_cycles = timer_stop();

    // --------------------------------
    // --- CPU ---
    // --------------------------------

    // Compute the matmul on the CPU
    // -----------------------------
    // // Enable VCD dump and counter
    // vcd_enable();
    timer_cycles_init();
    timer_start();
    
    // Compute result on the CPU
    cpuMatMulFixedPoint(A, B, R_cpu, a_rows, a_cols, b_cols, Q);

    // Apply manual shifting (>> q) on the CGRA result
    for (unsigned int i = 0; i < a_rows; i++) {
        for (unsigned int j = 0; j < b_cols; j++) {
            R_cpu_fixed[i * b_cols + j] = (data_t)(R_cpu[i * b_cols + j] >> Q);
        }
    }

    

    // Stop timer and disable VCD dump
    cpu_cycles = timer_stop();
    // vcd_disable();

    // --------------------------------
    // --- OE-CGRA ---
    // --------------------------------
    // oecgra, load kernel
    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

    // Col 0: &B[0][0], nItLoopColsC, &A[0][0], &C[0][3]
    cgra_input[0][0] = &B[0];
    cgra_input[0][1] = R_COLS/CGRA_N_ROWS;
    cgra_input[0][2] = &A[0];
    cgra_input[0][3] = &R_cgra[3];
    // Col 1: &C[1][0], &B[0][1], nItLoopsColsA, &A[1][0]
    cgra_input[1][0] = &R_cgra[R_COLS];
    cgra_input[1][1] = &B[1];
    cgra_input[1][2] = A_COLS;
    cgra_input[1][3] = &A[A_COLS];
    // Col 2: &A[2][0], &C[2][1], &B[0][2], nItLoopColsC
    cgra_input[2][0] = &A[2*A_COLS];
    cgra_input[2][1] = &R_cgra[2*R_COLS+1];
    cgra_input[2][2] = &B[2];
    cgra_input[2][3] = R_COLS/CGRA_N_ROWS;
    // Col 3: nItLoopRowsC, &A[3][0], &C[3][2], &B[0][3], 
    cgra_input[3][0] = R_ROWS/CGRA_N_COLS;
    cgra_input[3][1] = &A[3*A_COLS];
    cgra_input[3][2] = &R_cgra[3*R_COLS+2];
    cgra_input[3][3] = &B[3];

    // Set CGRA kernel L/S pointers
    for(int col_idx = 0 ; col_idx < CGRA_N_COLS ; col_idx++){
      cgra_set_read_ptr ( &cgra, cgra_slot, (uint32_t) cgra_input[col_idx], col_idx );
    }

    timer_cycles_init();
    timer_start();

    cgra_intr_flag = 0;
    cgra_set_kernel( &cgra, cgra_slot, TRANSFORMER );
    // Wait until CGRA is done
    while(cgra_intr_flag==0) {
      wait_for_interrupt();
    }

    // // Apply manual shifting (>> q) on the CGRA result
    // for (unsigned int i = 0; i < a_rows; i++) {
    //     for (unsigned int j = 0; j < b_cols; j++) {
    //         // R_cgra[i * b_cols + j] >>= Q;
    //         R_cgra_fixed[i * b_cols + j] = (data_t)(R_cgra[i * b_cols + j] >> Q);
    //     }
    // }

    cgra_cycles = timer_stop();

    // Skip verification part when running power simulation
#ifdef POWER_SIM
    return 0;
#endif

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
        .ptr = (uint8_t *) R_carus,
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

    timer_cycles_init();
    timer_start();

    if (run_dma_2d_transp_trans(&trans_R) != 0)
    {
        printf("Error! DMA R transaction failed\n");
        return 1;
    }

    DMA_WAIT(DMA_CHANNEL_A)

    dma_cycles += timer_stop();

    #ifdef DEBUG_DMA
        printf("\nTransposed output in Carus' vector register file\n");
        for(int i=CARUS_MATMUL_R_VREG; i<R_COLS+CARUS_MATMUL_R_VREG; i++){
            row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, i);
            for (int j = 0; j < R_ROWS; j++)
            {
                printf("%x ", row_ptr[2*j]);
            }
            printf("\n");
        }
        printf("\nCopied R_carus:\n");
        for(int i=0; i<R_ROWS; i++){
            for(int j=0; j<R_COLS; j++){
                printf("%x ", R_carus[i*R_COLS + j]);
            }
            printf("\n");
        }
    #endif

    

    // Print the number of CPU cycles
    printf("CPU: %u\n", cpu_cycles);
    printf("Carus: %u\n", carus_cycles);
    printf("DMA copy to&from carus: %u\n", dma_cycles);
    printf("CGRA: %u\n", cgra_cycles);

    // Check the output data
    // ---------------------
    // Check NM-Carus output data
    for (unsigned int i = 0; i < R_ROWS; i++)
    {
        row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, CARUS_MATMUL_R_VREG + i);
        for (unsigned int j = 0; j < R_COLS; j++)
        {
            // if (R_carus[i * R_COLS + j] != R[i * R_COLS + j])
            // {
            //     printf("NMC|gold R[%u,%u]: %x %x\n", i, j, R_carus[i * R_COLS + j], R[i * R_COLS + j]);
            //     return 1;
            // }
            // if (R_cpu_fixed[i * R_COLS + j] != R[i * R_COLS + j])
            // {
            //     printf("CPU|gold R[%u,%u]: %x %x\n", i, j, R_cpu_fixed[i * R_COLS + j], R[i * R_COLS + j]);
            //     // return 1;
            // }
            // if (R_cgra_fixed[i * R_COLS + j] != R[i * R_COLS + j])
            // {
            //     printf("CGRA|gold R[%u,%u]: %x %x\n", i, j, R_cgra_fixed[i * R_COLS + j], R[i * R_COLS + j]);
            //     // return 1;
            // }
            // if (R_cgra[i * R_COLS + j] != R[i * R_COLS + j])
            // {
            //     printf("CGRA|gold R[%u,%u]: %x %x\n", i, j, R_cgra[i * R_COLS + j], R[i * R_COLS + j]);
            //     // return 1;
            // }
            if (R_cpu[i * R_COLS + j] != R_cgra[i * R_COLS + j])
            {
                printf("int32: CPU|CGRA R[%u,%u]: %x %x\n", i, j, R_cpu[i * R_COLS + j], R_cgra[i * R_COLS + j]);
                // return 1;
            }
        }
    }

    // Return success
    return 0;
}

void cpuMatMulFixedPoint(data_t *A, data_t *B, int32_t *R, 
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
            // R[i * b_cols + j] = (data_t)(sum >> q);
            R[i * b_cols + j] = (int32_t)(sum);
        }
    }
}

// Interrupt controller variables
void handler_irq_cgra(uint32_t id) {
  cgra_intr_flag = 1;
}