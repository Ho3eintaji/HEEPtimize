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

#include "transformer.h"
#include "cgra_bitstream.h"
#include "heepatia.h"
#include "performance.h"

// For interrupt handling
#include "csr.h"
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"

// #include "timer_util.h"
#include "timer_sdk.h"
#include "fast_intr_ctrl.h"
#include "ext_irq.h"
#include "dma_sdk.h"
#include "vcd_util.h"

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

// Matrix multiplication using the standard three loops
void mmulSoftware(int32_t * output);
// Fill input matrixes with numbers
void fillMatrixInputs();
// Handler for the CGRA interruption
void handler_irq_cgra(uint32_t id);
// Record the cycle number at the start
void kcom_perfRecordStart( kcom_time_diff_t *perf );
// Record the cycle number and compute the total cycles
void kcom_perfRecordStop( kcom_time_diff_t *perf );
// Show the performance metrics
void showPerformance( kcom_perf_t* kperf, int full);
// Print a matrix
void printMatrix(int * matrix, int rows, int cols);

/****************************************************************************/
/**                                                                        **/
/*                            GLOBAL VARIABLES                              */
/**                                                                        **/
/****************************************************************************/

// Performance variable
static kcom_perf_t  kperf;

// Plic controller variables
volatile bool               cgra_intr_flag;

// CGRA variables
static cgra_t               cgra;
static uint8_t              cgra_slot;

// CGRA input and output buffers
static int32_t cgra_input[CGRA_N_COLS][CGRA_COL_INPUT_SIZE]    __attribute__ ((aligned (4)));

// Input and output matrixes
int32_t matrixA[ROWS_A*COLS_A]  __attribute__((section(".xheep_data_interleaved")));
int32_t matrixB[ROWS_B*COLS_B]  __attribute__((section(".xheep_data_interleaved")));
int32_t matrixC[ROWS_C*COLS_C]  __attribute__((section(".xheep_data_interleaved")));
int32_t outSW[ROWS_C*COLS_C]  __attribute__((section(".xheep_data_interleaved")));

#define data_t int32_t // element data type


data_t R_cpu[ROWS_C*COLS_C] __attribute__((section(".xheep_data_interleaved"))); // Result computed by the CPU

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
  fillMatrixInputs();

  uint32_t cpu_cycles = 0;
  uint32_t cgra_cycles = 0;
  uint32_t cgra_init_cycles = 0;
  uint32_t cgra_load_cycles = 0;
  uint32_t cgra_data_move_cycles = 0;
  uint32_t cgra_compute_cycles = 0;
  timer_cycles_init();

  // Enable and reset the CGRA performance counters
  cgra_perf_cnt_enable(&cgra, 1);
  cgra_perf_cnt_reset( &cgra );

  // if(ROWS_C < CGRA_N_COLS || COLS_C < CGRA_N_ROWS){
  //   kcom_perfRecordStart(&(kperf.time.sw));
  //   mmulSoftware(matrixC);
  //   kcom_perfRecordStop(&(kperf.time.sw));
  // } else {

    // Initialize the CGRA
    // kcom_perfRecordStart(&(kperf.time.load));

    // ===== Initialize the CGRA =====
    // initCGRA();

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
    // Select request slot of CGRA
    cgra_slot = cgra_get_slot(&cgra);

    // kcom_perfRecordStop(&(kperf.time.load));

    // Prepare the input vector for the CGRA
    // kcom_perfRecordStart(&(kperf.time.input));

    // Col 0: &B[0][0], nItLoopColsC, &A[0][0], &C[0][3]
    cgra_input[0][0] = &matrixB[0];
    cgra_input[0][1] = COLS_C/CGRA_N_ROWS;
    cgra_input[0][2] = &matrixA[0];
    cgra_input[0][3] = &matrixC[3];
    // Col 1: &C[1][0], &B[0][1], nItLoopsColsA, &A[1][0]
    cgra_input[1][0] = &matrixC[COLS_C];
    cgra_input[1][1] = &matrixB[1];
    cgra_input[1][2] = COLS_A;
    cgra_input[1][3] = &matrixA[COLS_A];
    // Col 2: &A[2][0], &C[2][1], &B[0][2], nItLoopColsC
    cgra_input[2][0] = &matrixA[2*COLS_A];
    cgra_input[2][1] = &matrixC[2*COLS_C+1];
    cgra_input[2][2] = &matrixB[2];
    cgra_input[2][3] = COLS_C/CGRA_N_ROWS;
    // Col 3: nItLoopRowsC, &A[3][0], &C[3][2], &B[0][3], 
    cgra_input[3][0] = ROWS_C/CGRA_N_COLS;
    cgra_input[3][1] = &matrixA[3*COLS_A];
    cgra_input[3][2] = &matrixC[3*COLS_C+2];
    cgra_input[3][3] = &matrixB[3];
    // kcom_perfRecordStop(&(kperf.time.input));

    // Set CGRA kernel L/S pointers
    // kcom_perfRecordStart( &(kperf.time.reprogramCols) );
    for(int col_idx = 0 ; col_idx < CGRA_N_COLS ; col_idx++){
      cgra_set_read_ptr ( &cgra, cgra_slot, (uint32_t) cgra_input[col_idx], col_idx );
    }
    // kcom_perfRecordStop( &(kperf.time.reprogramCols) );
    cgra_load_cycles = timer_stop();


    // CGRA Execution
    // kcom_perfRecordStart(   &(kperf.time.cgra) );
    timer_start();
    vcd_init();
    vcd_enable();
    cgra_intr_flag = 0;
    cgra_set_kernel( &cgra, cgra_slot, TRANSFORMER );
    // Wait until CGRA is done
    while(cgra_intr_flag==0) {
      wait_for_interrupt();
    }
    vcd_disable();
    // kcom_perfRecordStop(   &(kperf.time.cgra) );
    cgra_compute_cycles = timer_stop();
    cgra_cycles = cgra_init_cycles + cgra_load_cycles + cgra_data_move_cycles + cgra_compute_cycles;
  
    // Software 
    // kcom_perfRecordStart(&(kperf.time.sw));
    // timer_start();
    mmulSoftware(outSW);
    // cpu_cycles = timer_stop();
    timer_start();
    // Compute result on the CPU
    // cpuMatMul(A, B, R_cpu, a_rows, a_cols, b_cols);
    cpuMatMul(matrixA, matrixB, R_cpu, ROWS_A, COLS_A, COLS_B);
    // Stop timer and disable VCD dump
    cpu_cycles = timer_stop();
    // kcom_perfRecordStop(&(kperf.time.sw));

    checkErrors();
  // }

  // showPerformance(&kperf, 0);

  #ifdef PRINT_TIMING_DETAILS
    // information about application
    // printf("========================================\n");
    // printf("CGRA matrix multiplication\n");
    // printf("========================================\n");

    // // printf matrix size
    // printf("Matrix size: %u x %u * %u x %u\n", ROWS_A, COLS_A, COLS_A, COLS_B);

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
  for(int i = 0; i < ROWS_C*COLS_C; i++ ){
    if(outSW[i]!=matrixC[i]){
      errors++;
    }
  }
  printf("\rErrors: %d\n", errors);

  if(errors>0){
    printMatrix(matrixC, ROWS_C, COLS_C);
  }
}

