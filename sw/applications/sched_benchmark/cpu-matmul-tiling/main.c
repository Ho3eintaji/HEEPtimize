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

#define CPU_BUFFER_SIZE (120 * 1024 / sizeof(int32_t))
// #define DEBUG

/****************************************************************************/
/**                      PROTOTYPES OF LOCAL FUNCTIONS                     **/
/****************************************************************************/

// Prototype for the CPU matrix multiplication function
void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols);

// CPU matmul tiled
void cpuMatMulTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, int32_t *cpu_buffer, uint32_t cpu_buffer_size);

/****************************************************************************/
/**                           GLOBAL VARIABLES                             **/
/****************************************************************************/

// cache: whole data is cached
int32_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS] = {1};
int32_t *A_ram = cache;
int32_t *B_ram = cache + A_ROWS*A_COLS;
int32_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;

// CPU buffer
int32_t cpu_buffer[CPU_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};

/****************************************************************************/

int main(void)
{

    uint32_t t1, t2, t_cpu;
    
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

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);

    /* =======================================
    * ====== Runing on CPU ======
    * ======================================== */
   t1 = timer_get_cycles();
   cpuMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cpu_buffer, CPU_BUFFER_SIZE);
    t_cpu = timer_get_cycles() - t1;

    PRINTF("CPU MatMul completed in %u cycles.\n", t_cpu);

    PRINTF("R_ram[0]: %x\n", R_ram[0]);
    PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);

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


void cpuMatMulTiled(int32_t *A_ram, int32_t *B_ram, int32_t *R_ram,
    uint32_t A_rows, uint32_t A_cols, uint32_t B_cols,
    int32_t *cpu_buffer, uint32_t cpu_buffer_size) {

    unsigned int K = A_cols;
    unsigned int max_elements = cpu_buffer_size;
    dma_sdk_init();

    // Check if the entire matrices fit in the CPU buffer
    if (A_rows * A_cols + A_cols * B_cols + A_rows * B_cols <= max_elements) {
        // Direct execution without tiling
        dma_copy((uint32_t)cpu_buffer, (uint32_t)A_ram, A_rows * A_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
        dma_copy((uint32_t)(cpu_buffer + A_rows * A_cols), (uint32_t)B_ram, A_cols * B_cols, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
        cpuMatMul(cpu_buffer, cpu_buffer + A_rows * A_cols, R_ram, A_rows, A_cols, B_cols);
        dma_copy((uint32_t)R_ram, (uint32_t)(cpu_buffer + A_rows * A_cols + A_cols * B_cols), A_rows * B_cols, 0, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
    #ifdef DEBUG
        PRINTF("Direct execution without tiling.\n");
    #endif
        return; // Exit the function
    }

    uint32_t M = 0, N = 0;

    // Find maximum M and N that fit in CPU memory
    for (M = A_rows; M >= 1; --M) {
        uint32_t a_tile = M * K;
        if (a_tile >= max_elements) continue;

        uint32_t remaining = max_elements - a_tile;
        uint32_t denom = K + M;
        if (denom == 0) break;

        N = remaining / denom;
        if (N >= 1) {
            if (N > B_cols) {
                N = B_cols;
            }
            if ((K * N + M * N) <= remaining) break;
        }
    }

    if (M < 1) {
        fprintf(stderr, "Error: Cannot find valid tile sizes.\n");
        return;
    }

    /* Process each tile */
    for (uint32_t i = 0; i < A_rows; i += M) {
        uint32_t current_M = (i + M <= A_rows) ? M : A_rows - i;

        for (uint32_t j = 0; j < B_cols; j += N) {
            uint32_t current_N = (j + N <= B_cols) ? N : B_cols - j;

            /* Calculate buffer pointers */
            size_t a_size = current_M * K;
            size_t b_size = K * current_N;
            size_t r_size = current_M * current_N;

            

            if (a_size + b_size + r_size > cpu_buffer_size) {
                PRINTF("Tile exceeds CPU memory.\n");
                return -1;
            }
        #ifdef DEBUG
            PRINTF("tile size: %d x %d x %d\n", current_M, K, current_N);
        #endif

            int32_t *A_cpu = cpu_buffer;
            int32_t *B_cpu = A_cpu + a_size;
            int32_t *R_cpu = B_cpu + b_size;

            /* Copy A tile */
            dma_copy((uint32_t)(A_cpu), (uint32_t)(A_ram + i * K), current_M * K, 2, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);

            /* Copy B tile (row-wise slices) */
            for (unsigned int col = 0; col < K; ++col) {
                dma_copy((uint32_t)(B_cpu + col * current_N), (uint32_t)(B_ram + col * B_cols + j), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
            }


            /* Run CPU MatMul */
            cpuMatMul(A_cpu, B_cpu, R_cpu, current_M, K, current_N);

            /* Copy result back to R_ram */
            for (unsigned int row = 0; row < current_M; ++row) {
                dma_copy((uint32_t)(R_ram + (i + row) * B_cols + j), (uint32_t)(R_cpu + row * current_N), current_N, 1, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);
            }
        }
    }
}


/****************************************************************************/
/**                                 EOF                                    **/
/****************************************************************************/

