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
#include "carus.h"
#include "carus_matmul.h"

#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

#define CGRA_COL_INPUT_SIZE 4
#define CHECK_RESULTS
// #define DEBUG
#define CARUS_MEMORY_SIZE (64 * 1024) // 64KB Carus memory
#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define CARUS_MAX_A_ROWS 16  // row_a < 17
#define CARUS_MAX_A_COLS 15   // col_a < 16
#define CARUS_MAX_B_COLS 2024 // Based on the tiling loop in carusMatmulTiled
#define TEMP_R_CACHE_SIZE (CARUS_MAX_A_ROWS * CARUS_MAX_B_COLS)
#define CGRA_CPU_BUFFER_SIZE (120 * 1024 / sizeof(int32_t))

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

static volatile bool cgra_intr_flag;
static void handler_irq_cgra(uint32_t id) {cgra_intr_flag = 1;}
void cgraMatMulRun(data_t *A, data_t *B, data_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols);
void cgraMatMulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, data_t *cgra_buffer, uint32_t cgra_buffer_size, dma_data_type_t dma_type, timings_t * timing);
void carusMatmul(data_t *A_tile, data_t *B_tile, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, carus_cfg_t * cfg, dma_data_type_t dma_type, uint32_t AROWS, uint32_t ACOLS, uint32_t BCOLS, timings_t * timing);
void carusMatmulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, carus_cfg_t *cfg, dma_data_type_t dma_type, timings_t * timing);
void cpuMatMul(data_t *A, data_t *B, data_t *C, unsigned int A_rows, unsigned int A_cols, unsigned int B_cols);
void cpuMatMulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, data_t *cpu_buffer, uint32_t cpu_buffer_size, dma_data_type_t dma_type, timings_t * timing);

// Plic controller variables
static cgra_t     cgra;
static uint8_t    cgra_slot;
static int32_t cgra_input[CGRA_N_COLS][CGRA_COL_INPUT_SIZE] __attribute__ ((aligned (4)));
// cache: whole data is cached
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS + TEMP_R_CACHE_SIZE] = {0};
data_t *A_ram = cache;
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;
data_t *temp_R_cache = cache + A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS; // Fixed cache for temp_R
data_t cgra_cpu_buffer[CGRA_CPU_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};
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

    plic_irq_set_priority(CGRA_INTR, 1);
    plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled);
    plic_assign_external_irq_handler(CGRA_INTR, (void *) &handler_irq_cgra);
    cgra_intr_flag = 0;

    /* CGRA matmul bitstream loading */
    cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
    cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);
    cgra_slot = cgra_get_slot(&cgra);

     // ----- System initialization -----
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1
    // Initialize PLIC for external NM-Carus interrupt
    if (ext_irq_init() != 0) return 1;

    // ------ Initialize NM-Carus ------
    if (carus_init(CARUS_INSTANCE) != 0) return 1;
    if (carus_load_kernel(0, carus_matmul, CARUS_MATMUL_SIZE, 0) != 0) return 1;
    carus_cfg_t cfg = CARUS_CFG_INIT(CARUS_INSTANCE);
    switch (ELEM_SIZE){
        case 1: cfg.vtype = VTYPE_VSEW_8; break;
        case 2: cfg.vtype = VTYPE_VSEW_16; break;
        case 4: cfg.vtype = VTYPE_VSEW_32; break;
        default: return 1;
    }

    // intialize the timings recordings and allocate to zero
    timings_t * timing_cpu = (timings_t *)malloc(sizeof(timings_t));
    timings_t * timing_carus = (timings_t *)malloc(sizeof(timings_t));
    timings_t * timing_cgra = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_cpu, 0, sizeof(timings_t));
    memset(timing_carus, 0, sizeof(timings_t));
    memset(timing_cgra, 0, sizeof(timings_t));

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
   timing_cpu->t_tmp1 = timer_get_cycles();
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((uint32_t)heep_get_flash_address_offset((uint32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    timing_cpu->t_flash = timer_get_cycles() - timing_cpu->t_tmp1;

    timing_carus->t_flash = timing_cpu->t_flash;
    timing_cgra->t_flash = timing_cpu->t_flash;

    /* =======================================
    * ====== Runing on PE ====================
    * ======================================== */
    dma_sdk_init(); 

    // cpu
    t1 = timer_get_cycles();
    cpuMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cgra_cpu_buffer, CGRA_CPU_BUFFER_SIZE, dma_type, timing_cpu);
    timing_cpu->t_tot = timer_get_cycles() - t1;

    PRINTF("CPU: \n", t_pe);
    PRINTF("R_ram[0]: %x\n", R_ram[0]);
    PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);
    
    // set R_ram to 0
    dma_fill((uint32_t)R_ram, 0, R_ROWS*R_COLS, 0, dma_type, dma_type, 0);

    t1 = timer_get_cycles();
    carusMatmulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, &cfg, dma_type, timing_carus);
    timing_carus->t_tot = timer_get_cycles() - t1;

    PRINTF("Carus:\n", t_pe);
    PRINTF("R_ram[0]: %x\n", R_ram[0]);
    PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);

    // // set R_ram to 0
    // dma_fill((uint32_t)R_ram, 0, R_ROWS*R_COLS, 0, dma_type, dma_type, 0);

    // t1 = timer_get_cycles();
    // cgraMatMulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, cgra_cpu_buffer, CGRA_CPU_BUFFER_SIZE, dma_type, timing_cgra);
    // t_pe = timer_get_cycles() - t1;

    // PRINTF("CGRA:\n", t_pe);
    // PRINTF("R_ram[0]: %x\n", R_ram[0]);
    // PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);

    // PRINT all the timings
    PRINTF("CPU: flash: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d, total: %d\n", timing_cpu->t_flash, timing_cpu->t_prc, timing_cpu->t_dma_to, timing_cpu->t_dma_from, timing_cpu->n_dms, timing_cpu->t_tot);
    PRINTF("Carus: flash: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d, total: %d\n", timing_carus->t_flash, timing_carus->t_prc, timing_carus->t_dma_to, timing_carus->t_dma_from, timing_carus->n_dms, timing_carus->t_tot);
    // PRINTF("CGRA: flash: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d, total: %d\n", timing_cgra->t_flash, timing_cgra->t_prc, timing_cgra->t_dma_to, timing_cgra->t_dma_from, timing_cgra->n_dms, timing_cgra->t_tot);


    return 0;
}