// Initialize the CGRA
void initCGRA(){
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

  // Load kernel
  cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);

  cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
  // Select request slot of CGRA
  cgra_slot = cgra_get_slot(&cgra);
}

// Fill matrix inputs
void fillMatrixInputs(){
  for(int i = 0; i < ROWS_A; i++){
    for(int j=0; j < COLS_A; j++){
      matrixA[i*COLS_A+j] = (i*COLS_A+j+1)%100;
    }
  }

  for(int i = 0; i < ROWS_B; i++){
    for(int j=0;j < COLS_B; j++){
      matrixB[i*COLS_B+j] = (i*COLS_B+j+1)%100;
    }
  }
}

// Software matrix multiplication
void mmulSoftware(int32_t * out){
  for(int i = 0; i < ROWS_A; i++){
    for(int j=0;j < COLS_B; j++){
       out[i*COLS_C+j] = 0;
      for(int k=0; k < COLS_A; k++){
        out[i*COLS_C+j] += matrixA[i*COLS_A+k]*matrixB[k*COLS_B+j];
      }
    }
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

// Display the performance values
void showPerformance( kcom_perf_t* kperf, int full){
  printf("\rA:%dx%d, B:%dx%d\n", ROWS_A, COLS_A, ROWS_B, COLS_B);
  printf("\rSw: %d\n", kperf->time.sw.spent_cy);
  if (full){
    printf("\rLoad: %d\n", kperf->time.load.spent_cy);
    printf("\rProgram cols: %d\n", kperf->time.reprogramCols.spent_cy);
    printf("\rInput: %d\n", kperf->time.input.spent_cy);
    printf("\rCgra: %d\n", kperf->time.cgra.spent_cy);
  }
  int32_t overhead = kperf->time.input.spent_cy + kperf->time.reprogramCols.spent_cy + kperf->time.load.spent_cy;
  //printf("\rOverhead: %d\n", overhead);
  printf("\rTotal cgra: %d\n", overhead + kperf->time.cgra.spent_cy); 
}



/****************************************************************************/
/**                                                                        **/
/*                                 EOF                                      */
/**                                                                        **/
/****************************************************************************/
