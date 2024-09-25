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

// For interrupt handling
#include "csr.h"
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"
#include "fast_intr_ctrl.h"
#include "dma_util.h"
#include "vcd_util.h"
#include "ext_irq.h"
#include "carus.h"
#include "carus_matmul.h"
#include "caesar.h"
#include "timer_util.h"

#include "data.h"
#include "carus_data.h"

#include "caesar_commands.h"
#include "caesar_data.h"

/****************************************************************************/
/**                                                                        **/
/*                        DEFINITIONS AND MACROS                            */
/**                                                                        **/
/****************************************************************************/

// #define CHECK_RESULTS
#define VCD
// #define PRINT_TIMING_DETAILS
#define CGRA_COL_INPUT_SIZE 4

#ifdef PRINT_TIMING_DETAILS
    #define TIMER_ENABLED
#endif

/****************************************************************************/
/**                                                                        **/
/*                      PROTOTYPES OF LOCAL FUNCTIONS                       */
/**                                                                        **/
/****************************************************************************/

// Handler for the CGRA interruption
void handler_irq_cgra(uint32_t id);
// Print a matrix
void printMatrix(int * matrix, int rows, int cols);

/****************************************************************************/
/**                                                                        **/
/*                            GLOBAL VARIABLES                              */
/**                                                                        **/
/****************************************************************************/

// Plic controller variables
volatile bool               cgra_intr_flag;

// CGRA variables
static cgra_t               cgra;
static uint8_t              cgra_slot;

// CGRA input and output buffers
static int32_t cgra_input[CGRA_N_COLS][CGRA_COL_INPUT_SIZE]    __attribute__ ((aligned (4)));

int32_t R_cgra[R_ROWS*R_COLS];

data_t R_cpu[R_ROWS*R_COLS]; // Result computed by the CPU

/****************************************************************************/
/**                                                                        **/
/*                            LOCAL FUNCTIONS                               */
/**                                                                        **/
/****************************************************************************/

// Software matrix multiplication
// NOTE: force alignment on 32-bit boundary to prevent the execution time from
// being affected by the amount of previous compressed instructions. This
// guarantees a stable baseline independent on the previous code.
void __attribute__((noinline, aligned(4))) cpuMatMul(data_t *A, data_t *B, data_t *R_cpu, unsigned int a_rows, unsigned int a_cols, unsigned int b_cols);


void main()
{
#ifdef PRINT_TIMING_DETAILS
    printf("========================================\n");
    printf("Multi-Accelerators Matrix Multiplication\n");
    printf("========================================\n");
#endif

#ifdef VCD
    if (vcd_init() != 0) return 1;
#endif

    carus_cfg_t cfg = CARUS_CFG_INIT; // NM-Carus configuration
    dma_data_type_t dma_type = DMA_DATA_TYPE_WORD;
    data_t *row_ptr;

    unsigned int a_rows = A_ROWS;
    unsigned int a_cols = A_COLS;
    unsigned int b_cols = B_COLS;

    uint32_t cpu_cycles = 0;
    uint32_t carus_cycles = 0;
    uint32_t carus_init_cycles = 0;
    uint32_t carus_load_cycles = 0;
    uint32_t carus_data_move_cycles = 0;
    uint32_t carus_compute_cycles = 0;
    uint32_t cgra_cycles = 0;
    uint32_t cgra_init_cycles = 0;
    uint32_t cgra_load_cycles = 0;
    uint32_t cgra_data_move_cycles = 0;
    uint32_t cgra_compute_cycles = 0;
    uint32_t caesar_cycles = 0;
    uint32_t caesar_init_cycles = 0;
    uint32_t caesar_load_cycles = 0;
    uint32_t caesar_data_move_cycles = 0;
    uint32_t caesar_compute_cycles = 0;

#ifdef TIMER_ENABLED
    timer_init();
#endif

    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e)
        return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask); // MIE.meie = 1

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
    dma_init(NULL);

    // --------------------------------
    // --- NM-Carus ---
    // -------------------------------- 
    // carus, initialize
#ifdef TIMER_ENABLED
    timer_start();
#endif
    if (carus_init(0) != 0) return 1;
#ifdef TIMER_ENABLED
    carus_init_cycles = timer_stop();
#endif

    // carus, load kernel
#ifdef TIMER_ENABLED
    timer_start();
#endif
    // Load kernel
    if (carus_load_kernel(0, carus_matmul, CARUS_MATMUL_SIZE, NULL) != 0) return 1;
    // Set kernel configuration configuration
    cfg.vl = VL;
    switch (ELEM_SIZE)
    {
    case 1: 
        cfg.vtype = VTYPE_VSEW_8; 
        dma_type = DMA_DATA_TYPE_BYTE;
        break;
    case 2: 
        cfg.vtype = VTYPE_VSEW_16; 
        dma_type = DMA_DATA_TYPE_HALF_WORD;
        break;
    case 4: 
        cfg.vtype = VTYPE_VSEW_32; 
        dma_type = DMA_DATA_TYPE_WORD;
        break;
    default: return 1;
    }
    cfg.arg0 = ARG0; // n. rows of A
    cfg.arg1 = ARG1; // n. columns of A
    // Write kernel configuration
    if (carus_set_cfg(0, &cfg) != 0)
        return 1;
