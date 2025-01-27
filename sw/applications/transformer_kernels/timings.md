# Timings of transformer_C
Start
FFT cycles: 221987774

normalize time: 2101403

D multiplyweight (dense matmul)
input A: 120 X 400
input B: 400 X 16
dense time: 8772029

D normalize time: 303126

D clsConcatenate time: 13623

D posEmbedding time: 20482

D normalize time[0]: 306624

D multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4

multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
D MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
D MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[0][0]: 47956690

multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[0][1]: 50102293

multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[0][2]: 49180893

multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[0][3]: 47712906

transpose time[0]: 19873

multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 16

condense time[0]: 1370878

add time[0]: 29067

normalize time[0]: 306579
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
dense time[0]: 1092995

activation time[0]: 2103895

D multiplyweight (dense matmul)
input A: 121 X 4
input B: 4 X 16
dense time[0]: 1107303
add time[0]: 29068
normalize time[1]: 306663
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[1][0]: 46122066
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[1][1]: 45206331
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[1][2]: 46271133
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[1][3]: 46297097
transpose time[1]: 19873
D multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 16
condense time[1]: 1370878
add time[1]: 29067
normalize time[1]: 306552
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
dense time[1]: 1093005
activation time[1]: 1944402
multiplyweight (dense matmul)
input A: 121 X 4
input B: 4 X 16
dense time[1]: 1107303
add time[1]: 29068
normalize time[2]: 306582
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[2][0]: 49294358
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[2][1]: 47209713
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[2][2]: 44411506
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[2][3]: 49711613
transpose time[2]: 19873
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 16
condense time[2]: 1370878
add time[2]: 29067
normalize time[2]: 306548
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
dense time[2]: 1092995
activation time[2]: 1984434
multiplyweight (dense matmul)
input A: 121 X 4
input B: 4 X 16
dense time[2]: 1107303
add time[2]: 29068
normalize time[3]: 305730
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[3][0]: 47096413
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[3][1]: 43878802
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[3][2]: 45702080
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
MatMul_multiply (self-attention)
input A: 121 X 4
input B: 4 X 121
MatMul_multiply (self-attention)
input A: 121 X 121
input B: 121 X 4
single head time[3][3]: 48391131
transpose time[3]: 19873
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 16
condense time[3]: 1370848
add time[3]: 29067
normalize time[3]: 306569
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
dense time[3]: 1092995
activation time[3]: 2286915
multiplyweight (dense matmul)
input A: 121 X 4
input B: 4 X 16
dense time[3]: 1106417
add time[3]: 29068
normalize time: 2599
multiplyweight (dense matmul)
input A: 1 X 16
input B: 16 X 16
dense time: 990616
Total cycles: 56616929
Distances : 
From the prototype of class 0 = 462
From the prototype of class 1 = 107749

## Self Attention part

transpose time: 3448
scale time: 3898
matmul time: 762352
softmax time: 39353387
matmul time: 592468

transpose time: 3448
, width 121, height 4

scale time: 3898
, shift_scale 1, mat_size 484

matmul time: 762353
, seq_len 121, input_size 4, output_size 121

softmax time: 41198788
, seq_len 121

matmul time: 592468
, seq_len 121, input_size 121, output_size 4

# Seperated Kernels
normalize: seq=100, in_dim=400
NOT FITTING

Kernel: normalize, SEQ_LEN: 120, INPUT_DIM: 200
normalize time: 1164325
SIMILAR

Kernel: dense, SEQ_LEN: 120, INPUT_DIM: 400, OUTPUT_DIM: 16
multiplyweight (dense matmul)
input A: 120 X 400
input B: 400 X 16
computeDense time: 8771313
SIMILAR

Kernel: normalize, SEQ_LEN: 120, INPUT_DIM: 16
normalize time: 301226
SIMILAR

Kernel: clsConcatenate, SEQ_LEN: 120, INPUT_DIM: 16
clsConcatenate time: 15525
SIMILAR

Kernel: posEmbedding, SEQ_LEN: 120, INPUT_DIM: 16
posEmbedding time: 20606
SIMILAR

Kernel: normalize, SEQ_LEN: 121, INPUT_DIM: 16
normalize time: 303815
SIMILAR

Kernel: dense, SEQ_LEN: 121, INPUT_DIM: 16, OUTPUT_DIM: 4
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 4
computeDense time: 1092787
--
Kernel: dense, SEQ_LEN: 121, INPUT_DIM: 4, OUTPUT_DIM: 121
multiplyweight (dense matmul)
input A: 121 X 4
input B: 4 X 121
computeDense time: 1884133
--
Kernel: dense, SEQ_LEN: 121, INPUT_DIM: 121, OUTPUT_DIM: 4
multiplyweight (dense matmul)
input A: 121 X 121
input B: 121 X 4
computeDense time: 1632248
--
SIMILAR

Kernel: multihead_transpose, SEQ_LEN: 121, HEAD_HIDDEN_SIZE: 4, NUM_HEADS: 4
multihead_transpose time: 18310
SIMILAR

Kernel: dense, SEQ_LEN: 121, INPUT_DIM: 16, OUTPUT_DIM: 16
multiplyweight (dense matmul)
input A: 121 X 16
input B: 16 X 16
computeDense time: 1369783
SIMILAR

Kernel: MatMul_multiply, SEQ_LEN: 121, INPUT_DIM: 16, OUTPUT_DIM: 16
MatMul_multiply (self-attention)
input A: 121 X 16
input B: 16 X 16
MatMul_multiply time: 1428655

Kernel: add, SEQ_LEN: 121, INPUT_DIM: 16
add time: 29792
SIMILAR

Kernel: activation(gelu), DATA_SIZE: 484
activation time: 2245570
SIMILAR

Kernel: transpose, SEQ_LEN: 121, INPUT_DIM: 4
transpose time: 3934
SIMILAR

D Kernel: MatMul_scale, MAT_SIZE: 484, SHIFT_SCALE: 1
MatMul_scale time: 3901
SIMILAR

softmax time: 46190408
Kernel: softmax, SEQ_LEN: 121
SIMILAR

# ALL THE KERNELS ARE CORRECT!
