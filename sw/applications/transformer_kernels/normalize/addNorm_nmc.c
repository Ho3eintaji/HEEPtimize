//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include "addNorm_nmc.h"
#include "carus.h"

// Function implementations
AddNormalize createAddNormalize(int seq_len, int input_dim, quant_bit_width *weight, quant_bit_width *bias) {
    AddNormalize addNorm;
    addNorm.seq_len_ = seq_len;
    addNorm.input_dim_ = input_dim;
    addNorm.weight_ = weight;
    addNorm.bias_ = bias;
    return addNorm;
}

#define MAX_TILE_SIZE 28 // amount of vector registers for the input data in the kernel
// layer normalization operation with an optional scaling (weighting) and bias on the input data, performed in Carus
void normalize_carus(AddNormalize *addNorm, quant_bit_width *input, quant_bit_width *input_normalized) { 
#ifdef USE_2_CARUS_INSTANCES
    // compute tiling parameters
    int16_t tile_size = (addNorm->seq_len_ <= MAX_TILE_SIZE) ? addNorm->seq_len_ : MAX_TILE_SIZE;
    int16_t n_tiles = CEIL_INT_DIV(addNorm->seq_len_ , tile_size);
    int16_t last_tile_size = addNorm->seq_len_ % tile_size;
    int8_t carus_instance = 0;
    int16_t vl = (tile_size > addNorm->input_dim_)? tile_size : addNorm->input_dim_;
    // set Carus configuration
    carus_cfg_t cfg = CARUS_CFG_INIT(carus_instance);
    cfg.vl =(uint32_t) vl;
    cfg.vtype =(uint32_t) VTYPE_VSEW_32;
    cfg.koffs =(uint32_t) CARUS_BATCHNORM_MULTIVECTOR_OFFSET;
    cfg.arg0 =(uint32_t) tile_size;
    carus_set_cfg(carus_instance, &cfg);
    if(n_tiles>1)carus_set_cfg(!carus_instance, &cfg);
    // move weights to Carus (only once)
    carus_write_flatten_matrix(addNorm->input_dim_, addNorm->weight_, 0, GAMMA_VECTOR, carus_instance, 1); // move weights
    if(n_tiles>1)carus_write_flatten_matrix(addNorm->input_dim_, addNorm->weight_, 0, GAMMA_VECTOR, !carus_instance, 2); // move weights
    DMA_WAIT(1);
    DMA_WAIT(2);

    // move biases to Carus (only once)
    carus_write_flatten_matrix(addNorm->input_dim_, addNorm->bias_, 0, BETA_VECTOR, carus_instance, 1); // move bias
    if(n_tiles>1)carus_write_flatten_matrix(addNorm->input_dim_, addNorm->bias_, 0, BETA_VECTOR, !carus_instance, 2); // move bias
    DMA_WAIT(1);
    DMA_WAIT(2);
    // move input data to Carus for the first tile
    if(tile_size!=1)carus_write_matrix(tile_size, addNorm->input_dim_, input, 0, 0, carus_instance, 1); 
    else carus_write_flatten_matrix(addNorm->input_dim_, input, 0, 0, carus_instance, 1);
    DMA_WAIT(1);
    // compute mean and variance for the first tile
    compute_mean_stdev(tile_size, addNorm->input_dim_, input, (int32_t*)carus_vrf(carus_instance, MEAN_VECTOR), (int32_t*)carus_vrf(carus_instance, VAR_VECTOR));// mean and variances for first tile
    for(int t = 0; t< n_tiles; t++){ // iterate over the tiles
        if(t==n_tiles-1 && t!= 0){ // reduce kenrel size for last tile
            cfg.arg0 = last_tile_size;
            carus_set_cfg(carus_instance, &cfg);
        }
        // start the normalization
        carus_run_kernel(carus_instance);
        int16_t this_tile_size = (t==n_tiles-1 && last_tile_size!=0) ? last_tile_size : tile_size;
        int16_t next_tile_size = (t==n_tiles-2 && last_tile_size!=0) ? last_tile_size : tile_size;
        // compute mean and variance for the next tile while Carus is doing the normalization on the current tile
        if(t<n_tiles-1)compute_mean_stdev(next_tile_size, addNorm->input_dim_, input + (t+1)*addNorm->input_dim_*tile_size, (int32_t*)carus_vrf(!carus_instance, MEAN_VECTOR), (int32_t*)carus_vrf(!carus_instance, VAR_VECTOR)); // compute mean and variance for the next tile
        carus_wait_done(carus_instance);
        // move the normalized data back to the CPU
        if(this_tile_size!=1)carus_read_matrix(this_tile_size, addNorm->input_dim_, input_normalized + t*addNorm->input_dim_*tile_size, 0, 0, carus_instance, 1); // move outputs for current tile
        else carus_read_flatten_matrix(addNorm->input_dim_, input_normalized + t*addNorm->input_dim_*tile_size, 0, 0, carus_instance, 1);
        if(t<n_tiles-1){ // move the input data for the next tile to the other instance of Carus
            if(next_tile_size!=1)carus_write_matrix(next_tile_size, addNorm->input_dim_, input + (t+1)*addNorm->input_dim_*tile_size, 0, 0, !carus_instance, 2); // move inputs for next tile
            else carus_write_flatten_matrix(addNorm->input_dim_, input + (t+1)*addNorm->input_dim_*tile_size, 0, 0, !carus_instance, 2);
        }
        DMA_WAIT(1);
        DMA_WAIT(2);
        carus_instance = !carus_instance; // switch instances (ping-pong buffer)
    }
#else
    int16_t tile_size = (addNorm->seq_len_ <= MAX_TILE_SIZE) ? addNorm->seq_len_ : MAX_TILE_SIZE;
    int16_t n_tiles = CEIL_INT_DIV(addNorm->seq_len_, tile_size);
    int16_t last_tile_size = addNorm->seq_len_ % tile_size;
    int8_t carus_instance = 0;
    int16_t vl = (tile_size > addNorm->input_dim_) ? tile_size : addNorm->input_dim_;

    carus_cfg_t cfg = CARUS_CFG_INIT(carus_instance);
    cfg.vl   = (uint32_t) vl;
    cfg.vtype= (uint32_t) VTYPE_VSEW_32;
    cfg.koffs= (uint32_t) CARUS_BATCHNORM_MULTIVECTOR_OFFSET;
    cfg.arg0 = (uint32_t) tile_size;
    carus_set_cfg(carus_instance, &cfg);

    // load weights/bias once
    carus_write_flatten_matrix(addNorm->input_dim_, addNorm->weight_, 0, GAMMA_VECTOR, carus_instance, 1);
    carus_write_flatten_matrix(addNorm->input_dim_, addNorm->bias_,   0, BETA_VECTOR,  carus_instance, 1);
    DMA_WAIT(1);

    for(int t = 0; t < n_tiles; t++){
        int16_t this_tile_size = ((t == n_tiles - 1) && last_tile_size) ? last_tile_size : tile_size;
        // adjust kernel cfg for last tile
        cfg.arg0 = this_tile_size;
        carus_set_cfg(carus_instance, &cfg);

        // move next tile input
        carus_write_matrix(this_tile_size, addNorm->input_dim_,
                           input + t * addNorm->input_dim_ * tile_size,
                           0, 0, carus_instance, 1);
        DMA_WAIT(1);

        // compute mean/variance in CPU
        compute_mean_stdev(this_tile_size,
                           addNorm->input_dim_,
                           input + t * addNorm->input_dim_ * tile_size,
                           (int32_t*)carus_vrf(carus_instance, MEAN_VECTOR),
                           (int32_t*)carus_vrf(carus_instance, VAR_VECTOR));

        // run kernel
        carus_run_kernel(carus_instance);
        carus_wait_done(carus_instance);

        // read back
        carus_read_matrix(this_tile_size, addNorm->input_dim_,
                          input_normalized + t * addNorm->input_dim_ * tile_size,
                          0, 0, carus_instance, 1);
        DMA_WAIT(1);
    }
#endif

}

