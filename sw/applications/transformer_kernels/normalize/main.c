#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "addNormC.h"
#include "defines.h"
#include "data.h"

#define PRINT

int main() {

    // Create AddNormalize structure
    AddNormalize addNorm = createAddNormalize(SEQ_LEN, INPUT_DIM, weight, bias);


    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    normalize(&addNorm, input, input_normalized); 
    
    #ifdef PRINT
        time = timer_stop();
        PRINTF("normalize time: %d\n", time);
    #endif

    return 0;
}