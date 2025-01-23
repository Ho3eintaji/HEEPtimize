#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "matMulC.h"

#define PRINT
#define MAT_SIZE 64
#define SHIFT_SCALE 2

quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[MAT_SIZE] = {0};

int main() {
    for (int i = 0; i < MAT_SIZE; i++) {
        input[i] = i;
    }

#ifdef PRINT
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    MatMul_scale(input, SHIFT_SCALE, MAT_SIZE);

#ifdef PRINT
    time = timer_stop();
    PRINTF("MatMul_scale time: %d\n", time);
#endif
    return 0;
}