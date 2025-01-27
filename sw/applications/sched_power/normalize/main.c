// Author: Hossein Taji

#include <stdlib.h>
#include <stdio.h>
#include "heepatia.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "vcd_util.h"
#include "timer_sdk.h"
#include "ext_irq.h"
#include "carus.h"
#include "data.h"
#include "math.h"
#include "carus_batchnorm_multivector.h"

#ifdef POWER_SIM
#pragma message "Power simulation ENABLED: disabling verification checks"
#endif


// Definitions similar to carus-batchnorm
#define CARUS_INSTANCE 0
#define CARUS_VREG_SIZE 2048
#define DMA_CHANNEL 0
#define ERROR_TOLERANCE 2

#define MUL(x, y) (int32_t)(((int32_t)(x) * (int32_t)(y)) >> Q)
#define MUL_HQ(x, y) (int32_t)(((int32_t)(x) * (int32_t)(y)))
#define SHIFT(x) ((x) >> Q)

// Arrays to hold CPU outputs
data_t R_cpu[A_SIZE] __attribute__((section(".xheep_data_interleaved")));

void normalize(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *input_normalized);
void prepare_normalize_carus(int16_t seq_len, int16_t input_dim, int16_t *input);

dma_config_flags_t run_dma_signext_trans(dma_trans_t *trans)
    {
    dma_config_flags_t res1, res2, res3;

    res1 = dma_validate_transaction(trans, DMA_ENABLE_REALIGN, DMA_PERFORM_CHECKS_INTEGRITY);
    if(res1 != DMA_CONFIG_OK)
    {
        printf("Error in dma_validate_transaction 0x%x \n",res1);
    }

    res2 = dma_load_transaction(trans);
    if(res2 != DMA_CONFIG_OK)
    {
        printf("Error in dma_load_transaction 0x%x \n",res2);
    }

    res3 |= dma_launch(trans);
    if(res3 != DMA_CONFIG_OK)
    {
        printf("Error in dma_launch 0x%x \n",res3);
    }
    return res1|res2|res3;
} 


