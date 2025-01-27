#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "softmaxC.h"
#include "data.h"
#include "vcd_util.h"

#define PRINT

int main() {
    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: softmax, SEQ_LEN: %d\n", SEQ_LEN);
    #endif

    if (vcd_init() != 0) return 1;

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

        vcd_enable();
        computeSoftmax(input, SEQ_LEN);
        vcd_disable();

    #ifdef PRINT
        time = timer_stop();
        PRINTF("softmax time: %d\n", time);
    #endif
    return 0;
}