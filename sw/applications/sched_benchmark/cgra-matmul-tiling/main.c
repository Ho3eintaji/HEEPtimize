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
// #define DEBUG
#define CGRA_BUFFER_SIZE (32 * 1024 / sizeof(int32_t))

/****************************************************************************/
/**                      PROTOTYPES OF LOCAL FUNCTIONS                     **/
/****************************************************************************/

typedef struct {
    uint32_t t_prc;
    uint32_t t_flash;
    uint32_t t_dma_to;
    uint32_t t_dma_from;
    uint32_t n_dms;
    uint32_t t_tot;
    uint32_t t_tmp1;
    uint32_t t_tmp2;
} timings_t;

// Handler for the CGRA interruption
static void handler_irq_cgra(uint32_t id);

// Launch cgra matmul
void cgraMatMulRun(uint32_t *A, uint32_t *B, uint32_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols);

// CGRA matmul tiled
void cgraMatMulTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, int32_t *cgra_buffer, uint32_t cgra_buffer_size, timings_t * timing);

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

// cache: whole data is cached
int32_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS] = {1};
int32_t *A_ram = cache;
int32_t *B_ram = cache + A_ROWS*A_COLS;
int32_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;

// CGRA buffer
// int32_t cgra_buffer[] __attribute__((aligned(4)));
int32_t cgra_buffer[CGRA_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};

/****************************************************************************/

