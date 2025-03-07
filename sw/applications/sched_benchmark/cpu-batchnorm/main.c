// File: main.c
// Author: Francesco Poluzzi
// Date: 04/12/2024
// Description: Main file for the batch normalization application

#include <stdlib.h>
#include <stdio.h>

#include "heepatia.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "vcd_util.h"
#include "timer_sdk.h"
#include "ext_irq.h"
#include "data.h"
#include "math.h"

#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif

// #define DEBUG_DMA
#define DMA_CHANNEL 0
#define ERROR_TOLERANCE 2 // Tolerance for the error in the result (in Q format)

#define MUL(x, y) (int32_t) (((int32_t)(x) * (int32_t)(y)) >> Q)
#define MUL_HQ(x, y) (int32_t) (((int32_t)(x) * (int32_t)(y)))
#define SHIFT(x) ((x) >> Q)

data_t R_cpu[A_SIZE] __attribute__((section(".xheep_data_interleaved"))); // Result computed by the CPU

void normalize(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *input_normalized);

int main(void)
{
    dma_data_type_t dma_type = DMA_DATA_TYPE_WORD;
    dma_data_type_t dma_type_double;
    data_t *row_ptr;
    data_t_double *row_ptr_double;
    unsigned int a_rows = A_ROWS;
    unsigned int a_cols = A_COLS;
    unsigned int b_cols = B_COLS;
    uint32_t cpu_cycles;
    uint32_t dma_cycles_input_a;
    uint32_t dma_cycles_input_b;
    uint32_t dma_cycles_output;

    // System initialization
    // ---------------------
    // Enable fast interrupts for DMA and PLIC
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e)
        return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11); // 19: DMA, 11: PLIC
    CSR_SET_BITS(CSR_REG_MIE, mask);             // MIE.meie = 1

    // Initialize PLIC for external NM-Carus interrupt
    if (ext_irq_init() != 0)
        return 1;

    // Initialize the DMA
    dma_sdk_init();

    // Initialize the VCD trigger
    if (vcd_init() != 0)
        return 1;


    timer_cycles_init();
    timer_start();
    
    // Compute result on the CPU
    normalize(A_ROWS, A_COLS, A, R_cpu);

    // Stop timer and disable VCD dump
    cpu_cycles = timer_stop();

    // Skip verification part when running power simulation
#ifdef POWER_SIM
    return 0;
#endif

    // Print the number of CPU cycles
    printf("CPU: %u\n", cpu_cycles);


    // Return success
    return 0;
}

// layer normalization operation with an optional scaling (weighting) and bias on the input data
void normalize(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *input_normalized) {
    for (int i = 0; i < seq_len; i++) { // iterates over the sequences (or batches)
        int16_t *input_ptr = input + i * (input_dim); // points to the current sequence in the input array.
        int16_t *input_normalized_ptr = input_normalized + i * (input_dim); // points to the corresponding location in the output array
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < input_dim; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        int16_t mean = (int16_t)((float)sum / (float)input_dim); 
        // compute variance
        input_ptr = input + i * (input_dim);
        int64_t variance = 0;
        for (int j = 0; j < input_dim; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(input_dim);
        variance_float = variance_float / (float)(1 << Q);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        int16_t sd_inv_int = (int16_t)(sd_inv * (1 << Q));
        // Normalize Each Element and Apply Scale (Weight) and Bias
        input_ptr = input + i * (input_dim);
        input_normalized_ptr = input_normalized + i * (input_dim);
        for (int j = 0; j < input_dim; j++) {
            *input_normalized_ptr = (int16_t)MUL((*input_ptr - mean), sd_inv_int); // normalize by subtracting the mean and multiplying by the inverse of the standard deviation
            // After normalization, the result is scaled using the weight_ (γ) and bias_ (β) stored in addNorm
            *input_normalized_ptr = (int16_t)(MUL((*input_normalized_ptr), W[j]) + B[j]);
            input_ptr++;
            input_normalized_ptr++;
        }
    }
}