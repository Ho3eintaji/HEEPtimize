#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "defines.h"
#include "matMulC.h"

#define PRINT
#define SEQ_LEN 4
#define INPUT_SIZE 8
#define OUTPUT_SIZE 8

quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[SEQ_LEN * INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) weight[INPUT_SIZE * OUTPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) output[SEQ_LEN * OUTPUT_SIZE] = {0};

int main() {
    for (int i = 0; i < SEQ_LEN * INPUT_SIZE; i++) {
        input[i] = i;
    }
    for (int i = 0; i < INPUT_SIZE * OUTPUT_SIZE; i++) {
        weight[i] = 1;
    }

#ifdef PRINT
    timer_cycles_init();
    int time = 0;
    timer_start();
#endif

    MatMul_multiply(SEQ_LEN, input, weight, output, INPUT_SIZE, OUTPUT_SIZE);

#ifdef PRINT
    time = timer_stop();
    PRINTF("MatMul_multiply time: %d\n", time);
#endif
    return 0;
}