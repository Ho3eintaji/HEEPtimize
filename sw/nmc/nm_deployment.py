import numpy as np
import scipy as sp
from c_gen import CFileGen
import sys
import os

# Matrix multiplication data (R = A x B)
def dumpMatmulData(f, A: np.ndarray, B: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, B_addr: int = 0, R_addr: int = 0, print_output: bool = False):
    print('Dumping matrix multiplication data: R = A x B')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- B shape: ' + str(B.shape))
    print('- B address: ' + hex(B_addr))
    print('- R shape: ' + str((A.shape[0], B.shape[1])))
    print('- R address: ' + hex(R_addr))

    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    header_gen.add_input_matrix('input_B', B)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type
    data_type = A.dtype.name
    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_B_OFFS', B_addr, "input B address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar 2D convolution data
def dumpConvData(f, A: np.ndarray, K: np.ndarray, M: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, K_addr: int = 0, M_addr: int = 0, R_addr: int = 0, print_output: bool = False):
    print('Dumping matrix multiplication data for 2D convolution: R = M x K')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- K shape: ' + str(K.shape))
    print('- K address: ' + hex(K_addr))
    print('- M shape: ' + str(M.shape))
    print('- M address: ' + hex(M_addr))
    print('- R shape: ' + str(R_exp.shape))
    print('- R address: ' + hex(R_addr))

    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    header_gen.add_input_matrix('transformed_M', M)
    header_gen.add_input_matrix('filter_K', K)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type
    data_type = A.dtype.name
    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_K_OFFS', K_addr, "input K address offset")
    header_gen.add_macro('CAESAR_M_OFFS', M_addr, "input K address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar Max Pooling data
def dumpMaxPoolData(f, A: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, R_addr: int = 0, size: int = 2, stride: int = 2, print_output: bool = False):
    print('Dumping max pooling data: R = max_pool(A)')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- R shape: ' + str(R_exp.shape))
    print('- R address: ' + hex(R_addr))

    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type
    data_type = A.dtype.name
    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definitions for pooling size and stride
    header_gen.add_macro('POOL_SIZE', size, "pooling size")
    header_gen.add_macro('POOL_STRIDE', stride, "pooling stride")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar element-wise operation data
def dumpElemWiseData(f, A: np.ndarray, B: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, B_addr: int = 0, R_addr: int = 0, print_output: bool = False):
    print('Dumping xor data: R = A^B')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- B shape: ' + str(B.shape))
    print('- B address: ' + hex(B_addr))
    print('- R shape: ' + str(R_exp.shape))
    print('- R address: ' + hex(R_addr))

    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    header_gen.add_input_matrix('input_B', B)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type
    data_type = A.dtype.name
    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_B_OFFS', B_addr, "input B address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar Relu data
def dumpReluData(f, A: np.ndarray, shamt: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, shamt_addr: int = 0, R_addr: int = 0, print_output: bool = False):
    print('Dumping Relu data: R = Relu(A)')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- shamt shape: ' + str(shamt.shape))
    print('- shamt address: ' + hex(shamt_addr))
    print('- R shape: ' + str(R_exp.shape))
    print('- R address: ' + hex(R_addr))

    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    header_gen.add_input_matrix('input_SHAMT', shamt)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type
    data_type = A.dtype.name
    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_SHAMT_OFFS', shamt_addr, "shift amount address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar Relu data
def dumpGEMMData(f, A: np.ndarray, B: np.ndarray, C: np.ndarray, ALPHA: np.ndarray, BETA: np.ndarray, R_exp: np.ndarray, A_addr: int = 0, B_addr: int = 0, C_addr: int = 0, ALPHA_addr: int = 0, BETA_addr: int = 0, R_addr: int = 0, print_output: bool = False):
    #We need to store B.T if width is 16b or 8b to leverage SIMD
    data_type = A.dtype.name
    B_transposed = B.T

    print('Dumping GEMM data: R = alpha*A.B +beta*C')
    print('- A shape: ' + str(A.shape))
    print('- A address: ' + hex(A_addr))
    print('- B shape: ' + str(B.shape))
    print('- B address: ' + hex(B_addr))
    print('- C shape: ' + str(C.shape))
    print('- C address: ' + hex(C_addr))
    print('- Alpha shape: ' + str(ALPHA.shape))
    print('- Alpha address: ' + hex(ALPHA_addr))
    print('- Beta shape: ' + str(BETA.shape))
    print('- Beta address: ' + hex(BETA_addr))
    print('- R shape: ' + str(R_exp.shape))
    print('- R address: ' + hex(R_addr))



    # Create C header generator
    header_gen = CFileGen()

    # Map data into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump input and output matrices
    header_gen.add_input_matrix('input_A', A)
    header_gen.add_input_matrix('input_B', B)
    header_gen.add_input_matrix('input_C', C)
    header_gen.add_input_matrix('input_ALPHA', ALPHA)
    header_gen.add_input_matrix('input_BETA', BETA)
    if print_output:
        header_gen.add_macro('EXPECTED_OUTPUT_AVAILABLE', 1, "golden result available")
        header_gen.add_output_matrix('output_R', R_exp)

    # Add macro definition for data type

    header_gen.add_macro_raw('data_t', f"{data_type}_t", "element data type")
    header_gen.add_macro('DATA_WIDTH', A.itemsize * 8, "element data width")

    # Add macro definition for data addresses
    header_gen.add_macro('CAESAR_A_OFFS', A_addr, "input A address offset")
    header_gen.add_macro('CAESAR_B_OFFS', B_addr, "input B address offset")
    header_gen.add_macro('CAESAR_C_OFFS', C_addr, "input C address offset")
    header_gen.add_macro('CAESAR_ALPHA_OFFS', ALPHA_addr, "input ALPHA address offset")
    header_gen.add_macro('CAESAR_BETA_OFFS', BETA_addr, "input BETA address offset")
    header_gen.add_macro('CAESAR_R_OFFS', R_addr, "output R address offset")

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)

