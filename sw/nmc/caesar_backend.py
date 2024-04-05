import sys
import numpy as np
import scipy as sp
import gen_caesar_instructions as caesar_instruction
import caesar_cmd_gen as cg

# Matrix multiplication R = A x B or and convolution R = A * B
def make_MatMul_cmds(element_type, A_addr, B_addr, R_addr, width, A, B, R, Debug = False, Conv2D = False) :

    #first check that the algorithms works
    if(Debug):
        print("Building MatMul (R = A x B) commands: ")
        print("element_type is: " + element_type)
        print("A address: " + str(hex(A_addr)))
        print("B address: " + str(hex(B_addr)))
        print("R address: " + str(hex(R_addr)))
        print("A shape is " + str(A.shape))
        print("B shape is " + str(B.shape))
        print("R shape is " + str(R.shape))

    if (A.shape[1] != B.shape[0]) :
        print(f"ERR! number of columns of A ({A.shape[1]}) differs from the number of columns of B ({B.shape[0]})\n", file=sys.stderr)
        print("ERR! Matrix multiplication cannot be performed. Exiting now.\n", file=sys.stderr)
        return (False, [], [])

    if element_type == "int32":
        return make_MatMul_32b_cmds(A_addr, B_addr, R_addr, width, A, B, R, Debug)
    elif element_type == "int16":
        # i need to differentiate cases between matmul and conv 2D because A is a vector in Conv2D
        if (Conv2D) : 
            if A.shape[1] <= 2 or B.shape[0] < 2 :
                print("ERR! Matrix size for int16 is too SMALL, at least 2x2 matrixes", file=sys.stderr)
                return (False, [], [])
        else : 
            if A.shape[0] < 2 or A.shape[1] <= 2 or B.shape[0] <= 2 or B.shape[1] < 2:
                print("ERR! Matrix size for int16 is too SMALL, at least [2x4]x[4x2] matrices")
                return (False, [], [])
        return make_MatMul_simd_cmds(element_type, A_addr, B_addr, R_addr, width, A, B, R, Debug)
    elif element_type == "int8":
        # i need to differentiate cases between matmul and conv 2D because A is a vector in Conv2D
        if (Conv2D) : 
            if  A.shape[1] <= 4 or B.shape[0] < 4 :
                print("ERR! Matrix size for int8 is too SMALL, at least 4x4 matrices")
                print(A)
                print(B)
                return (False, [], [])
        else : 
            if A.shape[0] < 4 or A.shape[1] <= 4 or B.shape[0] <= 4 or B.shape[1] < 4:
                print("ERR! Matrix size for int8 is too SMALL, at least 4x4 matrices")
                return (False, [], [])
        return make_MatMul_simd_cmds(element_type, A_addr, B_addr, R_addr, width, A, B, R, Debug)
    else:
        print("ERR! wrong elements_type specified\n", file=sys.stderr)
        return (False, [], [])


