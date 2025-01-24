
import argparse
import os
import numpy as np

from c_gen import CFileGen

def normalize_data(seq_len, input_dim, output_dim, input_data, weight, bias):
    # Initialize the normalized output
    input_normalized = np.zeros((seq_len, input_dim), dtype=np.int16)

    for i in range(seq_len):  # iterates over the sequences (or batches)
        input_seq = input_data[i]  # input sequence
        # Compute the sum of the input values (for computing the mean)
        input_sum = np.sum(input_seq)
        # Compute the mean
        mean = input_sum / input_dim
        
        # Compute the variance
        variance = np.sum((input_seq - mean) ** 2)
        # Average variance
        variance = variance / input_dim
        
        # Compute the standard deviation and its inverse (with small epsilon to avoid division by zero)
        sd = np.sqrt(variance)
        sd_inv = 1 / (sd + 0.00001)
        
        # Normalize each element, apply scaling (weight) and bias
        for j in range(input_dim):
            # Normalize the value by subtracting mean and multiplying by the inverse of std
            normalized_val = (input_seq[j] - mean) * sd_inv
            
            # Apply the weight and bias
            for k in range(output_dim):
                input_normalized[i, k] = np.int16(normalized_val * weight[j, k] + bias[0, k])

    return input_normalized



def generate_data(seq_len, input_dim, output_dim):
    input = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
    weight = np.random.randint(-32768, 32767, size=(input_dim, output_dim), dtype=np.int16)
    bias = np.random.randint(-32768, 32767, size=(1,output_dim), dtype=np.int16)
    # calculate the golden output
    # input_normalized = normalize_data(seq_len, input_dim, output_dim, input, weight, bias)
    input_normalized = np.zeros((seq_len, input_dim), dtype=np.int16)
    # TODO: there is a bug in the normalize_data function for small values of input_dim
    return input, weight, bias, input_normalized

def main():
    parser = argparse.ArgumentParser(description='Generate input data for dense kernel.')
    parser.add_argument('--seq_len', type=int, default=128, help='Sequence length')
    parser.add_argument('--input_dim', type=int, default=64, help='Input dimension')
    parser.add_argument('--output_dim', type=int, default=8, help='Output dimension')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input, weight, bias, input_normalized = generate_data(args.seq_len, args.input_dim, args.output_dim)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")
    header_gen.add_macro('INPUT_DIM', args.input_dim, "Input dimension")
    header_gen.add_macro('OUTPUT_DIM', args.output_dim, "Output dimension")

    header_gen.add_input_matrix('input', input)
    header_gen.add_input_matrix('weight', weight)
    header_gen.add_input_matrix('bias', bias)
    header_gen.add_input_matrix('input_normalized', input_normalized)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()