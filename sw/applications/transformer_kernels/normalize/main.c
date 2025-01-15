#include <stdio.h>
#include "timer_sdk.h"
#include "core_v_mini_mcu.h"
#include "addNormC.h"
#include "defines.h"

#define PRINT

#define SEQ_LEN 32//128
#define INPUT_DIM 64

#define INPUT_SIZE SEQ_LEN * INPUT_DIM
#define WEIGHT_SIZE INPUT_DIM
#define BIAS_SIZE INPUT_DIM

// allocate in this section: (section(".xheep_data_interleaved")
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input[INPUT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) weight[WEIGHT_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) bias[BIAS_SIZE] = {0};
quant_bit_width __attribute__((section(".xheep_data_interleaved"))) input_normalized[INPUT_SIZE] = {0};


int main() {

    // Initialize input, weight, and bias with some values
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = i; // Example initialization
    }
    for (int i = 0; i < INPUT_DIM; i++) {
        weight[i] = 1; // Example initialization
        bias[i] = 0; // Example initialization
    }

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


    // quant_bit_width *ptr_input = input;

    // for (quant_bit_width i = 0; i < 20; i++) {
    //     input[i] = i;
    // }
    // // Print input for the int16_t format
    // for (int i = 0; i < 5; i++) {
    //     printf("%d\n", *ptr_input);
    //     ptr_input = ptr_input + 1;        
    // }