void carusMatmul(data_t *A_tile, data_t *B_tile, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, carus_cfg_t * cfg, dma_data_type_t dma_type, uint32_t AROWS, uint32_t ACOLS, uint32_t BCOLS, timings_t * timing)
{
    data_t *row_ptr;

    // ----- Carus configuration -----
    cfg->vl    = (uint32_t)B_cols;
    cfg->arg0  = (uint32_t)A_rows;
    cfg->arg1  = (uint32_t)A_cols;

    if (carus_set_cfg(CARUS_INSTANCE, cfg) != 0) return 1;

    timing->t_tmp1 = timer_get_cycles();
    for (unsigned int i = 0; i < A_cols; i++)
    {
        row_ptr = (data_t *)carus_vrf(0, CARUS_MATMUL_B_VREG + i);
        dma_copy((uint32_t)row_ptr, (uint32_t)(B_tile + i * B_cols), B_cols * ELEM_SIZE, 0, dma_type, dma_type, 0);
    }

    // move row by row A
    row_ptr = (data_t *)carus_vrf(0, CARUS_MATMUL_A_VREG);
    for (unsigned int i = 0; i < A_rows; i++)
    {
        dma_copy((uint32_t)(row_ptr+i*A_cols), (uint32_t)(A_tile + i * ACOLS), A_cols * ELEM_SIZE, 0, dma_type, dma_type, 0); // Corrected indexing: using A_cols
    }
    timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;

    timing->t_tmp1 = timer_get_cycles();
    // Run the kernel
    if (carus_run_kernel(0) != 0) return 1;
    if (carus_wait_done(0) != 0) return 1;
    timing->t_prc += timer_get_cycles() - timing->t_tmp1;

}

void carusMatmulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t AROWS, uint32_t ACOLS, uint32_t BCOLS, carus_cfg_t *cfg, dma_data_type_t dma_type, timings_t * timing) {

    // Maximum tile sizes based on Carus limitations
    const uint32_t MAX_A_ROWS = CARUS_MAX_A_ROWS;  // row_a < 17
    const uint32_t MAX_A_COLS = CARUS_MAX_A_COLS;   // col_a < 16
    const uint32_t MAX_B_COLS = CARUS_MAX_B_COLS; // up to 1024 practically

    // Split into tiles.  Prioritize making tiles as large as possible within the limits.
    for (uint32_t i = 0; i < AROWS; i += MAX_A_ROWS) {
        uint32_t current_A_rows = (i + MAX_A_ROWS <= AROWS) ? MAX_A_ROWS : AROWS - i;

        for (uint32_t j = 0; j < BCOLS; j += MAX_B_COLS) { // Assuming BCOLS in carusMatmul is limited.  63 gives good utilization.
            uint32_t current_B_cols = (j + MAX_B_COLS <= BCOLS) ? MAX_B_COLS : BCOLS - j;

            for (uint32_t k = 0; k < ACOLS; k += MAX_A_COLS) {
                uint32_t current_A_cols = (k + MAX_A_COLS <= ACOLS) ? MAX_A_COLS : ACOLS - k;

                // // Check size limits *before* DMA transfers
                // if (current_A_rows * current_A_cols * ELEM_SIZE + current_A_cols * current_B_cols * ELEM_SIZE  > CARUS_MEMORY_SIZE)
                // {
                //    PRINTF("Tile too large for Carus memory.\n");
                //     return; // Or handle the error appropriately
                // }

                // Prepare pointers for this tile.
                data_t *tile_A = A_ram + i * ACOLS + k;
                data_t *tile_B = B_ram + k * BCOLS + j;

                // Intermediate result buffer – accumulate results *within* Carus memory
                data_t *temp_R = temp_R_cache; // Use fixed cache memory
                // memset(temp_R, 0, current_A_rows * current_B_cols * sizeof(data_t));

                #ifdef DEBUG
                    PRINTF("Tile size: %d x %d x %d\n", current_A_rows, current_A_cols, current_B_cols);
                #endif

                // Call the existing carusMatmul function.
                carusMatmul(tile_A, tile_B, current_A_rows, current_A_cols, current_B_cols, cfg, dma_type, AROWS, ACOLS, BCOLS, timing);

                timing->t_tmp1 = timer_get_cycles();
                // Accumulate results.  DMA back to R_ram in tiles.
                for (int row = 0; row < current_A_rows; ++row)
                {
                   data_t *carus_result_row = (data_t *)carus_vrf(0, CARUS_MATMUL_R_VREG + row);
                   dma_copy((uint32_t)(temp_R + row * current_B_cols), (uint32_t)carus_result_row, current_B_cols * ELEM_SIZE, 0, dma_type, dma_type, 0); // Read one row each time from carus
                }

                for (int m = 0; m < current_A_rows; m++)
                {
                    for(int n = 0; n < current_B_cols; n++)
                    {
                        R_ram[(i + m) * BCOLS + (j + n)] += temp_R[m * current_B_cols + n];
                    }
                }
                timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
                // No need to free(temp_R) as it's now fixed cache
                timing->n_dms++;
            }
        }
    }
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

// Interrupt controller variables

//cgra Matmul
void cgraMatMulRun(data_t *A, data_t *B, data_t *C, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols){
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
  
void cgraMatMulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, data_t *cgra_buffer, uint32_t cgra_buffer_size, dma_data_type_t dma_type, timings_t * timing) {

    unsigned int K = A_cols;
    unsigned int max_elements = cgra_buffer_size;
    dma_sdk_init();

    // Check if the entire matrices fit in the CGRA buffer
    if (A_rows * A_cols + A_cols * B_cols + A_rows * B_cols <= max_elements) {
        // Direct execution without tiling
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)cgra_buffer, (uint32_t)A_ram, A_rows * A_cols, 1, dma_type, DMA_DATA_TYPE_WORD, 0);
        dma_copy((uint32_t)(cgra_buffer + A_rows * A_cols), (uint32_t)B_ram, A_cols * B_cols, 1, dma_type, DMA_DATA_TYPE_WORD, 0);
        timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        PRINTF("CGRA before: %u\n", timer_get_cycles());
        cgraMatMulRun(cgra_buffer, cgra_buffer + A_rows * A_cols, cgra_buffer + A_rows * A_cols + A_cols * B_cols, A_rows, A_cols, B_cols);
        while (!cgra_intr_flag) { wait_for_interrupt(); }
        PRINTF("CGRA after: %u\n", timer_get_cycles());
        timing->t_prc += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)R_ram, (uint32_t)(cgra_buffer + A_rows * A_cols + A_cols * B_cols), A_rows * B_cols, 0, DMA_DATA_TYPE_WORD, dma_type, 0);
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

            if (a_size + b_size + r_size > cgra_buffer_size) {
                PRINTF("Tile exceeds CGRA memory.\n");
                return -1;
            }

        #ifdef DEBUG
            PRINTF("tile size: %d x %d x %d\n", current_M, K, current_N);
        #endif

            int32_t *A_cgra = cgra_buffer;
            int32_t *B_cgra = A_cgra + a_size;
            int32_t *R_cgra = B_cgra + b_size;

            /* Copy B tile (row-wise slices) */
            timing->t_tmp1 = timer_get_cycles();
            for (unsigned int k = 0; k < K; ++k) {
                dma_copy((uint32_t)(B_cgra + k * current_N), (uint32_t)(B_ram + k * B_cols + j), current_N, 1, dma_type, DMA_DATA_TYPE_WORD, 0);
            }
            /* Copy A tile */
            dma_copy((uint32_t)(A_cgra), (uint32_t)(A_ram + i * K), current_M * K, 2, dma_type, DMA_DATA_TYPE_WORD, 0);
            timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;

            /* Run CGRA MatMul */
            timing->t_tmp1 = timer_get_cycles();
            PRINTF("CGRA before: %u\n", timer_get_cycles());
            cgraMatMulRun(A_cgra, B_cgra, R_cgra, current_M, K, current_N);
            while (!cgra_intr_flag) { wait_for_interrupt(); }
            PRINTF("CGRA after: %u\n", timer_get_cycles());
            timing->t_prc += timer_get_cycles() - timing->t_tmp1;

            timing->t_tmp1 = timer_get_cycles();
            /* Copy result back to R_ram */
            for (unsigned int row = 0; row < current_M; ++row) {
                dma_copy((uint32_t)(R_ram + (i + row) * B_cols + j), (uint32_t)(R_cgra + row * current_N), current_N, 1, DMA_DATA_TYPE_WORD, dma_type, 0);
            }
            timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
            timing->n_dms++;
        }
    }
}
  