import numpy as np
import scipy as sp

def python2c_dumparray(input_matrix, input_name, c_type, file_ptr) :

    file_ptr.write("\n/*** ***/\n")
    file_ptr.write(c_type + " " + input_name + "[]={ ")

    nvalues = input_matrix.flatten().shape[0]
    iterator = 0
    for elem in input_matrix.flatten():
        file_ptr.write(str(int(elem)))
        if iterator == nvalues-1:
            file_ptr.write("};")
        else:
            file_ptr.write(", ")
        iterator += 1
    file_ptr.write("\n")

def python2c_dumplist(input_list, input_name, c_type, file_ptr) :

    file_ptr.write("\n/*** ***/\n")
    file_ptr.write(c_type + " " + input_name + "[]={ ")

    nvalues = len(input_list)
    iterator = 0
    for elem in input_list:
        file_ptr.write(elem)
        if iterator == nvalues-1:
            file_ptr.write("};")
        else:
            file_ptr.write(", ")
        iterator += 1
    file_ptr.write("\n")

def make2DConv(dumpM = True, header_cpu = "convolutions_data.h") :

   f = open(header_cpu, "w")
   f.write("#ifndef CONVOLUTIONS_DATA_H\n")
   f.write("#define CONVOLUTIONS_DATA_H\n")

   # i n p u t s i g n a l
   A = np.array([[1 , 2], [3, 4]])
   # f i l t e r
   K = np.array([[10 , 11], [12, 13]])

   print("Computing A conv K with A.shape= " + str(A.shape) + " and K.shape=" + str(K.shape))
   (exp, gold, M, result) = expected2DConv(A,K,False)

   f.write("#ifndef CONVOLUTIONS_DATA_H\n")
   f.write("#define CONVOLUTIONS_DATA_H\n")

   if(result):

    print("comparison successfull")

    python2c_dumparray(A, "input_A0", "int", f)
    python2c_dumparray(K, "input_K0", "int", f)
    if(dumpM):
        python2c_dumparray(M, "input_M0", "int", f)

    f.write("//output expected, i.e. A conv K or M matmul A.traspose\n")
    python2c_dumparray(exp, "output_expected_C0", "int", f)

    f.write("#define INPUT_A0_SIZE " + str(A.flatten().shape[0]) + "\n")
    f.write("#define INPUT_K0_SIZE " + str(K.flatten().shape[0]) + "\n")
    if(dumpM):
        f.write("#define INPUT_M0_SIZE " + str(M.flatten().shape[0]) + "\n")
    f.write("#define OUTPUT_EXPECTED_C0_SIZE " + str(exp.flatten().shape[0]) + "\n")

    n = A.shape[0]
    e = exp.shape[0]
    (mm_result, cmd_list) = make_MatMul_cmds(0, 0, 32, M, A.reshape(n*n,1), exp.reshape(e*e,1))

    if(mm_result):
        python2c_dumplist(cmd_list, "caesar_commads_C0", "int", f)

   else:
    print("expected")
    print(exp)

    print("gold")
    print(gold)
    return False

   # i n p u t s i g n a l
   A = np.array([[1 , 2, 3], [4, 5, 6], [7, 8, 9]])
   # f i l t e r
   K = np.array([[10 , 11], [12, 13]])

   print("Computing A conv K with A.shape= " + str(A.shape) + " and K.shape=" + str(K.shape))
   (exp, gold, M, result) = expected2DConv(A,K,False)

   if(result):

    print("comparison successfull")

    python2c_dumparray(A, "input_A1", "int", f)
    python2c_dumparray(K, "input_K1", "int", f)
    if(dumpM):
        python2c_dumparray(M, "input_M1", "int", f)

    f.write("//output expected, i.e. A conv K or M matmul A.traspose\n")
    python2c_dumparray(exp, "output_expected_C1", "int", f)

    f.write("#define INPUT_A0_SIZE " + str(A.flatten().shape[0]) + "\n")
    f.write("#define INPUT_K0_SIZE " + str(K.flatten().shape[0]) + "\n")
    if(dumpM):
        f.write("#define INPUT_M0_SIZE " + str(M.flatten().shape[0]) + "\n")
    f.write("#define OUTPUT_EXPECTED_C0_SIZE " + str(exp.flatten().shape[0]) + "\n")

    n = A.shape[0]
    e = exp.shape[0]
    (mm_result, cmd_list) = make_MatMul_cmds(0, 0, 32, M, A.reshape(n*n,1), exp.reshape(e*e,1))

    if(mm_result):
        python2c_dumplist(cmd_list, "caesar_commads_C1", "int", f)

   else:
    print("expected")
    print(exp)

    print("gold")
    print(gold)
    return False

   A = np.array([[1 ,2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]])
   # f i l t e r
   K = np.array([[20 , 21], [22, 23]])

   print("Computing A conv K with A.shape= " + str(A.shape) + " and K.shape=" + str(K.shape))
   (exp, gold, M, result) = expected2DConv(A,K,False)

   if(result):
    print("comparison successfull")

    python2c_dumparray(A, "input_A2", "int", f)
    python2c_dumparray(K, "input_K2", "int", f)
    if(dumpM):
        python2c_dumparray(M, "input_M2", "int", f)

    f.write("//output expected, i.e. A conv K or M matmul A.traspose\n")
    python2c_dumparray(exp, "output_expected_C2", "int", f)

    f.write("#define INPUT_A2_SIZE " + str(A.flatten().shape[0]) + "\n")
    f.write("#define INPUT_K2_SIZE " + str(K.flatten().shape[0]) + "\n")
    if(dumpM):
        f.write("#define INPUT_M2_SIZE " + str(M.flatten().shape[0]) + "\n")
    f.write("#define OUTPUT_EXPECTED_C2_SIZE " + str(exp.flatten().shape[0]) + "\n")

   else:
    print("expected")
    print(exp)

    print("gold")
    print(gold)
    return False

   K = np.array([[20 , 21, 22], [23, 24, 25], [26, 27, 28]])
   print("Computing A conv K with A.shape= " + str(A.shape) + " and K.shape=" + str(K.shape))
   (exp, gold, M, result) = expected2DConv(A,K,False)

   if(result):
    print("comparison successfull")

    python2c_dumparray(A, "input_A3", "int", f)
    python2c_dumparray(K, "input_K3", "int", f)
    if(dumpM):
        python2c_dumparray(M, "input_M3", "int", f)
    f.write("//output expected, i.e. A conv K or M matmul A.traspose\n")
    python2c_dumparray(exp, "output_expected_C3", "int", f)

    f.write("#define INPUT_A3_SIZE " + str(A.flatten().shape[0]) + "\n")
    f.write("#define INPUT_K3_SIZE " + str(K.flatten().shape[0]) + "\n")
    if(dumpM):
        f.write("#define INPUT_M3_SIZE " + str(M.flatten().shape[0]) + "\n")
    f.write("#define OUTPUT_EXPECTED_C3_SIZE " + str(exp.flatten().shape[0]) + "\n")


   else:
    print("expected")
    print(exp)

    print("gold")
    print(gold)
    return False

   f.write("#endif")
   f.close()

   return True