#ifdef TIMER_ENABLED
    carus_load_cycles = timer_stop();
#endif

    // carus, data transfer
#ifdef TIMER_ENABLED
    timer_start();
#endif
#ifdef VCD
    vcd_enable();
#endif
    // Copy flattened matrix A
    row_ptr = (data_t *) (CARUS0_START_ADDRESS + vregs[CARUS_MATMUL_A_VREG]);
    if (dma_copy((uint8_t *) row_ptr, (uint8_t *) A, A_SIZE, dma_type) != 0) return 1;
    // Copy matrix B
    for (unsigned int i = 0; i < B_ROWS; i++) {
        row_ptr = CARUS0_START_ADDRESS + vregs[CARUS_MATMUL_B_VREG + i];
        if (dma_copy((uint8_t *) row_ptr, (uint8_t *) (B+i*B_COLS), B_COLS * ELEM_SIZE, dma_type) != 0)
            return 1;
    }
#ifdef TIMER_ENABLED
    carus_data_move_cycles = timer_stop();
#endif

    // carus, running the kernel
#ifdef TIMER_ENABLED
    timer_start();
#endif
    // Run the kernel
    if (carus_run_kernel(0) != 0) return 1;
    // Wait for the kernel to complete
    if (carus_wait_done(0) != 0) return 1;
#ifdef VCD
    vcd_disable();
#endif
#ifdef TIMER_ENABLED
    carus_compute_cycles = timer_stop();
#endif
#ifdef TIMER_ENABLED
    carus_cycles = carus_load_cycles + carus_data_move_cycles + carus_compute_cycles;
#endif
    // --------------------------------
    // --- NM-Caesar ---
    // --------------------------------
    // uint32_t* ptr_caesar_cmd;

    // correcting the destination addresses
    //the destination address are generated relatively to the memory, thus adding its system offset
    for(int i=0; i < CAESAR_CMDS_MATMUL_ADDR_SIZE >> 2; i++){
        caesar_cmds_matmul_addr[i] += CAESAR0_START_ADDRESS;
    }

    // caesar, data transfer
#ifdef TIMER_ENABLED
    timer_start();
#endif
#ifdef VCD
    vcd_enable();
#endif
    data_t* matrix_caesar_A = (data_t*) (CAESAR0_START_ADDRESS + CAESAR_A_OFFS);
    data_t* matrix_caesar_B = (data_t*) (CAESAR0_START_ADDRESS + CAESAR_B_OFFS);
    if (dma_copy((uint8_t *) matrix_caesar_A, (uint8_t *) A, A_SIZE, dma_type) != 0) return -1;
    if (dma_copy((uint8_t *) matrix_caesar_B, (uint8_t *) B, B_SIZE, dma_type) != 0) return -1;
#ifdef TIMER_ENABLED
    caesar_data_move_cycles = timer_stop();
#endif

    // caesar, running the kernel
#ifdef TIMER_ENABLED
    timer_start();
#endif
    // Set NM-Caesar in computing mode
    if (caesar_set_mode(0, CAESAR_MODE_COMP) != 0) return -1;
    //Use the DMA to send commands - performing matmul M * A
    dma_copy_to_addr_32b(caesar_cmds_matmul_addr, caesar_cmds_matmul, CAESAR_CMDS_MATMUL_SIZE >> 2);
#ifdef TIMER_ENABLED
    caesar_compute_cycles = timer_stop();
    caesar_cycles  =  caesar_load_cycles + caesar_data_move_cycles + caesar_compute_cycles;
#endif
    // Set NM-Caesar in memory mode
    if (caesar_set_mode(0, CAESAR_MODE_MEM) != 0) return -1;
#ifdef VCD
    vcd_disable();
#endif

    //set the pointer back
    uint32_t* R_caesar = (uint32_t*)(CAESAR0_START_ADDRESS + CAESAR_R_OFFS);


    // --------------------------------
    // --- OE-CGRA ---
    // --------------------------------
    // oecgra, load kernel
#ifdef TIMER_ENABLED
    timer_start();
#endif

    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

#ifdef VCD
    vcd_enable();
#endif

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
#ifdef TIMER_ENABLED
    cgra_load_cycles = timer_stop();
#endif

    // oe-cgra, running the kernel
#ifdef TIMER_ENABLED
    timer_start();
#endif
    cgra_intr_flag = 0;
    cgra_set_kernel( &cgra, cgra_slot, TRANSFORMER );
    // Wait until CGRA is done
    while(cgra_intr_flag==0) {
      wait_for_interrupt();
    }
