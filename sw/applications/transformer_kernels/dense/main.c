#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "dense_layer.h"
#include "defines.h"
#include "data.h" // Include the generated header file
#include "vcd_util.h"

#define PRINT

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: dense, SEQ_LEN: %d, INPUT_DIM: %d, OUTPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM, OUTPUT_DIM);
    #endif

    if (vcd_init() != 0) return 1;
    system_initialization();
    int time = 0;

    // Create Dense structure
    Dense dense;
    createDense(&dense, INPUT_DIM, OUTPUT_DIM, weight, bias);

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        time = 0;
        timer_start();
    #endif
    computeDense_carus(&dense, SEQ_LEN, input, output);
    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("computeDense carus: %d\n", time);
    #endif

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        time = 0;
        timer_start();
    #endif

    vcd_enable();
    computeDense(&dense, SEQ_LEN, input, output);
    vcd_disable();

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("computeDense cpu: %d\n", time);
    #endif



    return 0;
}
