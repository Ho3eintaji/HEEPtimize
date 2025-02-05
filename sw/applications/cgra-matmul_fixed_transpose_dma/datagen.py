import argparse
import os
import numpy as np
from c_gen import CFileGen

# VSEW decoder
def vtype_decoder(width: str) -> np.dtype:
    return {
        'int8': np.int8,
        'int16': np.int16,
        'int32': np.int32,
    }[width]

# Numpy dtype to C type
def ctype_decoder(sew: int) -> str:
    return {
        8: 'int8_t',
        16: 'int16_t',
        32: 'int32_t',
    }[sew]

def ctype_decoder_double(sew: int) -> str:
    return {
        8: 'int16_t',
        16: 'int32_t',
        32: 'int64_t',
    }[sew]

def main():
    descr = """\
# Generate the input data and the golden output for matrix multiplication on NM-Carus, used by main.c to run the kernel.
    R = A * B
"""

    # Create command line parser
    cmd_parser = argparse.ArgumentParser(
        prog='datagen',
        description='Matrix multiplication golden model.',
        epilog=descr
    )

    # Define command line options
    cmd_parser.add_argument('width', 
                            type=str, 
                            choices=['int8', 'int16'], 
                            default='int32',
                            help='element width (determines SEW).')
    cmd_parser.add_argument('--outdir', '-o', 
                            type=str, 
                            default=os.path.dirname(__file__),
                            help='directory where to store the otuput files.')
    cmd_parser.add_argument('--col_b', '-p',
                            type=int,
                            choices=range(1, 16),
                            default=8,
                            metavar='P',
                            help='Number of columns of matrix R. This is NM-Carus vector length (VL).')
    cmd_parser.add_argument('--seed', '-s',
                            type=int,
                            help='Seed for numpy PRG (normally used for debug).')
    cmd_parser.add_argument('--row_a', '-m',
                            type=int,
                            choices=range(0, 2048),
                            default=4,
                            help='Number of rows of matrix A.')
    cmd_parser.add_argument('--col_a', '-n',
                            type=int,
                            choices=range(0, 17),
                            default=4,
                            help='Number of columns of matrix A.')
    cmd_parser.add_argument('--decimal_bits', '-q',
                            type=int,
                            choices=range(0, 17),
                            default=8,
                            help='Number of decimal bits for the fixed point format.')
    cmd_parser.add_argument('--version', '-v', 
                            action='version', 
                            version='%(prog)s 0.1.0', 
                            help='print the version and exit.')

    # Parse command line arguments
    args = cmd_parser.parse_args()

    # Set parameters
    out_dir = args.outdir
    dtype = vtype_decoder(args.width)
    sew = dtype(0).itemsize * 8
    M = args.row_a
    N = args.col_a
    P = args.col_b
    Q = args.decimal_bits
    data_header = 'data.h'

    # Print arguments
    print('Matrix multiplication golden model.')
    print('    R = A * B')
    print('Arguments:')
    print('- output directory: ' + out_dir)
    print('- element width (sew): ' + str(sew))
    print('- row(A): ' + str(M))
    print('- col(A): ' + str(N))
    print('- col(B) (vector length): ' + str(P))

    # Generate random inputs
    if args.seed is not None:
        np.random.seed(args.seed)

    # 15 vector registers are allocated for the source matrix B, while 16
    # vector registers are allocated for the result matrix R. Therefore the
    # source matrix A has a maximum size of 16*15 elements (960 bytes at
    # most), and thus fits in a single 1kB vector register.
    # Matrix A [m x n]ù

    limit = 8

    A = np.random.randint(-limit, limit, size=(M, N), dtype=dtype)

    # Matrix B [n x p], where p = col(B)
    B = np.random.randint(-limit, limit, size=(N, P), dtype=dtype)

    # Golden output [m x p]
    R = np.matmul(A, B)

    A_fixed = (A * (2**Q)).astype(dtype)
    B_fixed = (B * (2**Q)).astype(dtype)
    R_fixed = (R * (2**Q)).astype(dtype)

    # Generate C files
    ctype = ctype_decoder(sew)
    ctype_double = ctype_decoder_double(sew)

    header_gen = CFileGen()

    header_gen.add_macro('ARG0', P, "kernel argument 0: columns of B")
    header_gen.add_macro('ARG1', N, "kernel argument 1: columns of A")
    header_gen.add_macro('VL', M, "vector length: rows of A")
    header_gen.add_macro('Q', Q, "vnumber of decimal bits in the fixed point format") 
    header_gen.add_macro('ELEM_SIZE', sew // 8, "element size in bytes")
    header_gen.add_macro_raw('data_t', ctype, "element data type")
    header_gen.add_macro_raw('data_t_double', ctype_double, "element data type for double width")

    header_gen.add_input_matrix('A', A_fixed)
    header_gen.add_input_matrix('B', B_fixed)
    header_gen.add_output_matrix('R', R_fixed)

    header_gen.add_attribute('section(".xheep_data_interleaved")')

    header_gen.write_header(out_dir, data_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

if __name__ == '__main__':
    main()
