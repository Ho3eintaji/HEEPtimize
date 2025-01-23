#include <stdio.h>
#include <stdlib.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "addNormC.h"
#include "data.h" // Include the generated header file

#define PRINT
#define DATA_SIZE (SEQ_LEN * INPUT_DIM)

int main() {
    // No need to read input data from files, data is already included in data.h

#ifdef PRINT
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    add(inputA, inputB, SEQ_LEN, INPUT_DIM); // Call the add function

#ifdef PRINT
    time = timer_stop();
    PRINTF("add time: %d\n", time);
#endif

    return 0;
}