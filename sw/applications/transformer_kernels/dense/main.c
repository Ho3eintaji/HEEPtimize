#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "dense_layerC.h"
#include "defines.h"
#include "data.h" // Include the generated header file

#define PRINT

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: dense, SEQ_LEN: %d, INPUT_DIM: %d, OUTPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM, OUTPUT_DIM);
    #endif

    // Create Dense structure
    Dense dense;
    createDense(&dense, INPUT_DIM, OUTPUT_DIM, weight, bias);

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    computeDense(&dense, SEQ_LEN, input, output);

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("computeDense time: %d\n", time);
    #endif

    return 0;
}
