// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: main.c
// Author: Hossein
// Date: 06/03/2025
// Description: Tiling version of cpu-add

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

#define CPU_BUFFER_KB (120 * 1024)
#define CPU_BUFFER_SIZE (CPU_BUFFER_KB / ELEM_SIZE)

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

uint32_t t1, t2, t_pe;

void cpuAdd(data_t *A, data_t *B, data_t *R, unsigned int N);
void cpuAddTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t add_size, timings_t *timing);

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

// CPU buffer
data_t cpu_buffer[CPU_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};


int main(void)
{


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

    // intialize the timings recordings and allocate to zero
    timings_t * timing_cpu = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_cpu, 0, sizeof(timings_t));

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    timing_cpu->t_tmp1 = timer_get_cycles();
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    timing_cpu->t_flash = timer_get_cycles() - timing_cpu->t_tmp1;
#ifdef CHECK_RESULTS
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((data_t *)R), R_ram2, R_ROWS*R_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(R_ram2, R_ROWS*R_COLS*ELEM_SIZE);
#endif

    /* =======================================
    * ====== Runing on CPU =================
    * ======================================== */
    dma_sdk_init();

    t1 = timer_get_cycles();
    cpuAddTiled(A_ram, B_ram, R_ram, A_ROWS*A_COLS, timing_cpu);
    timing_cpu->t_tot = timer_get_cycles() - t1;

    // PRINT all the timings
    PRINTF("CPU: flash: %d, total: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d\n", timing_cpu->t_flash, timing_cpu->t_tot, timing_cpu->t_prc, timing_cpu->t_dma_to, timing_cpu->t_dma_from, timing_cpu->n_dms);



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

// void carusAdd(void);
void cpuAddTiled(data_t *A_ram, data_t *B_ram, data_t *R_ram, uint32_t add_size, timings_t *timing) {
    // Maximum tile sizes based on Carus limitations
    // const uint32_t MAX_ADD_SIZE = CPU_BUFFER_KB/ELEM_SIZE; // number of elements
    dma_data_type_t dma_type;
    switch (ELEM_SIZE)
    {
    case 1:
        dma_type = DMA_DATA_TYPE_BYTE;
        break;
    case 2:
        dma_type = DMA_DATA_TYPE_HALF_WORD;
        break;
    case 4:
        dma_type = DMA_DATA_TYPE_WORD;
        break;

    default:
        return 1;
    }

    // Number of tiles
    uint32_t num_tiles = (add_size + CPU_BUFFER_SIZE - 1) / CPU_BUFFER_SIZE;

    // Loop over tiles
    for (uint32_t i = 0; i < num_tiles; i++) {
        uint32_t tile_size;
        if (i == num_tiles - 1) {
            // Last tile
            tile_size = (add_size - (i * CPU_BUFFER_SIZE));
        } else {
            tile_size = CPU_BUFFER_SIZE;
        }

        // move data from A_ram and B_ram to cpu_buffer
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)cpu_buffer, (uint32_t)(A_ram + i * CPU_BUFFER_SIZE), tile_size, 1, dma_type, dma_type, 0);
        dma_copy((uint32_t)(cpu_buffer + tile_size), (uint32_t)(B_ram + i * CPU_BUFFER_SIZE), tile_size, 1, dma_type, dma_type, 0);
        timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        cpuAdd(cpu_buffer, cpu_buffer + tile_size, cpu_buffer + 2*tile_size, tile_size);
        timing->t_prc += timer_get_cycles() - timing->t_tmp1;
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)(R_ram + i * CPU_BUFFER_SIZE), (uint32_t)(cpu_buffer + 2*tile_size), tile_size, 0, dma_type, dma_type, 0);
        timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
        timing->n_dms++;
    }
}