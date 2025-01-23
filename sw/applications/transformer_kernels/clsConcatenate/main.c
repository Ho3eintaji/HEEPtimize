#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "tokenPosEmbeddingC.h"
#include "defines.h"

#define PRINT

#define SEQ_LEN 32
#define INPUT_DIM 64

#define INPUT_SIZE SEQ_LEN * INPUT_DIM

// allocate in this section: (section(".xheep_data_interleaved")
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) cls_token_vector[INPUT_DIM] = {0};

int main() {
    // Initialize input and cls_token_vector with some values
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = i; // Example initialization
    }
    for (int i = 0; i < INPUT_DIM; i++) {
        cls_token_vector[i] = i; // Example initialization
    }

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