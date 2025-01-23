
import argparse
import os
import numpy as np

from c_gen import CFileGen

# Define constants
NUM_FRACTION_BITS = 12
SCALE_183 = 183  # 0.044715 in fixed-point 12-bit
SCALE_SQRT_2_PI = 3268  # sqrt(2/pi) in fixed-point 12-bit

# Define fixed-point multiplication
def MUL(a, b):
    return (a * b) >> NUM_FRACTION_BITS

# Simplified GELU Activation Function
def gelu_activation(input):
    x3 = MUL(MUL(input, input), input)  # Compute x^3
    x3 = MUL(x3, SCALE_183) + input  # Scale and add input
    x3 = MUL(x3, SCALE_SQRT_2_PI)  # Scale by sqrt(2/pi)
    
    # Convert to floating-point and apply tanh
    in_float = x3.astype(np.float32) / (1 << NUM_FRACTION_BITS)
    in_tanh_fxp = (np.tanh(in_float) * (1 << NUM_FRACTION_BITS)).astype(np.int32)
    in_tanh_fxp += (1 << NUM_FRACTION_BITS)  # Add fixed-point 1

    # Compute final output
    return MUL(in_tanh_fxp, input >> 1).astype(np.int16)

def generate_data(data_size):
    input = np.random.randint(-32768, 32767, size=(1, data_size), dtype=np.int16)
    # calculate the golden output
    output = gelu_activation(input)
    return input, output

def main():
    parser = argparse.ArgumentParser(description='Generate input data for gelu kernel.')
    parser.add_argument('--data_size', type=int, default=64, help='Data size')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input, output = generate_data(args.data_size)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('DATA_SIZE', args.data_size, "Data size")
    header_gen.add_input_matrix('input', input)
    header_gen.add_output_matrix('output', output)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()