# Caesar matrix multiplication commands
def dumpCmds(f, cmd_name: str, cmd_list: str, addr_list: int = None):
    print('Dumping matrix multiplication commands')

    # Create C header generator
    header_gen = CFileGen()

    # Map commands into .xheep_data_interleaved section
    # header_gen.add_attribute('section(".xheep_data_interleaved")')

    # Dump commands
    header_gen.add_code(cmd_name, cmd_list)

    # Dump command addresses
    if addr_list is not None:
        addr_name = cmd_name + '_addr'
        header_gen.add_code(addr_name, addr_list)

    # Write the header file
    header_base = os.path.basename(f.name)
    header_macro = header_base.upper().replace('.', '_') + '_'
    header_gen.append_header(f, header_macro)


#Convolve A and K, both must be squared matrixes, K size being <= A
#if transorm to matrix is true (must be for caesar), it dumps the matrix M, which is K transformed
def conv2MatMul(A: np.ndarray, K: np.ndarray):
    print("Computing A conv K with A.shape= " + str(A.shape) + " and K.shape=" + str(K.shape))
    (exp, gold, M, result) = expected2DConv(A,K,False)

    K_flat = np.reshape(K, (K.shape[0]*K.shape[1],1))

    if result == True:
        return (True, exp, M, K_flat)
    
    print("!!!!! conv2MatMul ERROR!!!!!", file=sys.stderr)
    print("expected", file=sys.stderr)
    print(exp, file=sys.stderr)
    print("gold", file=sys.stderr)
    print(gold, file=sys.stderr)
    return (False, None, None, None)


