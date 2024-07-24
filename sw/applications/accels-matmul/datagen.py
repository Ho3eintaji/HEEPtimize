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

def main():
    descr = """\
# Generate the input data and the golden output for matrix multiplication, used by main.c to run the kernel.
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
                            choices=['int8', 'int16', 'int32'], 
                            default='int32',
                            help='element width (determines SEW).')
    cmd_parser.add_argument('--outdir', '-o', 
                            type=str, 
                            default=os.path.dirname(__file__),
                            help='directory where to store the otuput files.')
    cmd_parser.add_argument('--col_b', '-p',
                            type=int,
                            choices=range(1, 65537),
                            default=8,
                            metavar='P',
                            help='Number of columns of matrix R. This is NM-Carus vector length (VL).')
    cmd_parser.add_argument('--seed', '-s',
                            type=int,
                            help='Seed for numpy PRG (normally used for debug).')
    cmd_parser.add_argument('--row_a', '-m',
                            type=int,
                            choices=range(0, 17),
                            default=4,
                            help='Number of rows of matrix A.')
    cmd_parser.add_argument('--col_a', '-n',
                            type=int,
                            choices=range(0, 16),
                            default=4,
                            help='Number of columns of matrix A.')
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

    data_header = 'data.h'
    data_carus_header = 'data_carus.h'

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
    # Matrix A [m x n]
    A = np.random.randint(-2**(sew-1), 2**(sew-1), size=(M, N), dtype=dtype)

    # Matrix B [n x p], where p = col(B)
    B = np.random.randint(-2**(sew-1), 2**(sew-1), size=(N, P), dtype=dtype)

    # Golden output [m x p]
    R = np.matmul(A, B)

    # Generate C files
    ctype = ctype_decoder(sew)

    header_gen = CFileGen()
    header_carus_gen = CFileGen()

    header_carus_gen.add_macro('ARG0', M, "kernel argument 0: rows of A")
    header_carus_gen.add_macro('ARG1', N, "kernel argument 1: columns of A")
    header_carus_gen.add_macro('VL', P, "vector length: columns of B")
    header_gen.add_macro('ELEM_SIZE', sew // 8, "element size in bytes")
    header_gen.add_macro_raw('data_t', ctype, "element data type")

    header_gen.add_input_matrix('A', A)
    header_gen.add_input_matrix('B', B)
    header_gen.add_output_matrix('R', R)

    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    header_gen.write_header(out_dir, data_header)
    header_carus_gen.write_header(out_dir, data_carus_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

if __name__ == '__main__':
    main()