#this matmul pretends the Amatrix is transposed
def make_MatMul_simd_cmds(element_type, A_addr, B_addr, R_addr, width, A, B, R, Debug = False) :

    if element_type == "int16":
        elements_word = 2
        biggest_number = 2 ** 15
        # Check if there isn't two different lines on the same 32b word, nb of columns should be a multiple of 2
        if (A.shape[1] % 2 != 0) :
            print("ERR! wrong Matrix size specified\n", file=sys.stderr)
            print(A.shape[1])
            print("Nb of columns should be a multiple of 2 for SIMD 16b\n")
            return (False, [], [])
    elif element_type == "int8":
        elements_word = 4
        biggest_number = 2 ** 7
        # Check if there isn t two different lines on the same 32b word, nb of columns should be a multiple of 2
        if (A.shape[1] % 4 != 0) :
            print("ERR! wrong Matrix size specified\n", file=sys.stderr)
            print(A.shape[1])
            print("Nb of columns should be a multiple of 4 for SIMD 8b\n")
            return (False, [], [])
    else:
        print("ERR! wrong elements_type specified\n", file=sys.stderr)
        return (False, [], [])

    cmd_list = []
    dest_list = []

    # Add the configuration register command for the bitwidth
    if element_type == "int16":
        instr_width = "CSRW"
        bitwidth = caesar_instruction.get_width2string("WORD_16B")
        cmd_width = caesar_instruction.get_opcode2string(instr_width) + bitwidth + bitwidth
    elif element_type == "int8":
        instr_width = "CSRW"
        bitwidth = caesar_instruction.get_width2string("WORD_8B")
        cmd_width = caesar_instruction.get_opcode2string(instr_width) + bitwidth + bitwidth

    else:
        print("ERR! wrong elements_type specified\n", file=sys.stderr)
        return (False, [], [])

    cmd = hex(int(cmd_width, 2))
    dest_list.append(str(hex(R_addr)))
    cmd_list.append(str(cmd))
    print("My CRSW command is : ")
    print(cmd)
    print("My address is : ")
    print(R_addr)

    #transform the C shape from column to row as it will be stored that way in memory (i.e. contiguous)
    C = np.zeros((R.shape), dtype=np.int32)
    C_reshaped = np.zeros((R.T.shape))

    #address are given in Byte, Caesar addresses words (32bit)

    A_offs_addr = int(A_addr/4)
    B_offs_addr = int(B_addr/4)
    R_offs_addr = int(R_addr/4)

    #in SIMD we leverages the DOT PRODUCT, so we need to transpose A

    B_transposed = B.T

    for i in range(A.shape[0]):

        i_simd_index = int(i / elements_word)

        for j in range(B_transposed.shape[0]):

            c = 0

            j_simd_index = int(j / elements_word)

            #the output is still 32b regardless the type
            R_addr_loop = R_offs_addr + i*C_reshaped.shape[0] + j

            for k in range(A.shape[1]):

                c += A[i,k]*B_transposed[j,k]

                k_simd_index = int(k / elements_word)

                if ((k % elements_word) == 0 ): # generate instructions only at the beginning of a new 32b data

                    A_addr_loop = A_offs_addr + i*int(A.shape[1]/elements_word) + k_simd_index
                    B_addr_loop = B_offs_addr + j*int(B_transposed.shape[1]/elements_word) + k_simd_index

                    addr1 = caesar_instruction.get_address2string(A_addr_loop)
                    addr0 = caesar_instruction.get_address2string(B_addr_loop)

                    instr = ""
                    if k_simd_index == 0 :   
                        instr = "DOT_FIRST"
                    elif 0 < k_simd_index < ((A.shape[1])/elements_word-1) :
                        instr = "DOT"
                    else :
                        instr = "STORE_DOT"

                    cmd_str = caesar_instruction.get_opcode2string(instr) + addr1 + addr0

                    if(Debug):
                        print(str(hex(R_addr_loop)) + " <== " + instr + " " + str(hex(A_addr_loop)) + " " + str(hex(B_addr_loop)))

                    cmd = hex(int(cmd_str, 2))
                    dest_list.append(str(hex(R_addr_loop*4)))
                    cmd_list.append(str(cmd))

            # if(c > biggest_number):
            #     print("Overflow detected, returning (select smaller input numbers)")
            #     return (False, [], [])

            C[i,j] = c


    if not (C == R).all():
        print("MatMul is wrong!")
        print("C is ")
        print(C)
        print("expected")
        print(R)
        return (False, None, None)

    return (True, cmd_list, dest_list)

