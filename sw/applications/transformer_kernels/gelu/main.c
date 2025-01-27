#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "dense_layerC.h"
#include "data.h"
#include "vcd_util.h"

#define PRINT

int main() {
    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: activation(gelu), DATA_SIZE: %d\n", DATA_SIZE);
    #endif

    if (vcd_init() != 0) return 1;

    Dense dense;
    createDense(&dense, DATA_SIZE, DATA_SIZE, NULL, NULL); // Initialize Dense structure

#ifdef PRINT_TOTAL_CYCLES
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    vcd_enable();
    activation(&dense, DATA_SIZE, input, output);
    vcd_disable();

#ifdef PRINT_TOTAL_CYCLES
    time = timer_stop();
    PRINTF("activation time: %d\n", time);
#endif

    destroyDense(&dense); // Clean up Dense structure
    return 0;
}