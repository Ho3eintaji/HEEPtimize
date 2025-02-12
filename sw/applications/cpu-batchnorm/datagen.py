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

def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

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
                            choices=['int8', 'int16', 'int32'], 
                            default='int32',
                            help='element width (determines SEW).')
    cmd_parser.add_argument('--outdir', '-o', 
                            type=str, 
                            default=os.path.dirname(__file__),
                            help='directory where to store the output files.')
    cmd_parser.add_argument('--vector_len', '-p',
                            type=int,
                            choices=range(1, 65536),
                            default=8,
                            metavar='VL',
                            help='Vector length.')
    cmd_parser.add_argument('--seed', '-s',
                            type=int,
                            help='Seed for numpy PRG (normally used for debug).')
    cmd_parser.add_argument('--vector_num', '-m',
                            type=int,
                            choices=range(1, 65536),
                            default=4,
                            metavar='N',
                            help='Number of vectors.')
    cmd_parser.add_argument('--decimal_bits', '-q',
                            type=int,
                            choices=range(0, 16),
                            default=8,
                            help='Number of decimal bits for the fixed point format.')
    
    # Receive fxp as a boolean argument
    cmd_parser.add_argument('--fxp', '-f',
                            type=str2bool,  # changed to custom boolean parser
                            default=True,
                            help='Fixed point operation (True for fxp, False for int).')
                            
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
    VL = args.vector_len
    N = args.vector_num
    Q = args.decimal_bits
    data_header = 'data.h'

    # Print arguments
    print('Batch normalization golden model.')

    # Generate random inputs
    if args.seed is not None:
        np.random.seed(args.seed)

    # 15 vector registers are allocated for the source matrix B, while 16
    # vector registers are allocated for the result matrix R. Therefore the
    # source matrix A has a maximum size of 16*15 elements (960 bytes at
    # most), and thus fits in a single 1kB vector register.
    # Matrix A [m x n]

    limit = 50

    WEIGTHS = np.random.randint(-limit, limit, size=(1, VL), dtype=dtype)
    BIASES = np.random.randint(-limit, limit, size=(1, VL), dtype=dtype)

    # Matrix B [n x p], where p = col(B)
    A = np.random.randint(-limit, limit, size=(N, VL), dtype=dtype)

    # Generate C files
    ctype = ctype_decoder(sew)
    ctype_double = ctype_decoder_double(sew)

    header_gen = CFileGen()

    header_gen.add_macro('ARG0', N, "kernel argument 0: number of vectors")
    header_gen.add_macro('VL', VL, "vector length")
    # Added differentiation for fixed point or pure integer version.
    if args.fxp == True:
        header_gen.add_macro('Q', Q, "number of decimal bits in the fixed point format")
        header_gen.add_macro('OPERATION', '"fxp"', "fixed point operation")
    else:
        header_gen.add_macro('Q', 0, "no fixed point scaling")
        header_gen.add_macro('OPERATION', '"int"', "integer operation")
    header_gen.add_macro('ELEM_SIZE', sew // 8, "element size in bytes")
    header_gen.add_macro_raw('data_t', ctype, "element data type")
    header_gen.add_macro_raw('data_t_double', ctype_double, "element data type for double width")

    header_gen.add_input_matrix('W', WEIGTHS)
    header_gen.add_input_matrix('B', BIASES)
    header_gen.add_input_matrix('A', A)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

if __name__ == '__main__':
    main()
