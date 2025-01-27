#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "matMulC.h"
#include "data.h"
#include "vcd_util.h"

#define PRINT

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: MatMul_multiply, SEQ_LEN: %d, INPUT_DIM: %d, OUTPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM, OUTPUT_DIM);
    #endif

    if (vcd_init() != 0) return 1;

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

        vcd_enable();
        MatMul_multiply(SEQ_LEN, input, weight, output, INPUT_DIM, OUTPUT_DIM);
        vcd_disable();

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("MatMul_multiply time: %d\n", time);
    #endif
    return 0;
}