def expected2DConv(A, K, Debug) :

    #adapted from https://www.baeldung.com/cs/convolution-matrix-multiplication

    #output size t = [(W−K+2P)/S]+1
    #W is the input volume - in your case 128
    #K is the Kernel size - in your case 5
    #P is the padding - in your case 0 i believe
    #S is the stride - which you have not provided.
    Output_gold = sp.signal.correlate2d(A, K, mode='valid', boundary='fill', fillvalue=0)

    n = A.shape[0]
    k = K.shape[0]
    t = n - k + 1
    #padding = 0 #do not change this
    #stride = 1 #do not change this
    #t = 1 + (n - k + 2*padding)/stride = n - k + 1

    if (t != Output_gold.shape[0]):
        print("t output size is wrong! z=" + str(t) + ", it should be " + str(Output_gold.shape[0]))


    Ki = np.zeros((k, t, n))
    Khat = []
    for i in range(k):
        c = np.zeros((t,1))
        c[0,0] = K[i,0]
        r = np.zeros((1,n))
        r[0][0:k] = K[i]
        Ki[i] = sp.linalg.toeplitz(c,r)
        if i > 0:
            Khat = np.concatenate((Khat,Ki[i]), axis=1)
        else:
            Khat = Ki[i]

    if(Debug):
        print("Khat of shape " + str(Khat.shape) + " is:")
        print(Khat)
    # M should be t^2 * n^2
    if (t >= k):
        M = np.zeros((t*t,n*n))
    else:
        M = np.zeros((k*k,n*n))

    if(Debug):
        print("Building M preliminary of shape " + str(M.shape))

    for m in range(t):
        if(Debug):
            print("Iteration: " + str(m) + " processing M index: ")
            print(str(m*t) + ":" + str(m*t+Khat.shape[0]))
            print(str(m*n) + ":" + str(m*n+Khat.shape[1]))

        M[m*t:m*t+Khat.shape[0],m*n:m*n+Khat.shape[1]] = Khat
        if(Debug):
            print("M is: ")
            print(M)

    # M should be t^2 * n^2
    M = M[0:t*t,0:n*n]
    if(Debug):
        print("Reshaping M to " + str(M.shape))
        print(M)

    v = A.reshape(n*n)
    Output_exp = np.dot(M, v.T).reshape(Output_gold.shape)

    return (Output_exp, Output_gold, M, (Output_exp == Output_gold).all())


