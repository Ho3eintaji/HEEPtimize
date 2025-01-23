
import argparse
import os
import numpy as np

from c_gen import CFileGen

def generate_data(seq_len):
    input = np.random.randint(-32768, 32767, size=(seq_len, seq_len), dtype=np.int16)
    return input

def main():
    parser = argparse.ArgumentParser(description='Generate input data for dense kernel.')
    parser.add_argument('--seq_len', type=int, default=32, help='Sequence length')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input = generate_data(args.seq_len)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")

    header_gen.add_input_matrix('input', input)

    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()