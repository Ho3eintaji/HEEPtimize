// File: main.c
// Author: Francesco Poluzzi
// Date: 04/12/2024
// Description: Main file for the batch normalization application

#include <stdlib.h>
#include <stdio.h>
#include <math.h>  // Include for sqrtf
#include "heepatia.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "vcd_util.h"
#include "timer_sdk.h"
#include "ext_irq.h"
#include "data.h" // Assuming this contains A, R_cpu, W, B, Q, ELEM_SIZE, MUL, SHIFT, MUL_HQ
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "handler.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "hart.h"
#include "x-heep.h"
#include "w25q128jw.h" // If you need SPI Flash interactions
#include "defines.h" // For constants, potentially including DMA_DATA_TYPE_*
#include "core_v_mini_mcu.h"

#define MUL(x, y) (data_t_double) (((data_t_double)(x) * (data_t_double)(y)) >> Q)
#define MUL_HQ(x, y) (data_t_double) (((data_t_double)(x) * (data_t_double)(y)))
#define SHIFT(x) ((x) >> Q)


#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

// #define DEBUG
#define CHECK_RESULTS
#define CPU_BUFFER_KB (120 * 1024) // Adjust as needed. Same as your cpuAdd
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

// Original normalize function (for CPU reference)
void normalize(uint32_t row_a, uint32_t col_a, data_t *input, data_t *input_normalized, data_t *Weight, data_t *Bias);

// Tiled version of normalize
void normalizeTiled(uint32_t row_a, uint32_t col_a, data_t *A_ram, data_t *R_ram, data_t *W_ram, data_t *B_ram, timings_t * timing);


// cache: whole data is cached
data_t cache [A_ROWS*A_COLS + W_ROWS*W_COLS + B_ROWS*B_COLS + A_ROWS*A_COLS] = {0};
data_t *A_ram = cache; 
data_t *B_ram = cache + A_ROWS*A_COLS;
data_t *W_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS;
data_t *R_ram = cache + A_ROWS*A_COLS + B_ROWS*B_COLS + W_ROWS*W_COLS;

// CPU buffer
data_t cpu_buffer[CPU_BUFFER_SIZE] __attribute__((section(".xheep_data_interleaved"))) = {0};


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

    // intialize the timings recordings and allocate to zero
    timings_t * timing_cpu = (timings_t *)malloc(sizeof(timings_t));
    memset(timing_cpu, 0, sizeof(timings_t));

    /* ==============================
    * ====== Putting data in cache ======
    * ============================== */
    // move data from A and B which are in flash to A_ram and B_ram which are in ram
    timing_cpu->t_tmp1 = timer_get_cycles();
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((int32_t *)A), A_ram, A_ROWS*A_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(A_ram, A_ROWS*A_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((int32_t *)W), W_ram, W_ROWS*W_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(W_ram, W_ROWS*W_COLS*ELEM_SIZE);
    if (w25q128jw_read_quad_dma_async((int32_t)heep_get_flash_address_offset((int32_t *)B), B_ram, B_ROWS*B_COLS*ELEM_SIZE) != FLASH_OK)return -1;
    w25q128jw_wait_quad_dma_async(B_ram, B_ROWS*B_COLS*ELEM_SIZE);
    timing_cpu->t_flash = timer_get_cycles() - timing_cpu->t_tmp1;

    /* =======================================
    * ====== Runing on CPU =================
    * ======================================== */
    dma_sdk_init();

    t1 = timer_get_cycles();
    normalizeTiled((uint32_t)A_ROWS, (uint32_t)A_COLS, A_ram, R_ram, W_ram, B_ram, timing_cpu);
    // t_pe = timer_get_cycles() - t1;
    timing_cpu->t_tot = timer_get_cycles() - t1;

    PRINTF("R_ram[0]: %x\n", R_ram[0]);
    PRINTF("R_ram[%d]: %x\n", A_ROWS*A_COLS-1, R_ram[A_ROWS*A_COLS-1]);

    PRINTF("CPU-Norm: flash: %d, total: %d, prc: %d, dma_to: %d, dma_from: %d, n_dms: %d\n", timing_cpu->t_flash, timing_cpu->t_tot, timing_cpu->t_prc, timing_cpu->t_dma_to, timing_cpu->t_dma_from, timing_cpu->n_dms);

}