# it does M x A
def make_MatMul_32b_cmds(A_addr, B_addr, R_addr, width, A, B, R, Debug= True) :


    biggest_number = 2 ** 31

    cmd_list = []
    dest_list = []

    # Add the configuration register command for the bitwidth
    if width == 32:
        instr_width = "CSRW"
        bitwidth = caesar_instruction.get_width2string("WORD_32B")
        cmd_width = caesar_instruction.get_opcode2string(instr_width) + bitwidth + bitwidth
    else:
        print("ERR! wrong width specified\n", file=sys.stderr)
        return (False, [], [])

    cmd = hex(int(cmd_width, 2))
    dest_list.append(str(hex(R_addr)))
    cmd_list.append(str(cmd))
    print("My CRSW command is : ")
    print(cmd)
    print("My address is : ")
    print(R_addr)

    #transform the C shape from column to row as it will be stored that way in memory (i.e. continuguos)
    C = np.zeros((R.shape))
    C_reshaped = np.zeros((R.T.shape))

    #address are given in Byte, Caesar addresses words (32bit)

    A_offs_addr = int(A_addr/4)
    B_offs_addr = int(B_addr/4)
    R_offs_addr = int(R_addr/4)

    for i in range(A.shape[0]):

        for j in range(B.shape[1]):

            c = 0

            addr_dest_loop = R_offs_addr + i*C_reshaped.shape[0] + j

            for k in range(A.shape[1]):

                c += A[i,k]*B[k,j]

                # the base address are given from SW side, so byte
                A_addr_loop = A_offs_addr + i*A.shape[1] + k
                B_addr_loop = B_offs_addr + k*B.shape[1] + j

                addr1 = caesar_instruction.get_address2string(A_addr_loop)
                addr0 = caesar_instruction.get_address2string(B_addr_loop)

                instr = ""
                if k == 0 :
                    instr = "MAC_FIRST"
                elif 0 < k < A.shape[1]-1 :
                    instr = "MAC"
                else :
                    instr = "STORE_MAC"

                cmd_str = caesar_instruction.get_opcode2string(instr) + addr1 + addr0

                if(Debug):
                    print(str(hex(addr_dest_loop)) + " <== " + instr + " " + str(hex(A_addr_loop)) + " " + str(hex(B_addr_loop)))

                cmd = hex(int(cmd_str, 2))
                dest_list.append(str(hex(addr_dest_loop*4)))
                cmd_list.append(str(cmd))

            if(c > biggest_number):
                print("Overflow detected, returning (select smaller input numbers)")
                return (False, [], [])

            C[i,j] = c


    if not (C == R).all():
        print("ERR! MatMul is wrong!", file=sys.stderr)
        print("C is ")
        print(C)
        print("expected")
        print(R)
        return (False, None, None)

    return (True, cmd_list, dest_list)

# Max Poling commands
def make_MaxPool_cmds(A: np.ndarray, R: np.ndarray, A_addr: int, R_addr: int, size: int, stride: int, debug: bool = False):
    print('Building NM-Caesar commands for Max Pooling')
    
    # Decode element width
    width = A.itemsize * 8

    # Compute output dimensions
    row_a, col_a = A.shape
    row_r, col_r = R.shape

    # Check if parameters are valid
    if size < 2:
        raise ValueError('unsupported parameters - size must be at least 2')
    elem_per_word = 32 // width
    if row_a % elem_per_word != 0:
        raise ValueError(f'unsupported parameters - rows(A) must be a multiple of {elem_per_word}')
    if col_a % elem_per_word != 0:
        raise ValueError(f'unsupported parameters - cols(A) must be a multiple of {elem_per_word}')
    if (col_a - size) % stride != 0:
        raise ValueError('unsupported parameters - (rows(A) - size) / stride must be an integer')
    if (row_a - size) % stride != 0:
        raise ValueError('unsupported parameters - (cols(A) - size) / stride must be an integer')
    
    # Generate commands
    cmd_gen = cg.CaesarCmdGen()
    cmd_list = []
    dest_list = []

    # Configure element width
    print(f"width: {width}")
    (cmd, addr) = cmd_gen.get_csr_cmd(R_addr, width)
    cmd_list.append(cmd)
    dest_list.append(addr)
    if debug:
        print(cmd_gen.print_cmd(cmd, addr))

    # Iterate over the output matrix
    # NOTE: reorder nested loops to prevent RAW hazards, which are not currently handled by NM-Caesar
    if width == 32:
        # Perform entire max pooling
        for k in range(size):
            for l in range(size):
                # Generate commands to compute the max value in the current pooling window
                for i in range(row_r):
                    for j in range(col_r):
                        addr1 = A_addr + (((i*stride+k)*col_a + (j*stride+l)) << 2)
                        dest_addr = R_addr + ((i*col_r + j) << 2)
                        if k == 0 and l == 0:
                            addr2 = A_addr + (((i*stride+k)*col_a + (j*stride+l) + 1) << 2)
                        elif k == 0 and l == 1:
                            continue
                        else:
                            addr2 = dest_addr
                        # Generate command
                        (cmd, addr) = cmd_gen.get_cmd('MAX', dest_addr, addr1, addr2)
                        cmd_list.append(cmd)
                        dest_list.append(addr)
                        if debug:
                            print(cmd_gen.print_cmd(cmd, addr))

    else:
        print("WARNING! Only implementing vertical pass. Please implement horizontal pass on CPU.", file=sys.stderr)
        # Perform only vertical pass. Horizontal pass is done by the host CPU
        col_a_word = col_a // elem_per_word
        for k in range(size):
            for i in range(row_r):
                for j in range(col_a_word):
                    addr1 = A_addr + (((i*stride+k)*col_a_word + j) << 2)
                    dest_addr = R_addr + ((i*col_a_word + j) << 2)
                    if k == 0:
                        addr2 = A_addr + (((i*stride+k+1)*col_a_word + j) << 2)
                    elif k == 1:
                        continue
                    else:
                        addr2 = dest_addr
                    # Generate command
                    (cmd, addr) = cmd_gen.get_cmd('MAX', dest_addr, addr1, addr2)
                    cmd_list.append(cmd)
                    dest_list.append(addr)
                    if debug:
                        print(cmd_gen.print_cmd(cmd, addr))

    return (cmd_list, dest_list)

