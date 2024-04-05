import nm_deployment
import numpy as np
import sys
import os
import caesar_backend as caesar
import argparse

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

# width decoder
def vtype_decoder(width: str) -> np.dtype:
    return {
        'int8': np.int8,
        'int16': np.int16,
        'int32': np.int32,
    }[width]

# Command line argument parser
cmd_parser = argparse.ArgumentParser(
    prog='make_app_nm',
    description='Generate commands and data for NM-Caesar'
)

c_data_type_list = ["int", "int32", "short", "int16", "char", "int8"]
kernel_list = ['matmul', 'conv2d_matmul', 'maxpool', 'and', 'or', 'xor', 'add', 'mul', 'relu', 'gemm']

# Define command line arguments
cmd_parser.add_argument('mem_type',
                        help='Memory type',
                        type=str,
                        choices=['carus', 'caesar'])

cmd_parser.add_argument('kernel',
                        help='Kernel: matmul, conv2d_matmul, maxpool',
                        type=str,
                        choices=kernel_list)

cmd_parser.add_argument('data_type',
                        help='Data type',
                        type=str,
                        choices=c_data_type_list)

cmd_parser.add_argument('--row_a',
                        help='Number of rows of M',
                        type=int,
                        default=16)

cmd_parser.add_argument('--col_a',
                        help='Number of columns of M',
                        type=int,
                        default=16)

cmd_parser.add_argument('--col_b',
                        help='Number of columns of A',
                        type=int,
                        default=16)

cmd_parser.add_argument('--size',
                        help='Size of the pooling window',
                        type=int,
                        default=2)

cmd_parser.add_argument('--stride',
                        help='Stride of the pooling window',
                        type=int,
                        default=2)

cmd_parser.add_argument('--shamt',
                        help='Shift amount.',
                        type=int,
                        default=0)

cmd_parser.add_argument('--alpha',
                        help='Alpha value',
                        type=int,
                        default=1)

cmd_parser.add_argument('--beta',
                        help='Beta value',
                        type=int,
                        default=1)

cmd_parser.add_argument('--min_value',
                        help='Minimum value of the input matrix',
                        type=int,
                        default=0)

cmd_parser.add_argument('--max_value',
                        help='Maximum value of the input matrix',
                        type=int,
                        default=10)

cmd_parser.add_argument('--print-output', '-p',
                        help='Print the expected output',
                        action='store_true')

cmd_parser.add_argument('--out-dir',
                        help='Output directory',
                        type=str,
                        default='./')

# Parse command line arguments
args = cmd_parser.parse_args()

try:
    memory_type = args.mem_type
    if(memory_type == "caesar" or memory_type=="carus"):
        print("Using " + memory_type)
    else:
        print("Wrong memory type")
        exit()
except:
    memory_type = "caesar"
    print("No memory specified: using " + memory_type)

# Kernel
kernel_type = args.kernel

try:
    data_type = c_data_type_decoder(args.data_type)
except:
    data_type = "int"
    print("No C data type specified: using " + data_type)

min_value = args.min_value
max_value = args.max_value

# Matrix dimensions
row_a = args.row_a
col_a = args.col_a
col_b = args.col_b

# Output options
print_expected_output = args.print_output
out_dir = args.out_dir

# Prepare output directory
if not os.path.exists(out_dir):
    # If it doesn't exist, create it
    os.makedirs(out_dir)

# Data and command files
fdata     = open(out_dir + "/caesar_data.h", "w")
fcommands = open(out_dir + "/caesar_commands.h", "w")

# Element type decoder
dtype = vtype_decoder(data_type)
dbytes = dtype(0).itemsize
dbits = dtype(0).itemsize * 8

