// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein Taji
// Date: 22/06/2023
// Description: Main file for running matmul on multi-accels platform

//TODO: one issue on cgra part

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

#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

#define CHECK_RESULTS
// #define DEBUG


#define CPU_BUFFER_SIZE (32 * 1024 / sizeof(data_t))

// putting an struct for records of timings
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

void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols);
void cpuMatMulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, data_t *cpu_buffer, uint32_t cpu_buffer_size, dma_data_type_t dma_type, timings_t * timing);


// cache: whole data is cached
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS ] = {0};
data_t *A_ram = cache;
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;
data_t cpu_buffer[CPU_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};
uint32_t t1, t2, t_pe;


int main(void)
{
    /* ============================== 
    * ====== Initialization =========
    * ============================== */

    // Initialize the DMA
    dma_sdk_init();
    dma_data_type_t dma_type;
    dma_data_type_t dma_type_double = DMA_DATA_TYPE_WORD;
    switch (ELEM_SIZE){
        case 1: dma_type = DMA_DATA_TYPE_BYTE; break;
        case 2: dma_type = DMA_DATA_TYPE_HALF_WORD; break;
        case 4: dma_type = DMA_DATA_TYPE_WORD; break;
        default: return 1;
    }

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

     // ----- System initialization -----
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1
    // Initialize PLIC for external NM-Carus interrupt
    if (ext_irq_init() != 0) return 1;


    // intialize the timings recordings and allocate to zero
    timings_t * timing_cpu = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_cpu, 0, sizeof(timings_t));

    // /* ==============================
    // * ====== Putting data in cache ======
    // * ============================== */
    //         timing_cpu->t_tmp1 = timer_get_cycles();
    // // move data from A and B which are in flash to A_ram and B_ram which are in ram
    // if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    // w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    // if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    // w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    //         timing_cpu->t_flash = timer_get_cycles() - timing_cpu->t_tmp1;

    //         /* =======================================
    // * ====== Runing on PE ====================
    //         * ======================================== */
    // dma_sdk_init(); 

    // // cpu
    //         t1 = timer_get_cycles();
    // cpuMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cpu_buffer, CPU_BUFFER_SIZE, dma_type, timing_cpu);
    //         timing_cpu->t_tot = timer_get_cycles() - t1;

    // PRINTF("R_ram[0]: %x\n", R_ram[0]);
    // PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);

    // // PRINT all the timings
    // PRINTF("CPU: flash: %d, total: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d\n", timing_cpu->t_flash, timing_cpu->t_tot, timing_cpu->t_prc, timing_cpu->t_dma_to, timing_cpu->t_dma_from, timing_cpu->n_dms);

    /* ======================================================== */
    /*        Loop through all different experiments            */
    /* ======================================================== */

    uint32_t list_a_rows[] = {120,  121,    121,    121,    121,    1,      121};
    uint32_t list_a_cols[] = {400,  16,     4,      16,     4,      16,     121};
    uint32_t list_b_cols[] = {16,   4,      121,    16,     16,     16,     4};

    for (int i = 0; i < 7; i++){
        // reset timing
        memset(timing_cpu, 0, sizeof(timings_t));

        // move data from A and B which are in flash to A_ram and B_ram which are in ram
        timing_cpu->t_tmp1 = timer_get_cycles();
        if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, list_a_rows[i]*list_a_cols[i]*ELEM_SIZE) != FLASH_OK)return -1;
        w25q128jw_wait_quad_dma_async(A_ram, list_a_rows[i]*list_a_cols[i]*ELEM_SIZE);
        if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, list_a_cols[i]*list_b_cols[i]*ELEM_SIZE) != FLASH_OK)return -1;
        w25q128jw_wait_quad_dma_async(B_ram, list_a_cols[i]*list_b_cols[i]*ELEM_SIZE);
        timing_cpu->t_flash = timer_get_cycles() - timing_cpu->t_tmp1;

        /* =======================================
        * ====== Runing on CGRA ======
        * ======================================== */
        t1 = timer_get_cycles();
        cpuMatMulTiled(A_ram, B_ram, R_ram, list_a_rows[i], list_a_cols[i], list_b_cols[i], cpu_buffer, CPU_BUFFER_SIZE, dma_type, timing_cpu);
        timing_cpu->t_tot = timer_get_cycles() - t1;

        // PRINTF("'CPU' size: %dx%dx%d,\telement size: %d,\tflash: %d,\ttotal: %d,\tprc: %d,\tdma_to: %d,\tdma_from: %d,\tn_dms: %d\n", list_a_rows[i], list_a_cols[i], list_b_cols[i], list_element_sizes[j], timing_cgra->t_flash, timing_cgra->t_tot, timing_cgra->t_prc, timing_cgra->t_dma_to, timing_cgra->t_dma_from, timing_cgra->n_dms);
        PRINTF("'CPU' size: %dx%dx%d,\t\tflash: %d,\ttotal: %d,\tprc: %d,\tdma_to: %d,\tdma_from: %d,\tn_dms: %d\n", list_a_rows[i], list_a_cols[i], list_b_cols[i], timing_cpu->t_flash, timing_cpu->t_tot, timing_cpu->t_prc, timing_cpu->t_dma_to, timing_cpu->t_dma_from, timing_cpu->n_dms);
    }

    return 0;
}


