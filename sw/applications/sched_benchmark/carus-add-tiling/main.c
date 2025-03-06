// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein
// Date: 06/03/2025
// Description: Tiling version of carus-add

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
#include "carus_add.h"


#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif 

// #define DEBUG
#define CHECK_RESULTS

#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes

void carusAdd(data_t *A_tile, data_t *B_tile, uint32_t in_size, carus_cfg_t *cfg, dma_data_type_t dma_type, uint8_t elem_size);
void carusAddTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t add_size, carus_cfg_t *cfg, dma_data_type_t dma_type, uint8_t elem_size);

// cache: whole data is cached
#ifdef CHECK_RESULTS
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS * 2] = {0};
#else
data_t cache [A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS] = {0};
#endif
data_t *A_ram = cache;
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;
#ifdef CHECK_RESULTS
data_t *R_ram2 = cache + A_ROWS*A_COLS + B_ROWS*B_COLS + R_ROWS*R_COLS;
#endif


int main(void)
{
    uint32_t t1, t2, t_pe;

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
    if (carus_init(CARUS_INSTANCE) != 0) return 1;
    // Load kernel
    if (carus_load_kernel(0, carus_add, CARUS_ADD_SIZE, 0) != 0) return 1;

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

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
#ifdef CHECK_RESULTS
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)R), R_ram2, R_ROWS*R_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(R_ram2, R_ROWS*R_COLS*ELEM_SIZE);
#endif

    /* =======================================
    * ====== Runing on Carus =================
    * ======================================== */
   dma_sdk_init();

   t1 = timer_get_cycles();
   carusAddTiled(A_ram, B_ram, R_ram, A_ROWS*A_COLS, &cfg, dma_type, ELEM_SIZE);
   t_pe = timer_get_cycles() - t1;

   PRINTF("Carus Add completed in %u cycles.\n", t_pe);



#ifdef CHECK_RESULTS
    // Compare the results
    for (uint32_t i = 0; i < R_ROWS * R_COLS; i++) {
        if (R_ram[i] != R_ram2[i]) {
            PRINTF("Error: R[%d] = %x, R2[%d] = %x\n", i, R_ram[i], i, R_ram2[i]);
            return 1;
        }
    }
    PRINTF("Results match.\n");
#endif

   return 0;
    
}

void cpuAdd(data_t *A, data_t *B, data_t *R, unsigned int N) {
    for (unsigned int i = 0; i < N; i++) {
        R[i] = A[i] + B[i];
    }
}


// Original carusMatmul function (for single tile)
void carusAdd(data_t *A_tile, data_t *B_tile, uint32_t in_size, carus_cfg_t *cfg, dma_data_type_t dma_type, uint8_t elem_size)
{
    data_t *row_ptr;
    // ----- Carus configuration -----
    cfg->vl    = (uint32_t)in_size;

    if (carus_set_cfg(CARUS_INSTANCE, cfg) != 0) return 1;

    // Copy vector A
    row_ptr = (data_t *) carus_vrf(0, CARUS_ADD_A_VREG);
    dma_copy((uint32_t) row_ptr, (uint32_t) A_tile, in_size * elem_size, 0, dma_type, dma_type, 0);

    // Copy matrix B
    row_ptr = (data_t *) carus_vrf(0, CARUS_ADD_B_VREG);
    dma_copy((uint32_t) row_ptr, (uint32_t) B_tile, in_size * elem_size, 0, dma_type, dma_type, 0);

    // Run the kernel
    if (carus_run_kernel(0) != 0) return 1;
    if (carus_wait_done(0) != 0) return 1;

    // // Read returned VL for strip-mining
    // if (carus_get_cfg(0, &cfg) != 0) return 1;

}

// void carusAdd(void);
void carusAddTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t add_size, carus_cfg_t *cfg, dma_data_type_t dma_type, uint8_t elem_size){
    // Maximum tile sizes based on Carus limitations
    const uint32_t MAX_ADD_SIZE = VL_MAX; // number of elements
    data_t *row_ptr = (data_t *) carus_vrf(0, CARUS_ADD_R_VREG);

    // Number of tiles
    uint32_t num_tiles = (add_size + MAX_ADD_SIZE - 1) / MAX_ADD_SIZE;

    // Loop over tiles
    for (uint32_t i = 0; i < num_tiles; i++) {
        uint32_t tile_size;
        if (i == num_tiles - 1) {
            // Last tile
            tile_size = (add_size - (i * MAX_ADD_SIZE));
        } else {
            tile_size = MAX_ADD_SIZE;
        }
        carusAdd(A_ram + i * MAX_ADD_SIZE, B_ram + i * MAX_ADD_SIZE, tile_size, cfg, dma_type, elem_size);

        // Copy the result back to R_ram
        dma_copy((uint32_t) (R_ram + i * MAX_ADD_SIZE), (uint32_t) row_ptr, (uint32_t)(tile_size), 0, dma_type, dma_type, 0);
        //TODO: this is weird, I gave the size not in bytes but this is the correct way!
    }
}