// layer normalization operation with an optional scaling (weighting) and bias on the input data
// void normalize(int16_t row_a, int16_t col_a, int16_t *input, int16_t *input_normalized) {
void normalize(uint32_t row_a, uint32_t col_a, data_t *input, data_t *input_normalized, data_t *Weight, data_t *Bias) {
    for (int i = 0; i < row_a; i++) { // iterates over the sequences (or batches)
        data_t *input_ptr = input + i * (col_a); // points to the current sequence in the input array.
        data_t *input_normalized_ptr = input_normalized + i * (col_a); // points to the corresponding location in the output array
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < col_a; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        data_t mean = (data_t)((float)sum / (float)col_a); 
        // compute variance
        input_ptr = input + i * (col_a);
        int64_t variance = 0;
        for (int j = 0; j < col_a; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(col_a);
        variance_float = variance_float / (float)(1 << Q);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        data_t sd_inv_int = (data_t)(sd_inv * (1 << Q));
        // Normalize Each Element and Apply Scale (Weight) and Bias
        input_ptr = input + i * (col_a);
        input_normalized_ptr = input_normalized + i * (col_a);
        for (int j = 0; j < col_a; j++) {
            *input_normalized_ptr = (data_t)MUL((*input_ptr - mean), sd_inv_int); // normalize by subtracting the mean and multiplying by the inverse of the standard deviation
            // After normalization, the result is scaled using the weight_ (γ) and bias_ (β) stored in addNorm
            *input_normalized_ptr = (data_t)(MUL((*input_normalized_ptr), Weight[j]) + Bias[j]);
            input_ptr++;
            input_normalized_ptr++;
        }
    }
}


void normalizeTiled(uint32_t row_a, uint32_t col_a, data_t *A_ram, data_t *R_ram, data_t *W_ram, data_t *B_ram, timings_t * timing){
    dma_data_type_t dma_type;
    switch (ELEM_SIZE) {
        case 1:  dma_type = DMA_DATA_TYPE_BYTE;       break;
        case 2:  dma_type = DMA_DATA_TYPE_HALF_WORD;  break;
        case 4:  dma_type = DMA_DATA_TYPE_WORD;       break;
        default: return; // Or handle error
    }
    // dma_type = DMA_DATA_TYPE_WORD;

    uint32_t max_rows = ((CPU_BUFFER_SIZE>>1) / col_a);
    uint32_t num_row_tiles = (row_a + max_rows - 1) / (max_rows); // Tiles based on rows
#ifdef DEBUG
    PRINTF("Number of row tiles: %d, max rows: %d\n", num_row_tiles, max_rows);
#endif

    for (uint32_t i = 0; i < num_row_tiles; i++) {
        uint32_t row_tile_size = (i == num_row_tiles - 1) ? (row_a - i * max_rows) : max_rows;
        uint32_t tile_size = row_tile_size * col_a; // the number of elements in this tile.
#ifdef DEBUG
        PRINTF("Row tile size: %d, tile size: %d\n", row_tile_size, tile_size);
#endif
        // DMA input tile
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)cpu_buffer, (uint32_t)(A_ram + i * max_rows * col_a), tile_size, 1, dma_type, dma_type, 0);
        timing->t_dma_to += timer_get_cycles() - timing->t_tmp1;

        // Run CPU normalize
        timing->t_tmp1 = timer_get_cycles();
        normalize(row_tile_size, col_a, cpu_buffer, cpu_buffer + tile_size, W_ram, B_ram);
        timing->t_prc += timer_get_cycles() - timing->t_tmp1;

        // DMA output tile
        timing->t_tmp1 = timer_get_cycles();
        dma_copy((uint32_t)(R_ram + i * max_rows * col_a), (uint32_t)(cpu_buffer + tile_size), tile_size, 0, dma_type, dma_type, 0);
        timing->t_dma_from += timer_get_cycles() - timing->t_tmp1;
        timing->n_dms += 1;
    }
}