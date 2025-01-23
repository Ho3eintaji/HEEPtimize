
import argparse
import os
import numpy as np

from c_gen import CFileGen

def multihead_transpose(input, seq_len, head_hidden_size, num_heads):
    """
    Rearranges data from multi-head attention format.
    Args:
        input: Input data (1D array with size seq_len * head_hidden_size * num_heads).
        seq_len: Number of tokens in the sequence.
        head_hidden_size: Size of each head's hidden dimension.
        num_heads: Number of attention heads.
    Returns:
        output: Output data (1D array with reordered layout).
    """
    # Step 1: Reshape the input to (num_heads, seq_len, head_hidden_size)
    input_reshaped = input.reshape(num_heads, seq_len, head_hidden_size)
    
    # Step 2: Transpose to (seq_len, num_heads, head_hidden_size)
    transposed = input_reshaped.transpose(1, 0, 2)
    
    # Step 3: Flatten the transposed result into a 1D array
    output = transposed.flatten().reshape(1, num_heads * seq_len * head_hidden_size)
    return output


def generate_data(seq_len, head_hidden_size, num_heads):
    input = np.random.randint(-32768, 32767, size=(1, num_heads * seq_len * head_hidden_size), dtype=np.int16)
    # calculate the golden output
    output = multihead_transpose(input, seq_len, head_hidden_size, num_heads)
    # output = np.random.randint(-32768, 32767, size=(1, num_heads * seq_len * head_hidden_size), dtype=np.int16)
    
    return input, output

def main():
    parser = argparse.ArgumentParser(description='Generate input data for dense kernel.')
    parser.add_argument('--seq_len', type=int, default=32, help='Sequence length')
    parser.add_argument('--head_hidden_size', type=int, default=64, help='Head hidden size')
    parser.add_argument('--num_heads', type=int, default=4, help='Number of heads')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input, output = generate_data(args.seq_len, args.head_hidden_size, args.num_heads)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")
    header_gen.add_macro('HEAD_HIDDEN_SIZE', args.head_hidden_size, "Head hidden size")
    header_gen.add_macro('NUM_HEADS', args.num_heads, "Number of heads")
    header_gen.add_input_matrix('input', input)
    header_gen.add_output_matrix('output', output)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()