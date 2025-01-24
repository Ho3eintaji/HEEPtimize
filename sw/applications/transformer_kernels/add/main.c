#include <stdio.h>
#include <stdlib.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "addNormC.h"
#include "data.h" // Include the generated header file

#define PRINT
#define DATA_SIZE (SEQ_LEN * INPUT_DIM)

int main() {
    
    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: add, SEQ_LEN: %d, INPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM);
    #endif

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    add(inputA, inputB, SEQ_LEN, INPUT_DIM); // Call the add function

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("add time: %d\n", time);
    #endif

    return 0;
}