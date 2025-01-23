#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "tokenPosEmbeddingC.h"
#include "defines.h"
#include "data.h" // Include the generated header file

#define PRINT

int main() {
    // No need to read input data from files, data is already included in data.h

    TokenPosEmbedding tokenPosEmbedding;
    createTokenPosEmbedding(&tokenPosEmbedding, NULL, cls_token_vector, SEQ_LEN, INPUT_DIM, SEQ_LEN + 1);

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    clsConcatenate(&tokenPosEmbedding, input, input);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("clsConcatenate time: %d\n", time);
    #endif

    return 0;
}