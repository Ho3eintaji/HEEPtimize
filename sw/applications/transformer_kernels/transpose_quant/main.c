#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "transposeC.h"
#include "defines.h"

#define PRINT

#define SEQ_LEN 32
#define INPUT_DIM 64

#define INPUT_SIZE SEQ_LEN * INPUT_DIM
#define OUTPUT_SIZE INPUT_SIZE

// allocate in this section: (section(".xheep_data_interleaved")
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) output[OUTPUT_SIZE] = {0};

int main() {
    // Initialize input with some values
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = i; // Example initialization
    }

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    transpose_quant(input, output, SEQ_LEN, INPUT_DIM);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("transpose_quant time: %d\n", time);
    #endif

    return 0;
}