import argparse
import os
import numpy as np

from c_gen import CFileGen

def generate_data(seq_len, input_dim):
    inputA = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
    inputB = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
    return inputA, inputB

def main():
    parser = argparse.ArgumentParser(description='Generate input data for add kernel.')
    parser.add_argument('--seq_len', type=int, default=120, help='Sequence length')
    parser.add_argument('--input_dim', type=int, default=16, help='Input dimension')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    inputA, inputB = generate_data(args.seq_len, args.input_dim)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")
    header_gen.add_macro('INPUT_DIM', args.input_dim, "Input dimension")
    header_gen.add_input_matrix('inputA', inputA)
    header_gen.add_input_matrix('inputB', inputB)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()