void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols){
    for (unsigned int i = 0; i < A_rows; i++) {
        for (unsigned int j = 0; j < B_cols; j++) {
            C[i*B_cols+j] = 0;
            for (unsigned int k = 0; k < A_cols; k++) {
                C[i*B_cols+j] += A[i*A_cols+k] * B[k*B_cols+j];
            }
        }
    }
}

void cpuMatMulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, data_t *cpu_buffer, uint32_t cpu_buffer_size, dma_data_type_t dma_type, timings_t * timing) {

    unsigned int K = A_cols;
    unsigned int max_elements = cpu_buffer_size;
    dma_sdk_init();

    // Check if the entire matrices fit in the CPU buffer
    if (A_rows * A_cols + A_cols * B_cols + A_rows * B_cols <= max_elements) {
        // Direct execution without tiling
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)cpu_buffer, (uint32_t)A_ram, A_rows * A_cols, 1, dma_type, dma_type, 0);
        dma_copy((uint32_t)(cpu_buffer + A_rows * A_cols), (uint32_t)B_ram, A_cols * B_cols, 1, dma_type, dma_type, 0);
        timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        cpuMatMul(cpu_buffer, cpu_buffer + A_rows * A_cols, cpu_buffer + A_rows * A_cols + A_cols * B_cols, A_rows, A_cols, B_cols);
        timing->t_prc += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)R_ram, (uint32_t)(cpu_buffer + A_rows * A_cols + A_cols * B_cols), A_rows * B_cols, 0, dma_type, dma_type, 0);
        timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;

    #ifdef DEBUG
        PRINTF("Direct execution without tiling.\n");
    #endif
        return; // Exit the function
    }

    // --- Tile Size Calculation (Modified for Larger Tiles) ---
    uint32_t M, N;

    // Instead of finding the *maximum* M and N that fit, we'll aim for larger tiles
    // by prioritizing fewer, larger tiles, but still respecting the memory limit.

    // 1.  Calculate a "chunk size" for rows (M).  Start with a large fraction of A_rows.
    M = A_rows / 2;  // Start by trying to process half of A's rows at a time.
    if (M == 0) M = 1; // Ensure M is at least 1.

    // 2.  Iteratively reduce M until a suitable N can be found.
    while (M > 0) {
        uint32_t a_tile_size = M * K;
        if (a_tile_size >= max_elements) {
            M /= 2; // Reduce M if the A tile alone is too big.
            if (M==0) M=1;
            continue;
        }

        uint32_t remaining_space = max_elements - a_tile_size;

        // 3. Calculate N based on the remaining space. Try for large N (a fraction of B_cols).
        N = B_cols / 2; // Start by trying to process half of B's columns.
        if (N == 0) N = 1;

        // 4.  Iteratively reduce N until it fits within the remaining space.
        while (N > 0) {
            uint32_t b_tile_size = K * N;
            uint32_t r_tile_size = M * N;

            if (b_tile_size + r_tile_size <= remaining_space) {
                break; // Found a valid N for the current M.
            }
            N /= 2; // Reduce N if B and R tiles don't fit.
            if (N==0) N=1;
        }

        if (N > 0) {
            break; // Found valid M and N.
        }

        M /= 2; // If no valid N was found, reduce M and try again.
        if (M==0) M=1;
    }

    if (M < 1 || N < 1) {
        PRINTF("Error: Cannot find valid tile sizes.\n");
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
        

            data_t *A_cpu = cpu_buffer;
            data_t *B_cpu = A_cpu + a_size;
            data_t *R_cpu = B_cpu + b_size;

            timing->t_tmp1 = timer_get_cycles();
            /* Copy A tile */
            dma_copy((uint32_t)(A_cpu), (uint32_t)(A_ram + i * K), current_M * K, 2, dma_type, dma_type, 0);
            /* Copy B tile (row-wise slices) */
            for (unsigned int col = 0; col < K; ++col) {
                dma_copy((uint32_t)(B_cpu + col * current_N), (uint32_t)(B_ram + col * B_cols + j), current_N, 1, dma_type, dma_type, 0);
            }
            timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;
            /* Run CPU MatMul */
            timing->t_tmp1 = timer_get_cycles();
            cpuMatMul(A_cpu, B_cpu, R_cpu, current_M, K, current_N);
            timing->t_prc += timer_get_cycles() - timing->t_tmp1;

            /* Copy result back to R_ram */
            timing->t_tmp1 = timer_get_cycles();
            for (unsigned int row = 0; row < current_M; ++row) {
                dma_copy((uint32_t)(R_ram + (i + row) * B_cols + j), (uint32_t)(R_cpu + row * current_N), current_N, 1, dma_type, dma_type, 0);
            }
            timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
            timing->n_dms++;
        }
    }
}