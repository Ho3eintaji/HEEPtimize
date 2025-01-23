#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "dense_layerC.h"
#include "defines.h"

#define PRINT

#define SEQ_LEN 32
#define INPUT_DIM 64
#define OUTPUT_DIM 128

#define INPUT_SIZE SEQ_LEN * INPUT_DIM
#define OUTPUT_SIZE SEQ_LEN * OUTPUT_DIM
#define WEIGHT_SIZE INPUT_DIM * OUTPUT_DIM
#define BIAS_SIZE OUTPUT_DIM

// allocate in this section: (section(".xheep_data_interleaved")
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) weight[WEIGHT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) bias[BIAS_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) output[OUTPUT_SIZE] = {0};

int main() {

    // Initialize input, weight, and bias with some values
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = i; // Example initialization
    }
    for (int i = 0; i < WEIGHT_SIZE; i++) {
        weight[i] = 1; // Example initialization
    }
    for (int i = 0; i < BIAS_SIZE; i++) {
        bias[i] = 0; // Example initialization
    }

    // Create Dense structure
    Dense dense;
    createDense(&dense, INPUT_DIM, OUTPUT_DIM, weight, bias);

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    computeDense(&dense, [SEQ_LEN], input, output);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("computeDense time: %d\n", time);
    #endif

    return 0;
}
