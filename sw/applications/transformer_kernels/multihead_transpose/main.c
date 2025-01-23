#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "transposeC.h"
#include "defines.h"
#include "data.h"

#define PRINT

int main() {

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