def hex2bin(hex_int) : 

    hex_string = hex(hex_int)[2:]

    #format data
    if len(hex_string) > 4 : 
        hex_string = hex_string[-4:]
    elif len(hex_string) == 1 : 
        hex_string = "000" + hex_string
    elif len(hex_string) == 2 : 
        hex_string = "00" + hex_string
    elif len(hex_string) == 3 : 
        hex_string = "0" + hex_string
    else : 
        hex_string = hex_string

    hex_bin = ""

    for x in hex_string:
        if x == '0' : 
            hex_bin += '0000'
        elif x == '1' :
            hex_bin += "0001"
        elif x == '2' :
            hex_bin += "0010"
        elif x == '3' :
            hex_bin += "0011"
        elif x == '4' :
            hex_bin += "0100"
        elif x == '5' :
            hex_bin += "0101"
        elif x == '6' :
            hex_bin += "0110"
        elif x == '7' :
            hex_bin += "0111"
        elif x == '8' :
            hex_bin += "1000"
        elif x == '9' :
            hex_bin += "1001"
        elif x == 'a' :
            hex_bin += "1010"
        elif x == 'b' :
            hex_bin += "1011"
        elif x == 'c' :
            hex_bin += "1100"
        elif x == 'd' :
            hex_bin += "1101"
        elif x == 'e' :
            hex_bin += "1110"
        else :
            hex_bin += "1111"

    return hex_bin

def str_bin2hex(str_bin) : 

    result = ""
    new_byte = ""

    i=0

    while i < len(str_bin) :
        new_byte = str_bin[i] + str_bin[i+1] + str_bin[i+2] + str_bin[i+3]
        i+=4
        if new_byte == '0000' : 
            result += '0'
        elif new_byte == "0001" :
            result += '1'
        elif new_byte == "0010" :
            result += '2'
        elif new_byte == "0011" :
            result += '3'
        elif new_byte == "0100" :
            result += '4'
        elif new_byte == "0101" :
            result += '5'
        elif new_byte == "0110" :
            result += '6'
        elif new_byte == "0111" :
            result += '7'
        elif new_byte == "1000" :
            result += '8'
        elif new_byte == "1001" :
            result += '9'
        elif new_byte == "1010" :
            result += 'a'
        elif new_byte == "1011" :
            result += 'b'
        elif new_byte == "1100" :
            result += 'c'
        elif new_byte == "1101" :
            result += 'd'
        elif new_byte == "1110" :
            result += 'e'
        else :
            result += 'f'
    return hex(int(result,16))
