#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "matMulC.h"
#include "data.h"
#include "vcd_util.h"

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: MatMul_scale, MAT_SIZE: %d, SHIFT_SCALE: %d\n", MAT_SIZE, SHIFT_SCALE);
    #endif

    if (vcd_init() != 0) return 1;

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    vcd_enable();
        MatMul_scale(input, SHIFT_SCALE, MAT_SIZE);
    vcd_disable();

    #ifdef PRINT
        time = timer_stop();
        PRINTF("MatMul_scale time: %d\n", time);
    #endif
    return 0;
}