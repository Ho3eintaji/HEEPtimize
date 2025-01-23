
import argparse
import os
import numpy as np

from c_gen import CFileGen

def generate_data(mat_size):
    input = np.random.randint(-32768, 32767, size=(1, mat_size), dtype=np.int16)
    return input

def main():
    parser = argparse.ArgumentParser(description='Generate input data for dense kernel.')
    parser.add_argument('--mat_size', type=int, default=64, help='Matrix size')
    parser.add_argument('--shift_scale', type=int, default=2, help='Shift scale')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input = generate_data(args.mat_size)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('MAT_SIZE', args.mat_size, "Matrix size")
    header_gen.add_macro('SHIFT_SCALE', args.shift_scale, "Shift scale")
    header_gen.add_input_matrix('input', input)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()