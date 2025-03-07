import argparse
import os
import numpy as np
import nm_deployment
import sys
# import caesar_backend as caesar

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

# C data type decoder
def c_data_type_decoder(data_type: str) -> str:
    if (data_type == "int" or data_type == "int32"):
        return "int32"
    elif (data_type == "short" or data_type == "int16"):
        return "int16"
    elif (data_type == "char" or data_type == "int8"):
        return "int8"
    else:
        print("Wrong C data type", file=sys.stderr)
        exit(1)

c_data_type_list = ["int", "int32", "short", "int16", "char", "int8"]

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

    cmd_parser.add_argument('data_type',
                        help='Data type',
                        type=str,
                        choices=c_data_type_list)
    cmd_parser.add_argument('--outdir', '-o', 
                            type=str, 
                            default=os.path.dirname(__file__),
                            help='directory where to store the otuput files.')
    cmd_parser.add_argument('--row_a', '-m',
                            type=int,
                            choices=range(0, 1025),
                            default=4,
                            metavar='N',
                            help='Number of vectors.')
    cmd_parser.add_argument('--col_a', '-p',
                            type=int,
                            choices=range(0, 1025),
                            default=8,
                            metavar='VL',
                            help='Vector length.')
    cmd_parser.add_argument('--decimal_bits', '-q',
                            type=int,
                            choices=range(0, 16),
                            default=12,
                            help='Number of decimal bits for the fixed point format.')
    cmd_parser.add_argument('--seed', '-s',
                            type=int,
                            help='Seed for numpy PRG (normally used for debug).')
    cmd_parser.add_argument('--version', '-v', 
                            action='version', 
                            version='%(prog)s 0.1.0', 
                            help='print the version and exit.')

    # Parse command line arguments
    args = cmd_parser.parse_args()

    # Output directory
    out_dir = args.outdir
    
    # ------------------------------
    # Data generation
    # ------------------------------

    try:
        data_type = c_data_type_decoder(args.data_type)
    except:
        data_type = "int"
        print("No C data type specified: using " + data_type)   

    # Set parameters
    # dtype = vtype_decoder(args.width)
    dtype = vtype_decoder(data_type)
    dbytes = dtype(0).itemsize
    dbits = dtype(0).itemsize * 8

    sew = dtype(0).itemsize * 8

    VL = args.col_a
    N = args.row_a
    Q = args.decimal_bits

    # Print arguments
    print('Batch normalization golden model.')

    # -- Generate random inputs --
    if args.seed is not None:
        np.random.seed(args.seed)

    
    # range_scaling = 1  # Adjust this value to control the range
    # # Calculate the range based on sew and scaling
    # random_range = int((2**(sew-1)) * range_scaling)
    random_range = 50

    WEIGTHS = np.random.randint(-random_range, random_range, size=(1, VL), dtype=dtype)
    BIASES = np.random.randint(-random_range, random_range, size=(1, VL), dtype=dtype)

    # Matrix B [n x p], where p = col(B)
    A = np.random.randint(-random_range, random_range, size=(N, VL), dtype=dtype)

    ctype = ctype_decoder(sew)
    ctype_double = ctype_decoder_double(sew)

    # -- Generate file --
    data_header = 'data.h'
    header_gen = CFileGen()
    
    header_gen.add_macro('ARG0', N, "kernel argument 0: number of vectors")
    header_gen.add_macro('VL', VL, "vector length")
    header_gen.add_macro('Q', Q, "vnumber of decimal bits in the fixed point format") 
    header_gen.add_macro('ELEM_SIZE', sew // 8, "element size in bytes")
    header_gen.add_macro_raw('data_t', ctype, "element data type")
    header_gen.add_macro_raw('data_t_double', ctype_double, "element data type for double width")

    header_gen.add_input_matrix('W', WEIGTHS)
    header_gen.add_input_matrix('B', BIASES)
    header_gen.add_input_matrix('A', A)

    header_gen.add_attribute('section(".xheep_data_flash_only")')
    # header_gen.add_attribute('section(".xheep_data_interleaved")') # TODO: correct it

    header_gen.write_header(out_dir, data_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

if __name__ == '__main__':
    main()