// Compute the mean and standard deviation of the input data for layer normalization
void compute_mean_stdev(int16_t n_vectors, int16_t vec_len, int16_t* input, int32_t* mean_vec, int32_t* std_vec) {
    for (int i = 0; i < n_vectors; i++) { // iterates over the sequences (or batches)
        quant_bit_width *input_ptr = input + i *vec_len; // points to the current sequence in the input array.
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < vec_len; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        quant_bit_width mean = (quant_bit_width)((float)sum / (float)vec_len); 
        mean_vec[i] = (int32_t)mean;
        // compute variance
        input_ptr = input + i * (vec_len);
        int64_t variance = 0;
        for (int j = 0; j < vec_len; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(vec_len);
        variance_float = variance_float / (float)(1 << NUM_FRACTION_BITS);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        std_vec[i] = (int32_t)(sd_inv * (1 << NUM_FRACTION_BITS));
    }
}

// layer normalization operation with an optional scaling (weighting) and bias on the input data
void normalize(AddNormalize *addNorm, quant_bit_width *input, quant_bit_width *input_normalized) {
    for (int i = 0; i < addNorm->seq_len_; i++) { // iterates over the sequences (or batches)
        quant_bit_width *input_ptr = input + i * (addNorm->input_dim_); // points to the current sequence in the input array.
        quant_bit_width *input_normalized_ptr = input_normalized + i * (addNorm->input_dim_); // points to the corresponding location in the output array
        // compute the sum of the input values (for computing the mean)
        int sum = 0; 
        for (int j = 0; j < addNorm->input_dim_; j++) {
            sum += *input_ptr;
            input_ptr++;
        }
        // compute mean
        quant_bit_width mean = (quant_bit_width)((float)sum / (float)addNorm->input_dim_); 
        // compute variance
        input_ptr = input + i * (addNorm->input_dim_);
        int64_t variance = 0;
        for (int j = 0; j < addNorm->input_dim_; j++) {
            variance += MUL_HQ((*input_ptr - mean), (*input_ptr - mean));
            input_ptr++;
        }
        // adjust variance to get the average variance
        variance = SHIFT(variance); 
        float variance_float = (float)variance / (float)(addNorm->input_dim_);
        variance_float = variance_float / (float)(1 << NUM_FRACTION_BITS);
        // Calculate the Standard Deviation and Inverse
        float sd = sqrtf(variance_float);
        float sd_inv = (float)(1 / (sd + 0.00001)); // prevent zero divide!
        quant_bit_width sd_inv_int = (quant_bit_width)(sd_inv * (1 << NUM_FRACTION_BITS));
        // Normalize Each Element and Apply Scale (Weight) and Bias
        input_ptr = input + i * (addNorm->input_dim_);
        input_normalized_ptr = input_normalized + i * (addNorm->input_dim_);
        for (int j = 0; j < addNorm->input_dim_; j++) {
            *input_normalized_ptr = (quant_bit_width)MUL((*input_ptr - mean), sd_inv_int); // normalize by subtracting the mean and multiplying by the inverse of the standard deviation
            // After normalization, the result is scaled using the weight_ (γ) and bias_ (β) stored in addNorm
            *input_normalized_ptr = (quant_bit_width)(MUL((*input_normalized_ptr), addNorm->weight_[j]) + addNorm->bias_[j]);
            input_ptr++;
            input_normalized_ptr++;
        }
    }
}

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
