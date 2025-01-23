#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "dense_layerC.h"
#include "data.h"

#define PRINT

int main() {
    Dense dense;
    createDense(&dense, DATA_SIZE, DATA_SIZE, NULL, NULL); // Initialize Dense structure

#ifdef PRINT
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    activation(&dense, DATA_SIZE, input, output);

#ifdef PRINT
    time = timer_stop();
    PRINTF("activation time: %d\n", time);
#endif

    destroyDense(&dense); // Clean up Dense structure
    return 0;
}