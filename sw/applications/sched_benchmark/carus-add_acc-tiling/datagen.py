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
    cmd_parser.add_argument('--vl', '-p',
                            type=int,
                            choices=range(1, 65537),
                            default=8,
                            metavar='P',
                            help='vector length as number of elements.')
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
    max_num_vectors_for_each_input = 16  # 16 RFs are used for data max (in1, in2) --> out = in1 = in1 + in2
    vector_KB = 2048  # 2KB per vector  (64 KB / 32 RFs -> 2KB per RF)
    max_input_KB = max_num_vectors_for_each_input * vector_KB
    element_size = sew // 8
    vlmax = max_input_KB // element_size
    vl = args.vl

    # # Check vector length
    # if vl > vlmax:
    #     raise ValueError(f"Vector length (vl = {vl}) won't fit in the available memory for SEW = {sew}. Maximum vl is {vlmax}.")


    # Print arguments
    print('Vector addition golden model.')
    print('    r[i] = a[i] + b[i]')
    print('Arguments:')
    print('- output directory: ' + out_dir)
    print('- element width (sew): ' + str(sew))
    print('- vector length (vl = col(B)): ' + str(vl))

    # -- Generate random inputs --
    if args.seed is not None:
        np.random.seed(args.seed)
    
    # Scaling factor to reduce the random range
    range_scaling = 1  # Adjust this value to control the range
    # Calculate the range based on sew and scaling
    random_range = int((2**(sew-1)) * range_scaling)

    # TODO: correct it
    # Vector a
    A = np.random.randint(-random_range, random_range, size=(1,vl), dtype=dtype)
    # A = np.random.randint(0, 10, size=(1,vl), dtype=dtype)
    # Vector b
    B = np.random.randint(-random_range, random_range, size=(1, vl), dtype=dtype)
    # B = np.random.randint(0, 10, size=(1, vl), dtype=dtype)
    # Golden output
    R = A + B

    ctype = ctype_decoder(sew)

    # -- Generate file --
    data_header = 'data.h'
    header_gen = CFileGen()
    
    header_gen.add_macro('VL', vl, "vector length: columns of B")
    header_gen.add_macro('ELEM_SIZE', element_size, "element size in bytes")
    header_gen.add_macro_raw('data_t', ctype, "element data type")

    header_gen.add_macro_raw('Max_input_KB', max_input_KB, "maximum input size in KB")
    header_gen.add_macro('VL_MAX', vlmax, "maximum vector length")

    header_gen.add_input_matrix('A', A)
    header_gen.add_input_matrix('B', B)
    header_gen.add_output_matrix('R', R)

    header_gen.add_attribute('section(".xheep_data_flash_only")')
    # header_gen.add_attribute('section(".xheep_data_interleaved")') # TODO: correct it

    header_gen.write_header(out_dir, data_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

if __name__ == '__main__':
    main()
