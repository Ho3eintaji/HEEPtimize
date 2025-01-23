#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "transposeC.h"
#include "defines.h"

#define PRINT

#define SEQ_LEN 32
#define HEAD_HIDDEN_SIZE 64
#define NUM_HEADS 8

#define INPUT_SIZE SEQ_LEN * HEAD_HIDDEN_SIZE * NUM_HEADS
#define OUTPUT_SIZE SEQ_LEN * HEAD_HIDDEN_SIZE * NUM_HEADS

// allocate in this section: (section(".xheep_data_interleaved")
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) output[OUTPUT_SIZE] = {0};

int main() {
    // Initialize input with some values
    for (int i = 0; i < INPUT_SIZE-1; i++) {
        input[i] = i; // Example initialization
    }

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    multihead_transpose(input, output, SEQ_LEN, HEAD_HIDDEN_SIZE, NUM_HEADS);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("multihead_transpose time: %d\n", time);
    #endif

    return 0;
}