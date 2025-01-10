#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "addNormC.h"

#define PRINT

#define SEQ_LEN 128
#define INPUT_DIM 64

#define INPUT_SIZE SEQ_LEN * INPUT_DIM
#define WEIGHT_SIZE INPUT_DIM
#define BIAS_SIZE INPUT_DIM




int main() {

    // allocate in this section: (section(".xheep_data_interleaved")
    quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
    quant_bit_width __attribute__((section(".xheep_data_interleaved"))) weight[WEIGHT_SIZE] = {0};
    quant_bit_width __attribute__((section(".xheep_data_interleaved"))) bias[BIAS_SIZE] = {0};
    quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input_normalized[INPUT_SIZE] = {0};

    int seq_len = SEQ_LEN; // Example sequence length
    int input_dim = INPUT_DIM; // Example input dimension

    // Initialize input, weight, and bias with some values
    for (int i = 0; i < seq_len * input_dim; i++) {
        input[i] = i % 256; // Example initialization
    }
    for (int i = 0; i < input_dim; i++) {
        weight[i] = 1; // Example initialization
        bias[i] = 0; // Example initialization
    }

    // Create AddNormalize structure
    AddNormalize addNorm = createAddNormalize(seq_len, input_dim, weight, bias);


    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    normalize(&addNorm, input, input_normalized); 
    
    #ifdef PRINT
        time = timer_stop();
        PRINTF("normalize time: %d\n", time);
    #endif

    return 0;
}
