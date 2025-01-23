#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "softmaxC.h"

#define PRINT
#define SEQ_LEN 8

int16_t __attribute__((section(".xheep_data_interleaved"))) input[SEQ_LEN * SEQ_LEN] = {0};

int main() {
    for (int i = 0; i < SEQ_LEN * SEQ_LEN; i++) {
        input[i] = (int16_t)i;
    }

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