int main(void)
{

    uint32_t t1, t2, t_cgra, t_cpu;
    
    /* ============================== 
    * ====== Initialization =========
    * ============================== */

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

    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;

    plic_Init();
    if (ext_irq_init() != 0) return 1;

    plic_irq_set_priority(CGRA_INTR, 1);
    plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled);
    plic_assign_external_irq_handler(CGRA_INTR, (void *) &handler_irq_cgra);
    cgra_intr_flag = 0;

    /* CGRA matmul bitstream loading */
    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

    // intialize the timings recordings and allocate to zero
    timings_t * timing_cgra = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_cgra, 0, sizeof(timings_t));

    // /* ==============================
    // * ====== Putting data in cache ======
    // * ============================== */
    // // move data from A and B which are in flash to A_ram and B_ram which are in ram
    // timing_cgra->t_tmp1 = timer_get_cycles();
    // if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    // w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    // if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    // w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    // timing_cgra->t_flash = timer_get_cycles() - timing_cgra->t_tmp1;

    // /* =======================================
    // * ====== Runing on CGRA ======
    // * ======================================== */
    // t1 = timer_get_cycles();
    // cgraMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cgra_buffer, CGRA_BUFFER_SIZE, timing_cgra);
    // timing_cgra->t_tot = timer_get_cycles() - t1;


    // PRINTF("size: %dx%dx%d, CGRA: flash: %d, total: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d\n", A_ROWS, A_COLS, B_COLS, timing_cgra->t_flash, timing_cgra->t_tot, timing_cgra->t_prc, timing_cgra->t_dma_to, timing_cgra->t_dma_from, timing_cgra->n_dms);


    /* ======================================================== */
    /*        Loop through all different experiments            */
    /* ======================================================== */

    // uint32_t list_a_rows[] = {120,  121,    121,    121,    121,    1,      121};
    // uint32_t list_a_cols[] = {400,  16,     4,      16,     4,      16,     121};
    // uint32_t list_b_cols[] = {16,   4,      121,    16,     16,     16,     4};
    uint32_t list_a_rows[] = {120,  124,    124,    124,    124,    1,      124};
    uint32_t list_a_cols[] = {400,  16,     4,      16,     4,      16,     121};
    uint32_t list_b_cols[] = {16,   4,      124,    16,     16,     16,     4};

    uint32_t list_element_sizes[] = {4};

    for (int i = 0; i < 7; i++){
        for (int j = 0; j < 1; j++){
            // reset timing
            memset(timing_cgra, 0, sizeof(timings_t));

            // move data from A and B which are in flash to A_ram and B_ram which are in ram
            timing_cgra->t_tmp1 = timer_get_cycles();
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, list_a_rows[i]*list_a_cols[i]*list_element_sizes[j]) != FLASH_OK)return -1;
            w25q128jw_wait_quad_dma_async(A_ram, list_a_rows[i]*list_a_cols[i]*list_element_sizes[j]);
            if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, list_a_cols[i]*list_b_cols[i]*list_element_sizes[j]) != FLASH_OK)return -1;
            w25q128jw_wait_quad_dma_async(B_ram, list_a_cols[i]*list_b_cols[i]*list_element_sizes[j]);
            timing_cgra->t_flash = timer_get_cycles() - timing_cgra->t_tmp1;

            /* =======================================
            * ====== Runing on CGRA ======
            * ======================================== */
            t1 = timer_get_cycles();
            cgraMatMulTiled(A_ram, B_ram, R_ram, list_a_rows[i], list_a_cols[i], list_b_cols[i], cgra_buffer, CGRA_BUFFER_SIZE, timing_cgra);
            timing_cgra->t_tot = timer_get_cycles() - t1;

            PRINTF("'CGRA' size: %dx%dx%d,\telement size: %d,\tflash: %d,\ttotal: %d,\tprc: %d,\tdma_to: %d,\tdma_from: %d,\tn_dms: %d\n", list_a_rows[i], list_a_cols[i], list_b_cols[i], list_element_sizes[j], timing_cgra->t_flash, timing_cgra->t_tot, timing_cgra->t_prc, timing_cgra->t_dma_to, timing_cgra->t_dma_from, timing_cgra->n_dms);
        }

    }


    return 0;
}
// Interrupt controller variables
static void handler_irq_cgra(uint32_t id) {
  cgra_intr_flag = 1;
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

void cgraMatMulTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram,
    uint32_t A_rows, uint32_t A_cols, uint32_t B_cols,
    int32_t *cgra_buffer, uint32_t cgra_buffer_size, timings_t *timing) {

    unsigned int K = A_cols;
    unsigned int max_elements = cgra_buffer_size;
    dma_sdk_init();

    // Check if the entire matrices fit in the CGRA buffer
    if (A_rows * A_cols + A_cols * B_cols + A_rows * B_cols <= max_elements) {
        // Direct execution without tiling
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)cgra_buffer, (uint32_t)A_ram, A_rows * A_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
        dma_copy((uint32_t)(cgra_buffer + A_rows * A_cols), (uint32_t)B_ram, A_cols * B_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
        timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        cgraMatMulRun(cgra_buffer, cgra_buffer + A_rows * A_cols, cgra_buffer + A_rows * A_cols + A_cols * B_cols, A_rows, A_cols, B_cols);
        while (!cgra_intr_flag) { wait_for_interrupt(); }
        timing->t_prc += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)R_ram, (uint32_t)(cgra_buffer + A_rows * A_cols + A_cols * B_cols), A_rows * B_cols, 0, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
        timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
        return; // Exit the function
    }

    unsigned int M = 0, N = 0;
    unsigned int A_rows_div_4 = A_rows >> 2; // A_rows / 4
    unsigned int B_cols_div_4 = B_cols >> 2; // B_cols / 4

    // Find maximum M and N that fit in CGRA memory and are multiples of 4
    for (M = A_rows_div_4 << 2; M >= 4; M -= 4) {
        unsigned int a_tile = M * K;
        if (a_tile >= max_elements) continue;

        unsigned int remaining = max_elements - a_tile;
        unsigned int denom = K + M;
        if (denom == 0) break;

        unsigned int max_N = remaining / denom;
        N = (max_N >> 2) << 2; // max_N / 4 * 4
        if (N >= 4) {
            if (N > B_cols) {
                N = B_cols_div_4 << 2;
                if (N < 4) continue;
            }
            if ((K * N + M * N) <= remaining) break;
        }
    }

    if (M < 4) {
        PRINTF("Error: Cannot find valid tile sizes.\n");
        return;
    }

    /* Process each tile */
    for (unsigned int i = 0; i < A_rows; i += M) {
        unsigned int current_M = (i + M <= A_rows) ? M : A_rows - i;
        current_M &= ~0x3; // current_M = (current_M / 4) * 4;
        current_M = current_M < 4 ? 4 : current_M;

        for (unsigned int j = 0; j < B_cols; j += N) {
            unsigned int current_N = (j + N <= B_cols) ? N : B_cols - j;
            current_N &= ~0x3; // current_N = (current_N / 4) * 4;
            current_N = current_N < 4 ? 4 : current_N;

            /* Calculate buffer pointers */
            size_t a_size = current_M * K;
            size_t b_size = K * current_N;
            size_t r_size = current_M * current_N;

            if (a_size + b_size + r_size > CGRA_BUFFER_SIZE) {
                PRINTF("Tile exceeds CGRA memory.\n");
                return -1;
            }

        #ifdef DEBUG
            PRINTF("tile size: %d x %d x %d\n", current_M, K, current_N);
        #endif

            int32_t *A_cgra = cgra_buffer;
            int32_t *B_cgra = A_cgra + a_size;
            int32_t *R_cgra = B_cgra + b_size;

            timing->t_tmp1 = timer_get_cycles();
            /* Copy B tile (row-wise slices) */
            for (unsigned int k = 0; k < K; ++k) {
                dma_copy((uint32_t)(B_cgra + k * current_N), (uint32_t)(B_ram + k * B_cols + j), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
            }
            /* Copy A tile */
            dma_copy((uint32_t)(A_cgra), (uint32_t)(A_ram + i * K), current_M * K, 2, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
            timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;

            /* Run CGRA MatMul */
            timing->t_tmp1 = timer_get_cycles();
            cgraMatMulRun(A_cgra, B_cgra, R_cgra, current_M, K, current_N);
            while (!cgra_intr_flag) { wait_for_interrupt(); }
            timing->t_prc += timer_get_cycles() - timing->t_tmp1;

            /* Copy result back to R_ram */
            timing->t_tmp1 = timer_get_cycles();
            for (unsigned int row = 0; row < current_M; ++row) {
                dma_copy((uint32_t)(R_ram + (i + row) * B_cols + j), (uint32_t)(R_cgra + row * current_N), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
            }
            timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
            timing->n_dms += 1;
        }
    }
}


/****************************************************************************/
/**                                 EOF                                    **/
/****************************************************************************/

