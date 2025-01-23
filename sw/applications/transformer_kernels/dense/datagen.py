
import argparse
import os
import numpy as np

from c_gen import CFileGen

def generate_data(seq_len, input_dim, output_dim):
    input = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
    weight = np.random.randint(-32768, 32767, size=(input_dim, output_dim), dtype=np.int16)
    bias = np.random.randint(-32768, 32767, size=(1,output_dim), dtype=np.int16)
    # calculate the golden output
    output = np.dot(input, weight) + bias
    return input, weight, bias, output

def main():
    parser = argparse.ArgumentParser(description='Generate input data for dense kernel.')
    parser.add_argument('--seq_len', type=int, default=32, help='Sequence length')
    parser.add_argument('--input_dim', type=int, default=64, help='Input dimension')
    parser.add_argument('--output_dim', type=int, default=128, help='Output dimension')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input, weight, bias, output = generate_data(args.seq_len, args.input_dim, args.output_dim)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")
    header_gen.add_macro('INPUT_DIM', args.input_dim, "Input dimension")
    header_gen.add_macro('OUTPUT_DIM', args.output_dim, "Output dimension")
    header_gen.add_macro('OUTPUT_DIM', args.output_dim, "Output dimension")
    header_gen.add_input_matrix('input', input)
    header_gen.add_input_matrix('weight', weight)
    header_gen.add_input_matrix('bias', bias)
    header_gen.add_output_matrix('output', output)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()