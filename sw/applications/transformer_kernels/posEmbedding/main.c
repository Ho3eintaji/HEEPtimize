#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "tokenPosEmbeddingC.h"
#include "defines.h"
#include "data.h"

#define PRINT

int main() {

    TokenPosEmbedding tokenPosEmbedding;
    createTokenPosEmbedding(&tokenPosEmbedding, pos_matrix, cls_token_vector, SEQ_LEN, INPUT_DIM, SEQ_LEN + 1);

    #ifdef PRINT
        timer_cycles_init();
        int time = 0;
        timer_start();
    #endif

    posEmbedding(&tokenPosEmbedding, input);

    #ifdef PRINT
        time = timer_stop();
        PRINTF("posEmbedding time: %d\n", time);
    #endif

    return 0;
}