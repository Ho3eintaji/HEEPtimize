/*
                              *******************
******************************* C SOURCE FILE *******************************
**                            *******************                          **
**                                                                         **
** project  : HEEPsilon                                                    **
** filename : main.c                                                       **
** version  : 1                                                            **
** date     : 01/10/23                                                     **
**                                                                         **
*****************************************************************************
**                                                                         **
** Copyright (c) EPFL                                                      **
** All rights reserved.                                                    **
**                                                                         **
*****************************************************************************
*/

/***************************************************************************/
/***************************************************************************/

/**
* @file   main.c
* @date   01/10/23
* @brief  An application to run a matrix multiplication.
*
*/

/****************************************************************************/
/**                                                                        **/
/*                             MODULES USED                                 */
/**                                                                        **/
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// #include "transformer.h"
#include "cgra_bitstream.h"
#include "heepatia.h"

// For interrupt handling
#include "csr.h"
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"

#include "timer_util.h"
#include "data.h"

#define PRINT_TIMING_DETAILS


/****************************************************************************/
/**                                                                        **/
/*                        DEFINITIONS AND MACROS                            */
/**                                                                        **/
/****************************************************************************/

// Size of the input buffer for the CGRA
#define CGRA_COL_INPUT_SIZE 4

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

int32_t matrixC[R_ROWS*R_COLS];

#define data_t int32_t // element data type


data_t R_cpu[R_ROWS*R_COLS]; // Result computed by the CPU

// Software matrix multiplication
// NOTE: force alignment on 32-bit boundary to prevent the execution time from
// being affected by the amount of previous compressed instructions. This
// guarantees a stable baseline independent on the previous code.
void __attribute__((noinline, aligned(4))) cpuMatMul(data_t *A, data_t *B, data_t *R_cpu, unsigned int a_rows, unsigned int a_cols, unsigned int b_cols);


/****************************************************************************/
/**                                                                        **/
/*                            LOCAL FUNCTIONS                               */
/**                                                                        **/
/****************************************************************************/

void main()
{
    printf("Multi-Accels Matrix multiplication...\n");

    // fillMatrixInputs();

    uint32_t cpu_cycles = 0;
    uint32_t cgra_cycles = 0;
    uint32_t cgra_init_cycles = 0;
    uint32_t cgra_load_cycles = 0;
    uint32_t cgra_data_move_cycles = 0;
    uint32_t cgra_compute_cycles = 0;
    timer_init();

    // Init the PLIC
    plic_Init();
    plic_irq_set_priority(CGRA_INTR, 1);
    plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled);
    plic_assign_external_irq_handler( CGRA_INTR, (void *) &handler_irq_cgra);

    // Enable interrupt on processor side
    // Enable global interrupt for machine-level interrupts
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    // Set mie.MEIE bit to one to enable machine-level external interrupts
    const uint32_t mask = 1 << 11;//IRQ_EXT_ENABLE_OFFSET;
    CSR_SET_BITS(CSR_REG_MIE, mask);
    cgra_intr_flag = 0;

    timer_start();
    // Load kernel
    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

    // Col 0: &B[0][0], nItLoopColsC, &A[0][0], &C[0][3]
    cgra_input[0][0] = &B[0];
    cgra_input[0][1] = R_COLS/CGRA_N_ROWS;
    cgra_input[0][2] = &A[0];
    cgra_input[0][3] = &matrixC[3];
    // Col 1: &C[1][0], &B[0][1], nItLoopsColsA, &A[1][0]
    cgra_input[1][0] = &matrixC[R_COLS];
    cgra_input[1][1] = &B[1];
    cgra_input[1][2] = A_COLS;
    cgra_input[1][3] = &A[A_COLS];
    // Col 2: &A[2][0], &C[2][1], &B[0][2], nItLoopColsC
    cgra_input[2][0] = &A[2*A_COLS];
    cgra_input[2][1] = &matrixC[2*R_COLS+1];
    cgra_input[2][2] = &B[2];
    cgra_input[2][3] = R_COLS/CGRA_N_ROWS;
    // Col 3: nItLoopRowsC, &A[3][0], &C[3][2], &B[0][3], 
    cgra_input[3][0] = R_ROWS/CGRA_N_COLS;
    cgra_input[3][1] = &A[3*A_COLS];
    cgra_input[3][2] = &matrixC[3*R_COLS+2];
    cgra_input[3][3] = &B[3];

    // Set CGRA kernel L/S pointers
    for(int col_idx = 0 ; col_idx < CGRA_N_COLS ; col_idx++){
      cgra_set_read_ptr ( &cgra, cgra_slot, (uint32_t) cgra_input[col_idx], col_idx );
    }
    cgra_load_cycles = timer_stop();


    // CGRA Execution
    timer_start();
    cgra_intr_flag = 0;
    cgra_set_kernel( &cgra, cgra_slot, TRANSFORMER );
    // Wait until CGRA is done
    while(cgra_intr_flag==0) {
      wait_for_interrupt();
    }
    cgra_compute_cycles = timer_stop();
    cgra_cycles = cgra_init_cycles + cgra_load_cycles + cgra_data_move_cycles + cgra_compute_cycles;
  
    // Software 
    timer_start();
    cpuMatMul(A, B, R_cpu, A_ROWS, A_COLS, B_COLS);
    cpu_cycles = timer_stop();

    checkErrors();

  #ifdef PRINT_TIMING_DETAILS
    // information about application
    printf("========================================\n");
    printf("CGRA matrix multiplication\n");
    printf("========================================\n");

    // printf matrix size
    printf("Matrix size: %u x %u * %u x %u\n", A_ROWS, A_COLS, A_COLS, B_COLS);

    // printf all timing details
    printf("CGRA init cycles: %u\n", cgra_init_cycles);
    printf("CGRA load cycles: %u\n", cgra_load_cycles);
    printf("CGRA data move cycles: %u\n", cgra_data_move_cycles);
    printf("CGRA compute cycles: %u\n", cgra_compute_cycles);
    printf("CGRA total cycles: %u\n", cgra_cycles);
    printf("CPU cycles: %u\n", cpu_cycles);
#endif
  
  return EXIT_SUCCESS;
}

// Check if the SW and CGRA executions give the same result
void checkErrors(){
  int errors = 0;
  for(int i = 0; i < R_ROWS*R_COLS; i++ ){
    if(R_cpu[i]!=matrixC[i]){
      errors++;
    }
  }
  printf("\rErrors: %d\n", errors);

  if(errors>0){
    printMatrix(matrixC, R_ROWS, R_COLS);
  }
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

// Print matrix
void printMatrix(int * matrix, int rows, int cols){
  for(int i = 0; i < rows; i++){
    printf("[ ");
    for(int j=0; j < cols; j++){
      printf("%d ", matrix[i*cols+j]);
    }
    printf("]\n");
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