### MATRIX MULTIPLICATION
if kernel_type == "matmul":
    # Input data
    A = np.random.randint(min_value, max_value, size=(row_a,col_a), dtype=dtype)
    B = np.random.randint(min_value, max_value, size=(col_a,col_b), dtype=dtype)

    # Input address
    A_addr = int("0x4000", 16)
    B_addr = int("0x0", 16)
    R_addr = B_addr + (B.shape[0]*B.shape[1])*dbytes
    R_exp = np.matmul(A,B, dtype=np.int32)

    if memory_type == "caesar":
        (mm_result, cmd_list, dest_list) = caesar.make_MatMul_cmds(element_type = data_type, A_addr = A_addr, B_addr = B_addr, R_addr = R_addr, width = dbits, A = A, B = B, R = R_exp)
        if mm_result == False:
            print("Error generating commands", file=sys.stderr)
            exit(1)
        
        # Generate Caesar commands
        nm_deployment.dumpCmds(fcommands, "caesar_cmds_matmul", cmd_list, dest_list)
        
        # Generate data
        if dtype != np.int32:
            # Transpose matrix B so Caesar can execute dot product
            B = B.T
        nm_deployment.dumpMatmulData(fdata, A, B, R_exp, A_addr, B_addr, R_addr, print_expected_output)

# 2D CONVOLUTION AS MATRIX MULTIPLICATION
elif kernel_type == "conv2d_matmul":
    # Input data
    A = np.random.randint(min_value, high=max_value, size=(row_a,col_a), dtype=dtype)
    K = np.random.randint(min_value, high=max_value, size=(col_b,col_b), dtype=dtype)

    K_addr = int("0x0", 16)
    M_addr = int("0x4000", 16)
    R_addr = K_addr + (A.shape[0]*A.shape[1])*dbytes

    # Generate transformed matrix M (transform convolution to matmul)
    (result, R_exp, M_trans, K_flat) = nm_deployment.conv2MatMul(A, K)
    if result == False:
        print("Error generating transformed matrix", file=sys.stderr)
        exit(1)

    if memory_type == "caesar":
        R_exp_flat = np.reshape(R_exp, (R_exp.shape[0]*R_exp.shape[1],1))
        (mm_result, cmd_list, dest_list) = caesar.make_MatMul_cmds(element_type = data_type , A_addr = M_addr, B_addr = K_addr, R_addr = R_addr, width = dbits, A = M_trans, B = K_flat, R = R_exp_flat, Conv2D = True)
        if mm_result == False:
            print("Error generating 2D convolution commands", file=sys.stderr)
            exit(1)

        # Generate Caesar commands
        nm_deployment.dumpCmds(fcommands, "caesar_cmds_matmul", cmd_list, dest_list)
        
        # Generate data
        # if dtype != np.int32:
        #     M_trans = M_trans.T
        nm_deployment.dumpConvData(fdata, A, K, M_trans, R_exp, K_addr=K_addr, M_addr=M_addr, R_addr=R_addr, print_output=print_expected_output)

# MAX pooling
elif kernel_type == "maxpool":
    # Input data
    A = np.random.randint(min_value, high=max_value, size=(row_a,col_a), dtype=dtype)
    size = args.size
    stride = args.stride

    # Input address
    A_addr = int("0x0", 16)
    R_addr = int("0x4000", 16)
    
    # Generate expected output
    R = nm_deployment.expectedMaxPool(A, size, stride)

    # Generate commands and destination addresses
    (cmd_list, dest_list) = caesar.make_MaxPool_cmds(A, R, A_addr, R_addr, size, stride, debug=False)
    nm_deployment.dumpCmds(fcommands, "caesar_cmds_maxpool", cmd_list, dest_list)

    # Dump generated input data
    nm_deployment.dumpMaxPoolData(fdata, A, R, A_addr, R_addr, size=size, stride=stride, print_output=print_expected_output)


