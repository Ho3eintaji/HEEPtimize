#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "transposeC.h"
#include "defines.h"
#include "data.h"
#include "vcd_util.h"

#define PRINT

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: multihead_transpose, SEQ_LEN: %d, HEAD_HIDDEN_SIZE: %d, NUM_HEADS: %d\n", SEQ_LEN, HEAD_HIDDEN_SIZE, NUM_HEADS);
    #endif

    if (vcd_init() != 0) return 1;
    
    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    vcd_enable();
    multihead_transpose(input, output, SEQ_LEN, HEAD_HIDDEN_SIZE, NUM_HEADS);
    vcd_disable();

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("multihead_transpose time: %d\n", time);
    #endif

    return 0;
}