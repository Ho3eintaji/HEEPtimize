#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "transposeC.h"
#include "defines.h"
#include "data.h"
#include "vcd_util.h"

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: transpose, SEQ_LEN: %d, INPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM);
    #endif

    if (vcd_init() != 0) return 1;

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    vcd_enable();
    transpose_quant(input, output, SEQ_LEN, INPUT_DIM);
    vcd_disable();

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("transpose time: %d\n", time);
    #endif

    return 0;
}