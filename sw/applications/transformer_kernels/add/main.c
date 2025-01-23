#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "addNormC.h"

#define PRINT
#define DATA_SIZE 64

quant_bit_width __attribute__((section(".xheep_data_interleaved"))) inputA[DATA_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) inputB[DATA_SIZE] = {0};

int main() {
    for (int i = 0; i < DATA_SIZE; i++) {
        inputA[i] = i;
        inputB[i] = 2;
    }

#ifdef PRINT
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    add(inputA, inputB, 1, DATA_SIZE); // Call the add function

#ifdef PRINT
    time = timer_stop();
    PRINTF("add time: %d\n", time);
#endif

    return 0;
}