# Elemet-wise operations A op B with A stored in addr1 and B in addr0
def make_ElementWise_cmds(op: str, A: np.ndarray, B: np.ndarray, R: np.ndarray, A_addr: int, B_addr: int, R_addr: int, debug: bool = False):
    print('Building NM-Caesar commands for ' + op)

    # Decode the operation
    if op == 'and':
        caesar_cmd = 'AND'
    elif op == 'or':
        caesar_cmd = 'OR'
    elif op == 'xor':
        caesar_cmd = 'XOR'
    elif op == 'add':
        caesar_cmd = 'ADD'
    elif op == 'sub':
        caesar_cmd = 'SUB'
    elif op == 'mul':
        caesar_cmd = 'MULT'
    else:
        raise ValueError(f'unsupported operation: {op}')
    
    # Decode element width
    width = A.itemsize * 8

    # Compute output dimensions
    row_a, col_a = A.shape

    elem_per_word = 32 // width
    # I didn t manage to make it work when rows *column of the matrices are not a multiple of element_per_word
    if (row_a*col_a) % elem_per_word != 0:
        raise ValueError(f'unsupported parameters - rows(A)*cols(A) must be a multiple of {elem_per_word}')

    # Generate commands
    cmd_gen = cg.CaesarCmdGen()
    cmd_list = []
    dest_list = []

    # Configure element width
    print(f"width: {width}")
    (cmd, addr) = cmd_gen.get_csr_cmd(R_addr, width)
    cmd_list.append(cmd)
    dest_list.append(addr)
    if debug:
        print(cmd_gen.print_cmd(cmd, addr))

    for i in range(row_a * col_a) :
        if i % elem_per_word ==0 :
            addr1 = A_addr + (i//elem_per_word <<2)
            addr0 = B_addr + (i//elem_per_word <<2)
            dest_addr = R_addr + (i//elem_per_word <<2)
            # Generate command
            (cmd, addr) = cmd_gen.get_cmd(caesar_cmd, dest_addr, addr1, addr0)
            cmd_list.append(cmd)
            dest_list.append(addr)
            if debug:
                print(cmd_gen.print_cmd(cmd, addr))
    return (cmd_list, dest_list)

# Xor commands A^B with A stored in addr1 and B in addr0
def make_Relu_cmds(A: np.ndarray, shamt: np.ndarray, A_addr: int, shamt_addr: int, R_addr: int, debug: bool = False):
    print('Building NM-Caesar commands for Max Pooling')
    
    # Decode element width
    width = A.itemsize * 8

    # Compute output dimensions
    row_a, col_a = A.shape

    elem_per_word = 32 // width
    # I didn t manage to make it work when rows *column of the matrices are not a multiple of element_per_word
    if (row_a*col_a) % elem_per_word != 0:
        raise ValueError(f'unsupported parameters - rows(A)*cols(A) must be a multiple of {elem_per_word}')

    # Generate commands
    cmd_gen = cg.CaesarCmdGen()
    cmd_list = []
    dest_list = []

    # Configure element width
    print(f"width: {width}")
    (cmd, addr) = cmd_gen.get_csr_cmd(R_addr, width)
    cmd_list.append(cmd)
    dest_list.append(addr)
    if debug:
        print(cmd_gen.print_cmd(cmd, addr))

    # ReLU
    if shamt.flatten()[0] == 0:
        for i in range(row_a * col_a) :
            if i % elem_per_word !=0:
                continue
            addr1 = A_addr + (i//elem_per_word <<2)
            addr0 = shamt_addr
            dest_addr = R_addr + (i//elem_per_word <<2)
            # Generate command
            # I didn t implement yet the Relu operation, i need to do it with Max(a,0)
            (cmd, addr) = cmd_gen.get_cmd('MAX', dest_addr, addr1, addr0)
            cmd_list.append(cmd)
            dest_list.append(addr)
            if debug:
                print(cmd_gen.print_cmd(cmd, addr))

    # Leaky ReLU
    else:
        # Compute R = A >>> shamt first
        for i in range(row_a * col_a):
            if i % elem_per_word != 0:
                continue
            addr1 = shamt_addr
            addr0 = A_addr + (i//elem_per_word <<2)
            dest_addr = R_addr + (i//elem_per_word <<2)
            # Generate command
            (cmd, addr) = cmd_gen.get_cmd('SHIFT_R', dest_addr, addr1, addr0)
            cmd_list.append(cmd)
            dest_list.append(addr)
            if debug:
                print(cmd_gen.print_cmd(cmd, addr))
                
        # Compute the maximum
        for i in range(row_a * col_a):
            if i % elem_per_word !=0:
                continue
            addr1 = A_addr + (i//elem_per_word <<2)
            addr0 = R_addr + (i//elem_per_word <<2)
            dest_addr = R_addr + (i//elem_per_word <<2)
            # Generate command
            (cmd, addr) = cmd_gen.get_cmd('MAX', dest_addr, addr1, addr0)
            cmd_list.append(cmd)
            dest_list.append(addr)
            if debug:
                print(cmd_gen.print_cmd(cmd, addr))

    return (cmd_list, dest_list)

# GEMM commands
def make_GEMM_cmds(A: np.ndarray, B: np.ndarray, C: np.ndarray, ALPHA: np.ndarray, BETA: np.ndarray, R: np.ndarray, A_addr: int, B_addr: int, C_addr: int, ALPHA_addr: int, BETA_addr: int, R_addr: int, dtype, debug: bool = False):
    print('Building NM-Caesar commands for GEMM')
    
    # Decode element width
    width = A.itemsize * 8

    # Compute output dimensions
    row_a, col_a = A.shape
    row_c, col_c = C.shape

    elements_word = 32 // width
    if (row_c*col_c) % elements_word != 0:
        raise ValueError(f'unsupported parameters - rows(C)*cols(C) must be a multiple of {elements_word}')
    if (row_a*col_a) % elements_word != 0:
        raise ValueError(f'unsupported parameters - rows(A)*cols(A) must be a multiple of {elements_word}')
    if col_a % elements_word != 0:
        raise ValueError(f'unsupported parameters - cols(A) must be a multiple of {elements_word}')

    # Generate commands
    cmd_gen = cg.CaesarCmdGen()
    cmd_list = []
    dest_list = []

    # Configure element width
    print(f"width: {width}")
    (cmd, addr) = cmd_gen.get_csr_cmd(R_addr, width)
    cmd_list.append(cmd)
    dest_list.append(addr)
    if debug:
        print(cmd_gen.print_cmd(cmd, addr))

    #I unfortunately need to separate the addition of C and dot(A,B), i can not use a MAC operation to hide the addition of C because i would not work for 16 and 8b 
    # since i would add diffent value of C (ie C00, C01 for 16b)
    print(ALPHA)
    #Generate commands for T=alpha*A
    for i in range(row_a * col_a) :
        if i % elements_word ==0 :
            addr1 = A_addr + (i//elements_word <<2) 
            addr0 = ALPHA_addr  
            dest_addr = A_addr + (i//elements_word <<2) 
            # Generate command
            (cmd, addr) = cmd_gen.get_cmd('MULT', dest_addr, addr1, addr0)
            cmd_list.append(cmd)
            dest_list.append(addr)
            if debug:
                print(cmd_gen.print_cmd(cmd, addr))


    print(BETA)
    #Generate commands for C1=beta*C
    for i in range(row_c * col_c) :
        addr1 = C_addr + (i<<2)
        addr0 = BETA_addr  
        dest_addr = C_addr + (i <<2)
        # Generate command
        (cmd, addr) = cmd_gen.get_cmd('MULT', dest_addr, addr1, addr0)
        cmd_list.append(cmd)
        dest_list.append(addr)
        if debug:
            print(cmd_gen.print_cmd(cmd, addr))

    #Generate commands for R1 =T.B
    D = np.zeros((R.shape), dtype=dtype)
    B_transposed = B.T
    dest_addr = R_addr 
    for i in range(A.shape[0]):

        for j in range(B_transposed.shape[0]):

            c = 0

            for k in range(A.shape[1]):

                c += A[i,k]*B_transposed[j,k]

                k_simd_index = int(k / elements_word)

                if ((k % elements_word) == 0 ): # generate instructions only at the beginning of a new 32b data

                    addr0 = A_addr + ((i*int(A.shape[1]/elements_word) + k_simd_index) <<2)
                    if dtype != np.int32:
                        addr1 = B_addr + ((k_simd_index + j*int(B_transposed.shape[1]/elements_word) ) <<2)
                    else :
                        addr1 = B_addr + ((k_simd_index*int(B_transposed.shape[0]/elements_word) + j ) <<2)
                    dest_addr = R_addr + ((i * B.shape[1] +j) <<2)

                    if k_simd_index == 0 :   
                        (cmd, addr) = cmd_gen.get_cmd('DOT_FIRST', dest_addr, addr1, addr0)
                    elif 0 < k_simd_index < ((A.shape[1])/elements_word-1) :
                        (cmd, addr) = cmd_gen.get_cmd('DOT', dest_addr, addr1, addr0)
                    else :
                        (cmd, addr) = cmd_gen.get_cmd('STORE_DOT', dest_addr, addr1, addr0)
                    
                    
                    
                    cmd_list.append(cmd)
                    dest_list.append(addr)
                    if debug:
                        print(cmd_gen.print_cmd(cmd, addr))

            D[i,j] = c
            
    #Generate commands for the final addition R = R1 + C1
    for i in range(row_c * col_c) :
        
        addr1 = C_addr + (i <<2)
        addr0 = R_addr + (i <<2)
        dest_addr = R_addr + (i<<2) 
        # Generate command
        (cmd, addr) = cmd_gen.get_cmd('ADD', dest_addr, addr1, addr0)
        cmd_list.append(cmd)
        dest_list.append(addr)
        if debug:
            print(cmd_gen.print_cmd(cmd, addr))


    R_calculated = BETA[0,0]*C + ALPHA[0,0]*D
    if not ((R_calculated) == R).all():
        print("MatMul is wrong!")
        print("C is ")
        print(R_calculated)
        print("expected")
        print(R)
        return (False, None, None)
    
    return (cmd_list, dest_list)
