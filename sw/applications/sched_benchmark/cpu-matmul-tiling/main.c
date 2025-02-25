// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein Taji
// Date: 22/06/2023
// Description: Main file for running matmul on multi-accels platform


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cgra_bitstream.h"
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

/****************************************************************************/
/**                        DEFINITIONS AND MACROS                          **/
/****************************************************************************/

#define CGRA_COL_INPUT_SIZE 4
#define CHECK_RESULTS
#define DEBUG

/****************************************************************************/
/**                      PROTOTYPES OF LOCAL FUNCTIONS                     **/
/****************************************************************************/

// Intialize system
void init_system(void);

// Handler for the CGRA interruption
static void handler_irq_cgra(uint32_t id);

// Prototype for the CPU matrix multiplication function
void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols);

// Launch cgra matmul
void cgraMatMulRun(uint32_t *A, uint32_t *B, uint32_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols);


/****************************************************************************/
/**                           GLOBAL VARIABLES                             **/
/****************************************************************************/

// Plic controller variables
static volatile bool cgra_intr_flag;

// CGRA variables
static cgra_t     cgra;
static uint8_t    cgra_slot;

// CGRA input buffer
static int32_t cgra_input[CGRA_N_COLS][CGRA_COL_INPUT_SIZE] __attribute__ ((aligned (4)));

// CGRA output buffer
int32_t R_cgra[R_ROWS*R_COLS] __attribute__((section(".xheep_data_interleaved"))); 

// // CPU output buffer
// data_t R_cpu[R_ROWS*R_COLS] __attribute__((section(".xheep_data_interleaved")));

// ram buffers
int32_t A_ram [A_ROWS*A_COLS] __attribute__((section(".xheep_data_interleaved"))) = {0};
int32_t B_ram [B_ROWS*B_COLS] __attribute__((section(".xheep_data_interleaved"))) = {0};

/****************************************************************************/

int main(void)
{

    uint32_t t1, t2, t_cgra, t_cpu;
    
    // Initialization
    init_system();

    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);

    /* CGRA matmul bitstream loading */
    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

    t1 = timer_get_cycles();
    cgraMatMulRun(A_ram, B_ram, R_cgra, A_ROWS, A_COLS, B_COLS);
    while(cgra_intr_flag == 0) { wait_for_interrupt(); }
    t_cgra =  timer_get_cycles() - t1;

    // t1 = timer_get_cycles();
    // cpuMatMul(A_ram, B_ram, R_cpu, A_ROWS, A_COLS, B_COLS);
    // t_cpu = timer_get_cycles() - t1;

#ifdef DEBUG
    PRINTF("CGRA|gold R[0]: %x\n", R_cgra[0]);
    // PRINTF("CPU|gold R[0]: %x\n", R_cpu[0]);
    PRINTF("CGRA|gold R[%d]: %x\n", R_ROWS*R_COLS-1, R_cgra[R_ROWS*R_COLS-1]);
    // PRINTF("CPU|gold R[%d]: %x\n", R_ROWS*R_COLS-1, R_cpu[R_ROWS*R_COLS-1]);
#endif // DEBUG

// #ifdef CHECK_RESULTS
//     // check carus, oe-cgra, and cput results to be the same as the golden result
//     for (unsigned int i = 0; i < R_ROWS; i++) {
//         for (unsigned int j = 0; j < R_COLS; j++) {
//             if (R_cpu[i*R_COLS+j] != R_cgra[i*R_COLS+j]) {
//                 PRINTF("CPU|CGRA R[%u,%u]: %x %x\n", i, j, R_cpu[i*R_COLS+j], R_cgra[i*R_COLS+j]);
//                 return -1;
//             }
//         }
//     }
// #endif // CHECK RESULTS

    PRINTF("CGRA cycles: %u\n", t_cgra);
    // PRINTF("CPU cycles: %u\n", t_cpu);

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


// Interrupt controller variables
static void handler_irq_cgra(uint32_t id) {
  cgra_intr_flag = 1;
}

// Intialize system
void init_system(void){
    if (vcd_init() != 0) return 1;
    timer_cycles_init();
    timer_start();

    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;

    plic_Init();
    if (ext_irq_init() != 0) return 1;

    plic_irq_set_priority(CGRA_INTR, 1);
    plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled);
    plic_assign_external_irq_handler(CGRA_INTR, (void *) &handler_irq_cgra);
    cgra_intr_flag = 0;
    
    // Initialize the DMA
    dma_sdk_init();
    // Pick the correct spi device based on simulation type
    spi_host_t* spi = spi_flash;
    // Init SPI host and SPI<->Flash bridge parameters
    if (w25q128jw_init(spi) != FLASH_OK){
        PRINTF("Error initializing SPI flash\n");
        return 1;
    } 
} 

//cgra Matmul
void cgraMatMulRun(uint32_t *A, uint32_t *B, uint32_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols)
{
    // Configure CGRA registers:
    // Col 0: &B[0][0], nItLoopColsC, &A[0][0], &C[0][3]
    cgra_input[0][0] = (int32_t)&B[0];
    cgra_input[0][1] = B_cols/CGRA_N_ROWS;
    cgra_input[0][2] = (int32_t)&A[0];
    cgra_input[0][3] = (int32_t)&C[3];
    // Col 1: &C[1][0], &B[0][1], nItLoopsColsA, &A[1][0]
    cgra_input[1][0] = (int32_t)&C[B_cols];
    cgra_input[1][1] = (int32_t)&B[1];
    cgra_input[1][2] = A_cols;
    cgra_input[1][3] = (int32_t)&A[A_cols];
    // Col 2: &A[2][0], &C[2][1], &B[0][2], nItLoopColsC
    cgra_input[2][0] = (int32_t)&A[2*A_cols];
    cgra_input[2][1] = (int32_t)&C[2*B_cols+1];
    cgra_input[2][2] = (int32_t)&B[2];
    cgra_input[2][3] = B_cols/CGRA_N_ROWS;
    // Col 3: nItLoopRowsC, &A[3][0], &C[3][2], &B[0][3]
    cgra_input[3][0] = A_rows/CGRA_N_COLS;
    cgra_input[3][1] = (int32_t)&A[3*A_cols];
    cgra_input[3][2] = (int32_t)&C[3*B_cols+2];
    cgra_input[3][3] = (int32_t)&B[3];

    // Set CGRA read pointers for each column
    for(int col_idx = 0; col_idx < CGRA_N_COLS; col_idx++){
        cgra_set_read_ptr(&cgra, cgra_slot, (int32_t)cgra_input[col_idx], col_idx);
    }
    
    /* CGRA Enable fast interrupts */
    CSR_SET_BITS(CSR_REG_MIE, ((1 << 11))); // ((1 << 19) | (1 << 11)) 19: DMA, 11: PLIC
    // Launch CGRA kernel
    cgra_intr_flag = 0;
    cgra_set_kernel(&cgra, cgra_slot, TRANSFORMER);

}


/****************************************************************************/
/**                                 EOF                                    **/
/****************************************************************************/
