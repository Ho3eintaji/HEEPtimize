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

    transpose_quant(input, output, SEQ_LEN, INPUT_DIM);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("transpose_quant time: %d\n", time);
    #endif

    return 0;
}