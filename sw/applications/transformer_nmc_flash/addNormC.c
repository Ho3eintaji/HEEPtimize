//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include "addNormC.h"



// Function implementations
AddNormalize createAddNormalize(int seq_len, int input_dim, quant_bit_width *weight, quant_bit_width *bias) {
    AddNormalize addNorm;
    addNorm.seq_len_ = seq_len;
    addNorm.input_dim_ = input_dim;
    addNorm.weight_ = weight;
    addNorm.bias_ = bias;
    // Initialize other fields as needed
    return addNorm;
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
            // After normalization, the result is scaled using the weight_ (scaling factor) and bias_ (shifting value) stored in addNorm
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
