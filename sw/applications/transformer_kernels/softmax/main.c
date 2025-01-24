#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "softmaxC.h"
#include "data.h"

#define PRINT

int main() {
    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: softmax, SEQ_LEN: %d\n", SEQ_LEN);
    #endif

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

        computeSoftmax(input, SEQ_LEN);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("softmax time: %d\n", time);
    #endif
    return 0;
}