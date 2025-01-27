#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "addNormC.h"
#include "defines.h"
#include "data.h"
#include "vcd_util.h"

int main() {

    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: normalize, SEQ_LEN: %d, INPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM);
    #endif

    if (vcd_init() != 0) return 1;

    // Create AddNormalize structure
    AddNormalize addNorm = createAddNormalize(SEQ_LEN, INPUT_DIM, weight, bias);


    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    vcd_enable();
    normalize(&addNorm, input, input_normalized); 
    vcd_disable();
    
    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("normalize time: %d\n", time);
    #endif

    return 0;
}