// Full main function
int main(void) {
    carus_cfg_t cfg = CARUS_CFG_INIT(0); // NM-Carus configuration
    dma_data_type_t dma_type = DMA_DATA_TYPE_WORD;
    dma_data_type_t dma_type_double;
    data_t *row_ptr;
    data_t_double *row_ptr_double;
    unsigned int a_rows = A_ROWS;
    unsigned int a_cols = A_COLS;
    unsigned int b_cols = B_COLS;

    // System initialization, interrupts, etc.
    // ---------------------------------------
    if (enable_fast_interrupt(kDma_fic_e, true) != kFastIntrCtrlOk_e) return 1;
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = (1 << 19) | (1 << 11);
    CSR_SET_BITS(CSR_REG_MIE, mask);
    if (ext_irq_init() != 0) return 1;
    dma_sdk_init();
    if (vcd_init() != 0) return 1;

    // Initialize NM-Carus
    // -------------------
    if (carus_init(CARUS_INSTANCE) != 0) return 1;
    if (carus_load_kernel(CARUS_INSTANCE, carus_batchnorm_multivector, CARUS_BATCHNORM_MULTIVECTOR_SIZE, 0) != 0){
        printf("Error loading kernel\n");
        return 1;
    }

    // Set kernel configuration configuration
    cfg.vl = VL;
    switch (ELEM_SIZE)
    {
    case 1:
        cfg.vtype = VTYPE_VSEW_16;
        dma_type = DMA_DATA_TYPE_BYTE;
        dma_type_double = DMA_DATA_TYPE_HALF_WORD;
        break;
    case 2:
        cfg.vtype = VTYPE_VSEW_32;
        dma_type = DMA_DATA_TYPE_HALF_WORD;
        dma_type_double = DMA_DATA_TYPE_WORD;
        break;
    default:
        printf("Error: unsupported element size\n");
        return 1;
    }

    cfg.arg0 = ARG0; // n. vectors - 1  OR n.input_dim - 1
    if (carus_set_cfg(CARUS_INSTANCE, &cfg) != 0) return 1;

    // Copy input matrix A
    row_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, 0 );
    dma_target_t tgt_src_A = {
        .ptr = (uint8_t *) A ,
        .inc_d1_du = 1,
        .inc_d2_du = 1,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_A = {
        .ptr = (uint8_t *) &row_ptr[0],
        .inc_d1_du = 1,
        .inc_d2_du = (CARUS_VREG_SIZE/(2*ELEM_SIZE)) - A_COLS +1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_A = {
        .src = &tgt_src_A,
        .dst = &tgt_dst_A,
        .size_d1_du = A_COLS ,
        .size_d2_du = A_ROWS,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .dim = DMA_DIM_CONF_2D,
        .win_du = 0,
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR,
        .channel = DMA_CHANNEL,
    };
    if (run_dma_signext_trans(&trans_A) != 0)
    {
        printf("Error! DMA transaction A failed\n");
        return 1;
    }
    DMA_WAIT(DMA_CHANNEL)

    // copy weight and bias
    data_t* weight_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, GAMMA_VECTOR );
    data_t* bias_ptr = (data_t *)carus_vrf(CARUS_INSTANCE, BETA_VECTOR );
    dma_target_t tgt_src_W = {
        .ptr = (uint8_t *) W,
        .inc_d1_du = 1,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_W = {
        .ptr = (uint8_t *) weight_ptr,
        .inc_d1_du = 1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_W = {
        .src = &tgt_src_W,
        .dst = &tgt_dst_W,
        .size_d1_du = W_SIZE,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR,
        .channel = DMA_CHANNEL,
    };
    if (run_dma_signext_trans(&trans_W) != 0)
    {
        printf("Error! DMA transaction A failed\n");
        return 1;
    }
    DMA_WAIT(DMA_CHANNEL)

        dma_target_t tgt_src_B = {
        .ptr = (uint8_t *) B,
        .inc_d1_du = 1,
        .type = dma_type,
        .trig = DMA_TRIG_MEMORY,   
    };
    dma_target_t tgt_dst_B = {
        .ptr = (uint8_t *) bias_ptr,
        .inc_d1_du = 1,
        .type = dma_type_double,
        .trig = DMA_TRIG_MEMORY,    
    };
    dma_trans_t trans_B = {
        .src = &tgt_src_B,
        .dst = &tgt_dst_B,
        .size_d1_du = B_SIZE,
        .src_addr = NULL,
        .mode = DMA_TRANS_MODE_SINGLE,
        .win_du = 0,
        .sign_ext = 1, // This flag enables sign extension!
        .end = DMA_TRANS_END_INTR,
        .channel = DMA_CHANNEL,
    };
    if (run_dma_signext_trans(&trans_B) != 0)
    {
        printf("Error! DMA transaction A failed\n");
        return 1;
    }
    DMA_WAIT(DMA_CHANNEL)
    

    // Prepare data for normalization
    // ------------------------------
    prepare_normalize_carus(A_ROWS, A_COLS, A);

    // Carus-based run
    // ---------------
    printf("Starting power measurement for %s\n", POWER_TARGET);
    
    // if (POWER_TARGET == carus) {
        timer_cycles_init();
        timer_start();
        vcd_enable();
        if (carus_run_kernel(CARUS_INSTANCE) != 0){
            printf("Error running kernel\n");
            return 1;
        }
        if (carus_wait_done(CARUS_INSTANCE) != 0)
            return 1;
        vcd_disable();
        uint32_t carus_cycles = timer_stop();


    // } else if (POWER_TARGET == cpu) {
        timer_cycles_init();
        timer_start();
        // I want to give one tenth of carus load to cpu
        unsigned int a_cols_cpu = A_COLS / 9;
        unsigned int a_rows_cpu = A_ROWS / 2;
        vcd_enable();
        normalize(a_rows_cpu, a_cols_cpu, A, R_cpu);
        vcd_disable();
        uint32_t cpu_cycles = timer_stop();


    // } else if (POWER_TARGET == carus-cpu) {
        if (carus_load_kernel(CARUS_INSTANCE, carus_batchnorm_multivector, CARUS_BATCHNORM_MULTIVECTOR_SIZE, 0) != 0){
            printf("Error loading kernel\n");
            return 1;
        }
        timer_cycles_init();
        timer_start();
        vcd_enable();
        if (carus_run_kernel(CARUS_INSTANCE) != 0){
            printf("Error running kernel\n");
            return 1;
        }
        normalize(a_rows_cpu, a_cols_cpu, A, R_cpu);
        if (carus_wait_done(CARUS_INSTANCE) != 0)
            return 1;
        uint32_t carus_cpu_cycles = timer_stop();
        vcd_disable();

#ifdef POWER_SIM
    return 0;
#endif

    // printf("CPU: %u\n", cpu_cycles);
    // printf("Carus: %u\n", carus_cycles);
    // printf("Carus-CPU: %u\n", carus_cpu_cycles);

    // // Optional verification checks
    // // ----------------------------
    //     // Check the output data
    // // ---------------------
    // // Check NM-Carus output data
    // for (unsigned int i = 0; i < A_ROWS; i++)
    // {
    //     int16_t* row_ptr_16 = (int16_t *)carus_vrf(CARUS_INSTANCE, i);
    //     for (unsigned int j = 0; j < A_COLS; j++)
    //     {
    //         if (abs(row_ptr_16[2*j] - R_cpu[i * A_COLS + j]) > ERROR_TOLERANCE)
    //         {
    //             printf("NMC|gold R[%u,%u]: %x %x\n", i, j, row_ptr_16[2*j], R_cpu[i * A_COLS + j]);
    //             // return 1;
    //         }
    //     }
    // }

    return 0;
}

// layer normalization operation with an optional scaling (weighting) and bias on the input data
void normalize(int16_t seq_len, int16_t input_dim, int16_t *input, int16_t *input_normalized) {
    for (int i = 0; i < seq_len; i++) { // iterates over the sequences (or batches)
        int16_t *input_ptr = input + i * (input_dim); // points to the current sequence in the input array.
        int16_t *input_normalized_ptr = input_normalized + i * (input_dim); // points to the corresponding location in the output array
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < input_dim; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        int16_t mean = (int16_t)((float)sum / (float)input_dim); 
        // compute variance
        input_ptr = input + i * (input_dim);
        int64_t variance = 0;
        for (int j = 0; j < input_dim; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(input_dim);
        variance_float = variance_float / (float)(1 << Q);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        int16_t sd_inv_int = (int16_t)(sd_inv * (1 << Q));
        // Normalize Each Element and Apply Scale (Weight) and Bias
        input_ptr = input + i * (input_dim);
        input_normalized_ptr = input_normalized + i * (input_dim);
        for (int j = 0; j < input_dim; j++) {
            *input_normalized_ptr = (int16_t)MUL((*input_ptr - mean), sd_inv_int); // normalize by subtracting the mean and multiplying by the inverse of the standard deviation
            // After normalization, the result is scaled using the weight_ (γ) and bias_ (β) stored in addNorm
            *input_normalized_ptr = (int16_t)(MUL((*input_normalized_ptr), W[j]) + B[j]);
            input_ptr++;
            input_normalized_ptr++;
        }
    }
}

// layer normalization operation with an optional scaling (weighting) and bias on the input data
void prepare_normalize_carus(int16_t seq_len, int16_t input_dim, int16_t *input) {
    int32_t* mean_ptr = (int32_t *)carus_vrf(CARUS_INSTANCE, MEAN_VECTOR );
    int32_t* variance_ptr = (int32_t *)carus_vrf(CARUS_INSTANCE, VAR_VECTOR );
    // compute means and variances
    for (int i = 0; i < seq_len; i++) { // iterates over the sequences (or batches)
        int16_t *input_ptr = input + i * (input_dim); // points to the current sequence in the input array.
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < input_dim; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        int16_t mean = (int16_t)((float)sum / (float)input_dim); 
        // compute variance
        input_ptr = input + i * (input_dim);
        int64_t variance = 0;
        for (int j = 0; j < input_dim; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(input_dim);
        variance_float = variance_float / (float)(1 << Q);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        int16_t sd_inv_int = (int16_t)(sd_inv * (1 << Q));
        mean_ptr[i] = mean;
        variance_ptr[i] = sd_inv_int;
    }
}