# XOR
elif kernel_type == "and" or  kernel_type == "or" or kernel_type == "xor" or kernel_type == "add" or kernel_type == "sub" or kernel_type == "mul":
    # Input data
    A = np.random.randint(min_value, max_value, size=(row_a,col_a), dtype=dtype)
    B = np.random.randint(min_value, max_value, size=(row_a,col_a), dtype=dtype)

    # Input address
    A_addr = int("0x4000", 16)
    B_addr = int("0x0", 16)
    R_addr = B_addr + (B.shape[0]*B.shape[1])*dbytes
    
    # Generate expected output
    R = nm_deployment.expectedElemWise(kernel_type, A, B)

    # Generate commands and destination addresses
    (cmd_list, dest_list) = caesar.make_ElementWise_cmds(kernel_type, A, B, R, A_addr, B_addr, R_addr, debug=False)
    nm_deployment.dumpCmds(fcommands, "caesar_cmds_" + kernel_type, cmd_list, dest_list)

    # Dump generated input data
    nm_deployment.dumpElemWiseData(fdata, A, B, R, A_addr, B_addr, R_addr, print_output=print_expected_output)

# Relu (or Leaky ReLU)
elif kernel_type == "relu":
    # Input data
    A = np.random.randint(min_value, max_value, size=(row_a,col_a), dtype=dtype)
    shamt = args.shamt * np.ones((1, 4), dtype=A.dtype)

    # Input address
    A_addr = int("0x4000", 16)
    shamt_addr = int("0x0", 16)
    R_addr = shamt_addr + 16
    
    # Generate expected output
    R = nm_deployment.expectedRelu(A, shamt)

    # Generate commands and destination addresses
    (cmd_list, dest_list) = caesar.make_Relu_cmds(A, shamt, A_addr, shamt_addr, R_addr, debug=False)
    nm_deployment.dumpCmds(fcommands, "caesar_cmds_relu", cmd_list, dest_list)

    # Dump generated input data
    nm_deployment.dumpReluData(fdata, A, shamt, R, A_addr, shamt_addr, R_addr, print_output=print_expected_output)

# Relu
elif kernel_type == "gemm":
    # Input data
    A = np.random.randint(min_value, max_value, size=(row_a,col_a), dtype=dtype)
    B = np.random.randint(min_value, max_value, size=(col_a,col_b), dtype=dtype)
    #The dot product of caesar stored the result always on 32b
    # So As i need to add it to C, i need C on 32b too
    C = np.random.randint(min_value, max_value, size=(row_a,col_b), dtype="int32")

    alpha = args.alpha * np.ones( shape=(1,4), dtype=dtype)
    beta = args.beta * np.ones( shape=(1,1), dtype="int32")

    # Input address
    A_addr = int("0x4000", 16)
    B_addr = int("0x0", 16)
    alpha_addr =  B_addr + (B.shape[0]*B.shape[1])*dbytes
    beta_addr =  B_addr + 16 + (B.shape[0]*B.shape[1])*dbytes
    C_addr = A_addr + (A.shape[0]*A.shape[1])*dbytes
    R_addr = B_addr + 32 + (B.shape[0]*B.shape[1])*dbytes
    
    # Generate expected output
    R = nm_deployment.expectedGEMM(A, B, C, args.alpha, args.beta)

    # Generate commands and destination addresses
    (cmd_list, dest_list) = caesar.make_GEMM_cmds(A, B, C, alpha, beta, R, A_addr, B_addr, C_addr, alpha_addr, beta_addr, R_addr, dtype, debug=False)
    nm_deployment.dumpCmds(fcommands, "caesar_cmds_gemm", cmd_list, dest_list)

    # Dump generated input data
    if dtype != np.int32:
        # Transpose matrix B so Caesar can execute dot product
        B = B.T
    nm_deployment.dumpGEMMData(fdata, A, B, C, alpha, beta, R, A_addr, B_addr, C_addr, alpha_addr, beta_addr, R_addr, print_output=print_expected_output)


else:
    print("Wrong kernel type", file=sys.stderr)
    exit(1)
