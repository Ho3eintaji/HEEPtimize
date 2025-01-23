#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "matMulC.h"
#include "data.h"

#define PRINT

int main() {

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