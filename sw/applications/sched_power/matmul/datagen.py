import argparse
import os
import numpy as np

import nm_deployment
import sys
import caesar_backend as caesar

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

    # Define command line options
    # cmd_parser.add_argument('width', 
    #                         type=str, 
    #                         choices=['int8', 'int16', 'int32'], 
    #                         default='int32',
    #                         help='element width (determines SEW).')
    cmd_parser.add_argument('data_type',
                        help='Data type',
                        type=str,
                        choices=c_data_type_list)
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
    M = args.row_a
    N = args.col_a
    P = args.col_b

    # Print arguments
    print('Matrix multiplication golden model.')
    print('    R = A * B')
    print('Arguments:')
    print('- output directory: ' + out_dir)
    print('- element width (sew): ' + str(sew))
    print('- row(A): ' + str(M))
    print('- col(A): ' + str(N))
    print('- col(B) (vector length): ' + str(P))

    # -- Generate random inputs --
    if args.seed is not None:
        np.random.seed(args.seed)
    # Matrix A [m x n]
    A = np.random.randint(-2**(sew-1), 2**(sew-1), size=(M, N), dtype=dtype)
    # Matrix B [n x p], where p = col(B)
    B = np.random.randint(-2**(sew-1), 2**(sew-1), size=(N, P), dtype=dtype)
    # Golden output [m x p]
    R = np.matmul(A, B)

    # -- Generate file --
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('ELEM_SIZE', sew // 8, "element size in bytes")
    ctype = ctype_decoder(sew)
    header_gen.add_macro_raw('data_t', ctype, "element data type")
    header_gen.add_input_matrix('A', A)
    header_gen.add_input_matrix('B', B)
    header_gen.add_output_matrix('R', R)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print('- generated header file in \'' + out_dir + '/' + data_header + '\'.')

    # ------------------------------
    # Carus data generation
    # ------------------------------
    # 15 vector registers are allocated for the source matrix B, while 16
    # vector registers are allocated for the result matrix R. Therefore the
    # source matrix A has a maximum size of 16*15 elements (960 bytes at
    # most), and thus fits in a single 1kB vector register.
    carus_data_header = 'carus_data.h'
    header_carus_gen = CFileGen()
    header_carus_gen.add_macro('ARG0', M, "kernel argument 0: rows of A")
    header_carus_gen.add_macro('ARG1', N, "kernel argument 1: columns of A")
    header_carus_gen.add_macro('VL', P, "vector length: columns of B")
    header_carus_gen.write_header(out_dir, carus_data_header)
    print('- generated header file in \'' + out_dir + '/' + carus_data_header + '\'.')

    # ------------------------------
    # Caesar data generation
    # ------------------------------

    # -- Caesar commands --
    A_addr = int("0x4000", 16)
    B_addr = int("0x0", 16)
    R_addr = B_addr + (B.shape[0]*B.shape[1])*dbytes

    (mm_result, cmd_list, dest_list) = caesar.make_MatMul_cmds(element_type = data_type, A_addr = A_addr, B_addr = B_addr, R_addr = R_addr, width = dbits, A = A, B = B, R = R, Debug = False)
    if mm_result == False:
        print("Error generating commands", file=sys.stderr)
        exit(1)

    caesar_commands_header = 'caesar_commands.h'
    caesar_commands_gen = CFileGen()
    cmd_name = 'caesar_cmds_matmul'
    caesar_commands_gen.add_code(cmd_name, cmd_list)
    if dest_list is not None:
        addr_name = cmd_name + '_addr'
        caesar_commands_gen.add_code(addr_name, dest_list)
    caesar_commands_gen.write_header(out_dir, caesar_commands_header)
    print('- generated header file in \'' + out_dir + '/' + caesar_commands_header + '\'.')

    # -- Caesar data --
    caesar_data_header = 'caesar_data.h'
    caesar_data_gen = CFileGen()
    data_type = A.dtype.name
    caesar_data_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    caesar_data_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")
    caesar_data_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    caesar_data_gen.add_macro('CAESAR_B_OFFS', B_addr, "input B address offset")
    caesar_data_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")
    caesar_data_gen.write_header(out_dir, caesar_data_header)
    print('- generated header file in \'' + out_dir + '/' + caesar_data_header + '\'.')

if __name__ == '__main__':
    main()
