//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include "add.h"
#include "carus.h"

// Add operation between two arrays; takes care of overflow
void add(quant_bit_width *input, quant_bit_width *to_be_added, int seq_len, int input_dim) {
    int32_t sum;
    for (int i = 0; i < seq_len * input_dim; i++) {
        sum = input[i] + to_be_added[i];
        if ((quant_bit_width)sum != sum) // In case of overflow in 16 bits
            input[i] = (sum > 0) ? INT16_MAX : INT16_MIN;
        else
            input[i] = (quant_bit_width)sum;
    }
}

// Add operation between two arrays performed with NM Carus
void add_carus(quant_bit_width *input, quant_bit_width *to_be_added, int seq_len, int input_dim) {
    
    carus_cfg_t cfg = CARUS_CFG_INIT(CARUS_ADD_INSTANCE);
    int32_t size = seq_len * input_dim;
    cfg.vl =(uint32_t) size;
    cfg.vtype =(uint32_t) VTYPE_VSEW_16;
    cfg.koffs =(uint32_t) CARUS_ADD_OFFSET;
    carus_set_cfg(CARUS_ADD_INSTANCE, &cfg);
    carus_dma_move_input_flattended_no_signext(size, input, 0, 0, CARUS_ADD_INSTANCE, 1);
    DMA_WAIT(1);
    carus_dma_move_input_flattended_no_signext(size, to_be_added, 0, 10, CARUS_ADD_INSTANCE, 1);
    DMA_WAIT(1);
    carus_run_kernel(CARUS_ADD_INSTANCE);
    carus_wait_done(CARUS_ADD_INSTANCE) ;
    carus_dma_move_output_flattended_no_signext(size, input, 0, 20, CARUS_ADD_INSTANCE, 1);
    DMA_WAIT(1);

}