def expected2DConv(A, K, Debug) :

    #adapted from https://www.baeldung.com/cs/convolution-matrix-multiplication

    # Parameters:
    # - A: input matrix
    # - K: kernel matrix (squared)
    # - Stride: assumed to be 1
    # - Padding: assumed to be 0

    # Convert inputs to 32 bits to emulate Caesar MACC behaviour
    A_wide = A.astype(np.int32)
    K_wide = K.astype(np.int32)
    R_gold = sp.signal.correlate2d(A_wide, K_wide, mode='valid', boundary='fill', fillvalue=0)

    # Get the shape of the input and output matrices
    A_rows = A.shape[0]
    A_cols = A.shape[1]
    K_rows = K.shape[0]
    K_cols = K_rows # K is squared
    R_rows = A_rows - K_rows + 1
    R_cols = A_cols - K_cols + 1
    #padding = 0 #do not change this
    #stride = 1 #do not change this

    if (R_rows != R_gold.shape[0] or R_cols != R_gold.shape[1]):
        print("Output size is wrong! (" + str(R_rows) + "," + str(R_cols) + ", it should be (" + str(R_gold.shape[0]) + "," + str(R_gold.shape[1]) + ")", file=sys.stderr)

    # Flatten the filter
    K_flat = K.flatten()

    # Generate doubly blocked Toeplitz matrix M from the input matrix A
    # M is (row(R) * col(R)) x (row(K) * col(K))
    M = np.zeros((R_rows*R_cols, K_rows*K_cols), dtype=A.dtype)

    if(Debug):
        print("Building M of shape " + str(M.shape))

    M_row_idx = 0
    for i in range(R_rows):
        for j in range(R_cols):
            # Flatten block of n*n elements of A
            M_block = A[i:i+K_rows, j:j+K_cols].flatten()
            M[M_row_idx, :] = M_block
            M_row_idx += 1

    # Check that M fits in half of Caesar memory
    mem_size = 2**15 # 32KB
    mem_limit = mem_size//2
    M_size = M.shape[0] * M.shape[1] * M.itemsize
    if M_size > mem_limit:
        print("M matrix is too big for Caesar memory! (" + str(M_size) + " bytes, max is " + str(mem_limit) + " bytes)", file=sys.stderr)
        return (False, None, None, None)
    
    if(Debug):
        print("M is: ")
        print(M)

    # Compute the convolution as the matrix multiplication R_flat = M * K_flat
    R_flat = np.matmul(M, K_flat, dtype=np.int32)
    R_exp = np.reshape(R_flat, (R_rows, R_cols))

    return (R_exp, R_gold, M, (R_exp == R_gold).all())

# Max pooling golden model
def expectedMaxPool(A: np.ndarray, size: int = 2, stride: int = 2) -> np.ndarray:
    # Compute output dimensions
    row_a, col_a = A.shape
    row_r = (row_a - size) // stride + 1
    col_r = (col_a - size) // stride + 1

    # Check that the input arguments are valid
    if (col_a - size) % stride != 0:
        raise ValueError('unsupported parameters - (rows(A) - size) / stride must be an integer')
    if (row_a - size) % stride != 0:
        raise ValueError('unsupported parameters - (cols(A) - size) / stride must be an integer')
    
    # Initialize output matrix
    R = np.zeros((row_r, col_r), dtype=A.dtype)

    # Compute output matrix
    for i in range(row_r):
        for j in range(col_r):
            R[i, j] = np.max(A[i*stride:i*stride+size, j*stride:j*stride+size])
    
    # Return the golden output
    return R

# Element-wise operations golden model
def expectedElemWise(op: str, A: np.ndarray, B: np.ndarray) -> np.ndarray:
    # Compute output dimensions
    row_a, col_a = A.shape
    print(A)
    print(B)
    # Initialize output matrix
    R = np.zeros((row_a, col_a), dtype=A.dtype)

    # Compute output matrix
    for i in range(row_a):
        for j in range(col_a):
            if op == 'and':
                R[i, j] = A[i,j] & B[i,j]
            elif op == 'or':
                R[i, j] = A[i,j] | B[i,j]
            elif op == 'xor':
                R[i, j] = A[i,j] ^ B[i,j]
            elif op == 'add':
                R[i, j] = A[i,j] + B[i,j]
            elif op == 'sub':
                R[i, j] = A[i,j] - B[i,j]
            elif op == 'mul':
                R[i, j] = A[i,j] * B[i,j]

    # Return the golden output
    return R


# ReLU golden model (shamt != 0 for Leaky ReLU)
def expectedRelu(A: np.ndarray, shamt: np.ndarray = 0) -> np.ndarray:
    s = shamt.flatten()[0]
    # Compute the output matrix
    if s == 0:
        R = np.maximum(A, 0)
    else:
        R = np.maximum(np.right_shift(A, s), A)

    # Return the golden output
    return R

# GEMM golden model
def expectedGEMM(A: np.ndarray, B: np.ndarray, C: np.ndarray, alpha: np.ndarray, beta: np.ndarray) -> np.ndarray:

    # Compute output matrix
    R = alpha * np.dot(A, B) + beta * C
    print(R)
    # Return the golden output
    return R
