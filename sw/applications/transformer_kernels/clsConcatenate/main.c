#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "tokenPosEmbeddingC.h"
#include "defines.h"
#include "data.h" // Include the generated header file
#include "vcd_util.h"

#define PRINT

int main() {
    #ifdef DEBUG_PRINTS
        PRINTF("Kernel: clsConcatenate, SEQ_LEN: %d, INPUT_DIM: %d\n", SEQ_LEN, INPUT_DIM);
    #endif

    if (vcd_init() != 0) return 1;

    TokenPosEmbedding tokenPosEmbedding;
    createTokenPosEmbedding(&tokenPosEmbedding, NULL, cls_token_vector, SEQ_LEN, INPUT_DIM, SEQ_LEN + 1);

    #ifdef PRINT_TOTAL_CYCLES
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    vcd_enable();
    clsConcatenate(&tokenPosEmbedding, input, input);
    vcd_disable();

    #ifdef PRINT_TOTAL_CYCLES
        time = timer_stop();
        PRINTF("clsConcatenate time: %d\n", time);
    #endif

    return 0;
}