#ifdef TIMER_ENABLED
    cgra_compute_cycles = timer_stop();
    cgra_cycles = cgra_load_cycles + cgra_data_move_cycles + cgra_compute_cycles;
#endif
#ifdef VCD
    vcd_disable();
#endif
  
    // --------------------------------
    // --- CPU ---
    // --------------------------------
#ifdef VCD
    vcd_enable();
#endif
#ifdef TIMER_ENABLED
    timer_start();
#endif
    cpuMatMul(A, B, R_cpu, A_ROWS, A_COLS, B_COLS);
#ifdef TIMER_ENABLED
    cpu_cycles = timer_stop();
#endif
#ifdef VCD
    vcd_disable();
#endif

#ifdef CHECK_RESULTS
    // check carus, oe-cgra, and cput results to be the same as the golden result
    for (unsigned int i = 0; i < R_ROWS; i++) {
        row_ptr = (data_t *) (CARUS0_START_ADDRESS + vregs[CARUS_MATMUL_R_VREG + i]);
        for (unsigned int j = 0; j < R_COLS; j++) {
            if (row_ptr[j] != R[i*R_COLS+j]) {
                printf("NM-Carus|gold R[%u,%u]: %x %x\n", i, j, row_ptr[j], R[i*R_COLS+j]);
                return 1;
            }
            if (R_cgra[i*R_COLS+j] != R[i*R_COLS+j]) {
                printf("CGRA|gold R[%u,%u]: %x %x\n", i, j, R_cgra[i*R_COLS+j], R[i*R_COLS+j]);
                return 1;
            }
            if (R_cpu[i*R_COLS+j] != R[i*R_COLS+j]) {
                printf("CPU|gold R[%u,%u]: %x %x\n", i, j, R_cpu[i*R_COLS+j], R[i*R_COLS+j]);
                return 1;
            }
            if (R_caesar[i*R_COLS+j] != R[i*R_COLS+j]) {
                printf("NM-Caesar|gold R[%u,%u]: %x %x\n", i, j, R_caesar[i*R_COLS+j], R[i*R_COLS+j]);
                return -1;
            }
        }
    }
    printf("Results are correct\n");
#endif

  #ifdef PRINT_TIMING_DETAILS
    printf("----------------------------------------\n");
    printf("Matrix size: %u x %u * %u x %u\n", A_ROWS, A_COLS, A_COLS, B_COLS);
    printf("----------------------------------------\n");
    // Then details of carus
    printf("NM-Carus\n");
    printf("----------------------------------------\n");
    printf("Initialization cycles: %u\n", carus_init_cycles);
    printf("Load kernel cycles: %u\n", carus_load_cycles);
    printf("Data move cycles: %u\n", carus_data_move_cycles);
    printf("Compute cycles: %u\n", carus_compute_cycles);
    printf("Total cycles: %u (load+mv+exe)\n", carus_cycles);
    printf("----------------------------------------\n");
    // Then details of caesar
    printf("NM-Caesar\n");
    printf("----------------------------------------\n");
    printf("Initialization cycles: %u\n", caesar_init_cycles);
    printf("Load kernel cycles: %u\n", caesar_load_cycles);
    printf("Data move cycles: %u\n", caesar_data_move_cycles);
    printf("Compute cycles: %u\n", caesar_compute_cycles);
    printf("Total cycles: %u (load+mv+exe)\n", caesar_cycles);
    printf("----------------------------------------\n");
    // Then details of oe-cgra
    printf("OE-CGRA\n");
    printf("----------------------------------------\n");
    printf("Initialization cycles: %u\n", cgra_init_cycles);
    printf("Load kernel cycles: %u\n", cgra_load_cycles);
    printf("Data move cycles: %u\n", cgra_data_move_cycles);
    printf("Compute cycles: %u\n", cgra_compute_cycles);
    printf("Total cycles: %u (load+mv+exe)\n", cgra_cycles);
    printf("----------------------------------------\n");
    // Finally details of cpu
    printf("CPU\n");
    printf("----------------------------------------\n");
    printf("Compute cycles: %u\n", cpu_cycles);
    printf("----------------------------------------\n"); 
#endif
  
  return EXIT_SUCCESS;
}

void cpuMatMul(data_t *A, data_t *B, data_t *R_cpu, unsigned int a_rows, unsigned int a_cols, unsigned int b_cols)
{
    for (unsigned int i = 0; i < a_rows; i++) {
        for (unsigned int j = 0; j < b_cols; j++) {
            R_cpu[i*b_cols+j] = 0;
            for (unsigned int k = 0; k < a_cols; k++) {
                R_cpu[i*b_cols+j] += A[i*a_cols+k] * B[k*b_cols+j];
            }
        }
    }
}

// Interrupt controller variables
void handler_irq_cgra(uint32_t id) {
  cgra_intr_flag = 1;
}



/****************************************************************************/
/**                                                                        **/
/*                                 EOF                                      */
/**                                                                        **/
/****************************************************************************/
