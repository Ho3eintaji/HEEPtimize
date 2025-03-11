// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein Taji
// Description: Main file for the matrix multiplication application


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
#include "carus_matmul.h"
#include "data.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"
#include "x-heep.h"
#include "w25q128jw.h"
#include "defines.h"
#include "core_v_mini_mcu.h"


#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

// #define DEBUG
// #define DEBUG_DMA

#define CARUS_MEMORY_SIZE (64 * 1024) // 64KB Carus memory
#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define CARUS_MAX_A_ROWS 16  // row_a < 17
#define CARUS_MAX_A_COLS 15   // col_a < 16
#define CARUS_MAX_B_COLS 2024 // Based on the tiling loop in carusMatmulTiled
#define TEMP_R_CACHE_SIZE (CARUS_MAX_A_ROWS * CARUS_MAX_B_COLS)

typedef struct {
    uint32_t t_prc;
    uint32_t t_flash;
    uint32_t t_dma_to;
    uint32_t t_dma_from;
    uint32_t n_dms;
    uint32_t t_tot;
    uint32_t t_acc;
    uint32_t t_tmp1;
    uint32_t t_tmp2;
} timings_t;
uint32_t t1, t2;


// Launch carus matmul
void carusMatmul(data_t *A_tile, data_t *B_tile, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, carus_cfg_t * cfg, dma_data_type_t dma_type, uint32_t AROWS, uint32_t ACOLS, uint32_t BCOLS, timings_t * timing);
void carusMatmulTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t A_rows, uint32_t A_cols, uint32_t B_cols, carus_cfg_t *cfg, dma_data_type_t dma_type, timings_t * timing);

// cache: whole data is cached
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS + TEMP_R_CACHE_SIZE] = {0};
data_t *A_ram = cache;
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;
data_t *temp_R_cache = cache + A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS; // Fixed cache for temp_R


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
    dma_data_type_t dma_type;
    dma_data_type_t dma_type_double = DMA_DATA_TYPE_WORD;

    // Initialize NM-Carus
    if (carus_init(CARUS_INSTANCE) != 0)
        return 1;

    // Load kernel
    if (carus_load_kernel(0, carus_matmul, CARUS_MATMUL_SIZE, 0) != 0){
        printf("Error loading kernel\n");
        return 1;
    }

    carus_cfg_t cfg = CARUS_CFG_INIT(0);
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

    default:
        return 1;
    }

    // intialize the timings recordings and allocate to zero
    timings_t * timing_carus = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_carus, 0, sizeof(timings_t));

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    timing_carus->t_tmp1 = timer_get_cycles();
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    timing_carus->t_flash = timer_get_cycles() - timing_carus->t_tmp1;

    /* =======================================
    * ====== Runing on Carus =================
    * ======================================== */
    dma_sdk_init();

    t1 = timer_get_cycles();
    carusMatmulTiled(A_ram, B_ram, R_ram, A_ROWS, A_COLS, B_COLS, &cfg, dma_type, timing_carus);
    timing_carus->t_tot = timer_get_cycles() - t1;

    PRINTF("R_ram[0]: %x\n", R_ram[0]);
    PRINTF("R_ram[%d]: %x\n", R_ROWS*R_COLS-1, R_ram[R_ROWS*R_COLS-1]);

    PRINTF("Carus-matmul: flash: %d, total: %d, prc: %d, dma_to: %d, dma_from: %d, acc: %d, n_dms: %d\n", timing_carus->t_flash, timing_carus->t_tot, timing_carus->t_prc, timing_carus->t_dma_to, timing_carus->t_dma_from, timing_carus->t_acc, timing_carus->n_dms);

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
        dma_copy((uint32_t)row_ptr, (uint32_t)(B_tile + i * B_cols), B_cols, 0, dma_type, dma_type, 0);
    }

    // move row by row A
    row_ptr = (data_t *)carus_vrf(0, CARUS_MATMUL_A_VREG);
    for (unsigned int i = 0; i < A_rows; i++)
    {
        dma_copy((uint32_t)(row_ptr+i*A_cols), (uint32_t)(A_tile + i * ACOLS), A_cols, 0, dma_type, dma_type, 0); // Corrected indexing: using A_cols
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
                   dma_copy((uint32_t)(temp_R + row * current_B_cols), (uint32_t)carus_result_row, current_B_cols, 0, dma_type, dma_type, 0); // Read one row each time from carus
                }
                timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;

                timing->t_tmp1 = timer_get_cycles();
                for (int m = 0; m < current_A_rows; m++)
                {
                    for(int n = 0; n < current_B_cols; n++)
                    {
                        R_ram[(i + m) * BCOLS + (j + n)] += temp_R[m * current_B_cols + n];
                    }
                }
                timing->t_acc += timer_get_cycles() - timing->t_tmp1;
                
                // No need to free(temp_R) as it's now fixed cache
                timing->n_dms++;
            }
        }
    }
}

