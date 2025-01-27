## List of kernels:
- stft
- normalize
- dense (computeDense)
- add
- gelu (activation)
- MatMul_scale
- MatMul_multiply
- softmax (computeSoftmax)
- clsConcatenate
- multihead_transpose
- posEmbedding
- transpose_quant


## Sizes in TSD
- stft:
    - stftVec = 160 * 15 = 2400 = 120 * 20 (120 is exactly D_SEQ)
    - rawInputSignal = 160 * 15 * 20 - 16 * 1024 = 16 * 1976
    - out = 2048
    - qkv = 2048
    - intermediate = &out - &intermediate = 160 * 15 * 20 - 16 * 1024 = 16 * 1976
    - input_normalized = TOTAL - (&out + 4096) = 63840 - (160 * 15 * 20 + 4096) = 11744

- normalize:
    - transformerBlock->addNorm = createAddNormalize(pre_seq_len, D_EMBEDDING, weightVector[0], biasVector[0]); //pre_seq_len=120, D_EMBEDDING=400

- dense:
    - For Q,K, and V: 
        - input A: %d X %d\n", seq_len, dense->input_size_ Where seq_len=121, input_size_=16
        - input B: %d X %d\n", dense->input_size_, dense->output_size_, where input_size_=16, output_size=4

- transpose:
    - transpose_quant(self_attn->key_layer_out, self_attn->key_transposed_layer_out, self_attn->pre_seq_len, self_attn->head_hidden_size);
        - pre_seq_len=16, head_hidden_size=4

- matmul_scale:
    - MatMul_scale(self_attn->key_transposed_layer_out, 1, self_attn->pre_seq_len * self_attn->head_hidden_size);
        - matmul_size = 121 * 4, shift=1

- perform Q x K^T
    - MatMul_multiply(self_attn->pre_seq_len, self_attn->query_layer_out, self_attn->key_transposed_layer_out, intermediate, self_attn->head_hidden_size, self_attn->pre_seq_len);
        - printf("input A: %d X %d\n", seq_len, input_size); --> seq_len=121, input_size=4
        - printf("input B: %d X %d\n", input_size, output_size); --> input_size=4, output_size=121

- intermediate = 121x121
- computeSoftmax(intermediate, self_attn->pre_seq_len);
    - intermediate is holding Q x K^T output (which is 121x121), pre_seq_len=121

- softMax(Q x K^T) x V
    - MatMul_multiply(self_attn->pre_seq_len, intermediate, self_attn->value_layer_out, output, self_attn->pre_seq_len, self_attn->head_hidden_size);
        - input A: seq_len=121, input_size=121
        - input B: input_size=121, output_size=4
        - intermediate is already softmax applied


- AFTER SINGLE HEAD!
- multihead_transpose(output, intermediate, seq_len, transformerBlock->head_hidden_size_, transformerBlock->num_heads_);
    - seq_len=121, head_hidden_size_=4, num_heads_=4
    - applied on intermediate

- computeDense(transformerBlock->condense[0], seq_len, intermediate, output);
    - seq_len=121
    - createDense(transformerBlock->condense[l], num_heads * head_hidden_size, input_dim, weightVector[l * 17 + num_heads * 3 + 4], biasVector[l * 17 + num_heads * 3 + 4]);
    - so input_size=4*4, output_size=input_dim=16
    - so input A: seq_len, dense->input_size_ Where 121x16
        - input B: %d X %d\n", dense->input_size_, dense->output_size_, where 16x16

- add(input, output, seq_len, transformerBlock->input_dim_);
    - seq_len=121, input_dim_=16
    - input is passed

- normalize(&transformerBlock->transformer_layer_1_addNorm[0], input, input_normalized);
    - 

- computeDense(transformerBlock->feedForward0[0], seq_len, input_normalized, intermediate);
    - 121 x A=16 ) x (A=16 x ff=4

- activation(transformerBlock->feedForward0[0], seq_len * transformerBlock->ff_size_, intermediate, intermediate);
    - length= 121*4

- computeDense(transformerBlock->feedForward0[0], seq_len, input_normalized, intermediate);
    - 121 x 4 ) x (4x 16

- add(input, output, seq_len, transformerBlock->input_dim_);
    - seq_len=121, input_dim_=16

- REPEATED FOR ALL LAYERS
    - "input" is always updated when going forward 

-  normalize(&transformerBlock->mlp_head_norm, input, input_normalized); 
- computeDense(transformerBlock->mlp_head_linear, 1, input_normalized, output);


scl enable devtoolset-10 bash 
source ~/.bashrc 
source heepatia/scripts/env.sh 

./run_simulations.sh softmax_121 "PROJECT=transformer_kernels/softmax PARAM_SEQ_LEN=121"

./run_simulations.sh gelu_484 "PROJECT=transformer_kernels/gelu PARAM_DATA_SIZE=484"


./run_simulations.sh MatMul_scale_121_16 "PROJECT=transformer_kernels/MatMul_scale PARAM_DATA_SIZE=1"

./run_simulations.sh transpose_121_16 "PROJECT=transformer_kernels/transpose PARAM_SEQ_LEN=121 PARAM_INPUT_DIM=4"



./run_simulations.sh multihead_transpose_121_4_4 "PROJECT=transformer_kernels/multihead_transpose PARAM_SEQ_LEN=121 PARAM_HEAD_HIDDEN_SIZE=4 PARAM_NUM_HEADS=4"
./run_simulations.sh add_121_16 "PROJECT=transformer_kernels/add PARAM_SEQ_LEN=121 PARAM_INPUT_DIM=16"
./run_simulations.sh MatMul_multiply_121_121_4 "PROJECT=transformer_kernels/MatMul_multiply PARAM_SEQ_LEN=121 PARAM_INPUT_DIM=121 PARAM_OUTPUT_DIM=4"
./run_simulations.sh posEmbedding_120_16 "PROJECT=transformer_kernels/posEmbedding PARAM_SEQ_LEN=120 PARAM_INPUT_DIM=16"
./run_simulations.sh normalize_121_16 "PROJECT=transformer_kernels/normalize PARAM_SEQ_LEN=121 PARAM_INPUT_DIM=16"
./run_simulations.sh dense1_16_16 "PROJECT=transformer_kernels/dense PARAM_SEQ_LEN=1 PARAM_INPUT_DIM=16 PARAM_OUTPUT_DIM=16"



