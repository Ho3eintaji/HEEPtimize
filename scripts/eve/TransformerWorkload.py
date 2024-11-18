# TSD Network Detailed Computational Analysis Script

import math
import json

def calculate_tsd_operations(
    T=12,                     # Duration of EEG signal in seconds
    F_sampling=256,           # Sampling frequency in Hz
    C=2000,                     # Number of EEG channels
    S=10,                      # Number of splits
    Num_bands=500,              # Number of frequency bands in STFT
    d_fft=1280,                # Dimensionality of STFT output per band
    d_m=128*128,                   # Embedding dimension
    Num_encoders=30,           # Number of encoder blocks
    Num_heads=100,              # Number of attention heads
    d_q=128,                    # Dimension of Q vectors
    d_k=128,                    # Dimension of K vectors
    d_v=128,                    # Dimension of V vectors
    d_ff=2048,                   # Dimension in FFN
    Num_classes=10,            # Number of output classes
    verbose=True              # Control verbosity of the function
):
    # Initialize a list to store operations
    operations = []
    operation_counter = 1  # Counter to assign execution order

    if verbose:
        print("=== TSD Network Computational Analysis ===\n")
    
    # Phase 1: Preprocessing (STFT)
    if verbose:
        print("### Phase 1: Preprocessing (STFT) ###")
    N_samples = T * F_sampling
    N_STFTs = C * S
    len_seq = C * S * Num_bands
    samples_per_segment = int(N_samples / S)
    if verbose:
        print(f"- Duration of EEG signal: {T} seconds")
        print(f"- Number of splits in time (S): {S}")
        print(f"- Number of channels (C): {C}")
        print(f"- Samples per segment: {samples_per_segment}")
        # print(f"- Total samples per channel: {N_samples}")
        print(f"- Number of STFTs to perform: {N_STFTs}")
        print(f"- Number of frequency bands per STFT: {Num_bands}")
        # print(f"- Total STFT outputs (len_seq): {len_seq}")
        print(f"- STFT output per frequency band: vector of size {d_fft}")
        print(f"- STFT output matrix size: ({len_seq}, {d_fft})\n")
        # print("- STFT Operations:")
    
    stft_operation_ids = []
    for channel_idx in range(1, C + 1):
        for split_idx in range(1, S + 1):
            for band_idx in range(1, Num_bands + 1):
                op_id = f"STFT_C{channel_idx}_S{split_idx}_B{band_idx}"
                operations.append({
                    'operation_id': op_id,
                    'execution_order': 1,  # All STFTs can run in parallel
                    'layer': 'Preprocessing',
                    'operation_type': 'STFT',
                    'details': {
                        'channel': channel_idx,
                        'segment': split_idx,
                        'band': band_idx
                    },
                    'input_size': [samples_per_segment],  # 1D input
                    'output_size': [d_fft],  # 1D output
                    'dependencies': [],  # No dependencies
                })
                stft_operation_ids.append(op_id)
    if verbose:
        print(f"- Total number of STFT operations: {len_seq}\n")

    # Update operation counter
    operation_counter += 1  # Next execution order after STFTs

    # Phase 2: Embedding Layer
    if verbose:
        print("### Phase 2: Embedding Layer ###")
        # print(f"- Input to embedding layer: matrix of size ({len_seq}, {d_fft})")
        # print(f"- Embedding weight matrix size: ({d_fft}, {d_m})")
        print(f"- MatMul: ({len_seq}, {d_fft}) x ({d_fft}, {d_m}) => Output: ({len_seq}, {d_m})\n")
    # Store Embedding MatMul operation
    operations.append({
        'operation_id': 'Embedding_MatMul',
        'execution_order': operation_counter,
        'layer': 'Embedding Layer',
        'operation_type': 'MatMul',
        'details': 'Embedding projection',
        'input_description': 'Concatenation of all STFT outputs',
        'input_size': [len_seq, d_fft],
        'weight_size': [d_fft, d_m],
        'output_size': [len_seq, d_m],
        'dependencies': stft_operation_ids,  # Depends on all STFTs
    })
    operation_counter += 1

    len_seq += 1  # Adding [CLS] token
    if verbose:
        print(f"- Sequence length after adding [CLS] token: {len_seq}\n")

    # Phase 3: Transformer Encoder Blocks
    if verbose:
        print("### Phase 3: Transformer Encoder Blocks ###")
        print(f"- Number of encoder blocks: {Num_encoders}")
    previous_output_id = 'Embedding_MatMul'  # Initial dependency
    for encoder_idx in range(Num_encoders):
        if verbose:
            print(f"\n#### Encoder Block {encoder_idx + 1} ####")
        encoder_operations = []

        # Multi-Head Attention (MHA)
        if verbose:
            print("- Multi-Head Attention (MHA):")
            # print(f"  * Input matrix size: ({len_seq}, {d_m})")
            # print(f"  * Number of heads: {Num_heads}")
            # print("  * Q, K, V Calculations for each head:")
        qkv_operation_ids = []
        for head_idx in range(Num_heads):
            # Q calculation
            op_id_q = f"Encoder{encoder_idx+1}_Head{head_idx+1}_Q"
            operations.append({
                'operation_id': op_id_q,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'Q'
                },
                'input_description': f"Output from previous layer ({previous_output_id})",
                'input_size': [len_seq, d_m],
                'weight_size': [d_m, d_q],
                'output_size': [len_seq, d_q],
                'dependencies': [previous_output_id],
            })
            # K calculation
            op_id_k = f"Encoder{encoder_idx+1}_Head{head_idx+1}_K"
            operations.append({
                'operation_id': op_id_k,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'K'
                },
                'input_description': f"Output from previous layer ({previous_output_id})",
                'input_size': [len_seq, d_m],
                'weight_size': [d_m, d_k],
                'output_size': [len_seq, d_k],
                'dependencies': [previous_output_id],
            })
            # V calculation
            op_id_v = f"Encoder{encoder_idx+1}_Head{head_idx+1}_V"
            operations.append({
                'operation_id': op_id_v,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'V'
                },
                'input_description': f"Output from previous layer ({previous_output_id})",
                'input_size': [len_seq, d_m],
                'weight_size': [d_m, d_v],
                'output_size': [len_seq, d_v],
                'dependencies': [previous_output_id],
            })
            # qkv_operation_ids.extend([op_id_q, op_id_k, op_id_v])
            if verbose:
                # print(f"    - Head {head_idx + 1}: Q, K, V MatMuls")
                print(f"    - Head {head_idx + 1}: Q matmul: ({len_seq, d_m}) x ({d_m, d_q}) => ({len_seq, d_q})")
                print(f"    - Head {head_idx + 1}: K matmul: ({len_seq, d_m}) x ({d_m, d_k}) => ({len_seq, d_k})")
                print(f"    - Head {head_idx + 1}: V matmul: ({len_seq, d_m}) x ({d_m, d_v}) => ({len_seq, d_v})")
        

        # QK^T and Softmax for each head + H calculation are missing
        # print(f"    - Attention scores MatMul: ({len_seq} x {d_q}) x ({len_seq} x {d_k})^T => ({len_seq} x {len_seq})")
        # print(f"    - Attention output MatMul: ({len_seq} x {len_seq}) x ({len_seq} x {d_v}) => ({len_seq} x {d_v})") #TODO: check if this is correct
        operation_counter += 1  # Increment after Q, K, V calculations
        qkt_ids = []  
        for head_idx in range(Num_heads):
            op_id_q = f"Encoder{encoder_idx+1}_Head{head_idx+1}_Q"
            op_id_k = f"Encoder{encoder_idx+1}_Head{head_idx+1}_K"
            op_id_v = f"Encoder{encoder_idx+1}_Head{head_idx+1}_V"

            op_id_qkt = f"Encoder{encoder_idx+1}_Head{head_idx+1}_QK^T"
            operations.append({
                'operation_id': op_id_qkt,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'Attention Score (QK^T)'
                },
                'input_description': f"Q ({op_id_q}), K^T ({op_id_k})",
                'input_size':[len_seq, d_q],
                'weight_size': [d_k, len_seq],
                'output_size': [len_seq, len_seq],
                'dependencies': [op_id_q, op_id_k],
            })
            qkt_ids.append(op_id_qkt)
            if verbose:
                # print(f"    - Head {head_idx + 1}: Attention Score (QK^T) MatMul")
                print(f"    - Head {head_idx + 1}: Attention Score (QK^T) MatMul: ({d_q}, {d_k}) x ({d_k}, {len_seq}) => ({len_seq}, {len_seq})")
        
        operation_counter += 1  # Increment after all QK^T calculations
        qkt_softmax_ids = []
        for head_idx in range(Num_heads):
            op_id_qkt = f"Encoder{encoder_idx+1}_Head{head_idx+1}_QK^T"
            op_id_qkt_softmax = f"Encoder{encoder_idx+1}_Head{head_idx+1}_QK^T_Softmax"
            operations.append({
                'operation_id': op_id_qkt_softmax,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'Softmax',
                'details': {
                    'head': head_idx + 1,
                    'type': 'Attention Score Softmax'
                },
                'input_description': f"Attention Scores ({op_id_qkt})",
                'input_size': [len_seq, len_seq],
                'output_size': [len_seq, len_seq],
                'dependencies': [op_id_qkt],
            })
            qkt_softmax_ids.append(op_id_qkt_softmax)
            if verbose:
                # print(f"    - Head {head_idx + 1}: Attention Score Softmax")
                print(f"    - Head {head_idx + 1}: Attention Score Softmax: ({len_seq}, {len_seq})")
                
        operation_counter += 1  # Increment after all Softmax calculations
        h_ids = []
        for head_idx in range(Num_heads):
            op_id_qkt_softmax = f"Encoder{encoder_idx+1}_Head{head_idx+1}_QK^T_Softmax"
            op_id_v = f"Encoder{encoder_idx+1}_Head{head_idx+1}_V"
            op_id_h = f"Encoder{encoder_idx+1}_Head{head_idx+1}_H"
            operations.append({
                'operation_id': op_id_h,
                'execution_order': operation_counter,
                'layer': f'Encoder Block {encoder_idx + 1}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'Attention Output (H)'
                },
                'input_description': f"Softmax ({op_id_qkt_softmax}), V ({op_id_v})",
                'input_size': [len_seq, len_seq],
                'weight_size': [len_seq, d_v],
                'output_size': [len_seq, d_v],
                'dependencies': [op_id_qkt_softmax, op_id_v],
            })
            h_ids.append(op_id_h)
            if verbose:
                # print(f"    - Head {head_idx + 1}: Attention Output (V) MatMul")
                print(f"    - Head {head_idx + 1}: Attention Output (V) MatMul: ({len_seq}, {len_seq}) x ({len_seq}, {d_v}) => ({len_seq}, {d_v})")

        operation_counter += 1  # Increment after attention calculations

        # Final linear projection
        op_id_proj = f"Encoder{encoder_idx+1}_MHA_Output_Projection"
        operations.append({
            'operation_id': op_id_proj,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'MatMul',
            'details': 'Final linear projection after concatenation',
            'input_description': f"Concatenation of attention outputs from all heads ({h_ids})",
            'input_size': [len_seq, d_v * Num_heads],
            'weight_size': [d_v * Num_heads, d_m],
            'output_size': [len_seq, d_m],
            'dependencies': h_ids,
        })
        if verbose:
            # print("  * Output Projection MatMul")
            print(f"  * Output Projection MatMul: ({len_seq}, {d_v * Num_heads}) x ({d_v * Num_heads}, {d_m}) => ({len_seq}, {d_m})")
        operation_counter += 1

        # Residual Connection and LayerNorm after MHA
        op_id_res_ln = f"Encoder{encoder_idx+1}_MHA_Residual_LayerNorm"
        operations.append({
            'operation_id': op_id_res_ln,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'Residual_LayerNorm',
            'details': 'Residual connection and LayerNorm after MHA',
            'input_description': [previous_output_id, op_id_proj],
            'input_size': [len_seq, d_m],
            'output_size': [len_seq, d_m],
            'dependencies': [previous_output_id, op_id_proj],
        })
        if verbose:
            # print("  * Residual Connection and LayerNorm after MHA")
            print(f"  * Residual Connection and LayerNorm after MHA: ({len_seq}, {d_m})")
        operation_counter += 1
        previous_output_id = op_id_res_ln  # Update previous output

        # Feed-Forward Network (FFN)
        if verbose:
            print("\n- Feed-Forward Network (FFN):")
        # First Linear Layer
        op_id_ffn1 = f"Encoder{encoder_idx+1}_FFN_Layer1"
        operations.append({
            'operation_id': op_id_ffn1,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'MatMul',
            'details': 'First Linear Layer in FFN',
            'input_description': previous_output_id,
            'input_size': [len_seq, d_m],
            'weight_size': [d_m, d_ff],
            'output_size': [len_seq, d_ff],
            'dependencies': [previous_output_id],
        })
        if verbose:
            # print("  * First Linear Layer MatMul")
            print(f"  * First Linear Layer MatMul: ({len_seq}, {d_m}) x ({d_m}, {d_ff}) => ({len_seq}, {d_ff})")
        operation_counter += 1

        # GeLU activation after first FFN layer
        op_id_gelu1 = f"Encoder{encoder_idx+1}_FFN_GeLU1"
        operations.append({
            'operation_id': op_id_gelu1,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'Activation',
            'details': 'GeLU activation after first FFN layer',
            'input_description': op_id_ffn1,
            'input_size': [len_seq, d_ff],
            'output_size': [len_seq, d_ff],
            'dependencies': [op_id_ffn1],
        })
        if verbose:
            # print("  * GeLU activation after first FFN layer")
            print(f"  * GeLU activation after first FFN layer: ({len_seq}, {d_ff})")
        # Second Linear Layer
        op_id_ffn2 = f"Encoder{encoder_idx+1}_FFN_Layer2"
        operations.append({
            'operation_id': op_id_ffn2,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'MatMul',
            'details': 'Second Linear Layer in FFN',
            'input_description': op_id_gelu1,
            'input_size': [len_seq, d_ff],
            'weight_size': [d_ff, d_m],
            'output_size': [len_seq, d_m],
            'dependencies': [op_id_gelu1],
        })
        if verbose:
            # print("  * Second Linear Layer MatMul")
            print(f"  * Second Linear Layer MatMul: ({len_seq}, {d_ff}) x ({d_ff}, {d_m}) => ({len_seq}, {d_m})")
        operation_counter += 1

        # GeLU activation after second FFN layer
        op_id_gelu2 = f"Encoder{encoder_idx+1}_FFN_GeLU2"
        operations.append({
            'operation_id': op_id_gelu2,
            'execution_order': operation_counter,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'Activation',
            'details': 'GeLU activation after second FFN layer',
            'input_description': op_id_ffn2,
            'input_size': [len_seq, d_m],
            'output_size': [len_seq, d_m],
            'dependencies': [op_id_ffn2],
        })
        if verbose:
            # print("  * GeLU activation after second FFN layer")
            print(f"  * GeLU activation after second FFN layer: ({len_seq}, {d_m})")
        # Residual Connection and LayerNorm after FFN
        op_id_res_ln_ffn = f"Encoder{encoder_idx+1}_FFN_Residual_LayerNorm"
        operations.append({
            'operation_id': op_id_res_ln_ffn,
            'execution_order': operation_counter + 1,
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation_type': 'Residual_LayerNorm',
            'details': 'Residual connection and LayerNorm after FFN',
            'input_description': [previous_output_id, op_id_gelu2],
            'input_size': [len_seq, d_m],
            'output_size': [len_seq, d_m],
            'dependencies': [previous_output_id, op_id_gelu2],
        })
        if verbose:
            # print("  * Residual Connection and LayerNorm after FFN")
            print(f"  * Residual Connection and LayerNorm after FFN: ({len_seq}, {d_m})")
        operation_counter += 1
        previous_output_id = op_id_res_ln_ffn  # Update previous output

    # Phase 4: Classification Layer
    if verbose:
        print("\n### Phase 4: Classification Layer ###")
        print("- Using [CLS] token output")
        # print(f"- Input vector size: (1, {d_m})")
        # print(f"- Classification weight matrix size: ({d_m}, {Num_classes})")
        print(f"- MatMul: (1, {d_m}) x ({d_m}, {Num_classes}) => Output: (1, {Num_classes})")
    # Store Classification operation
    op_id_cls = 'Classification_MatMul'
    operations.append({
        'operation_id': op_id_cls,
        'execution_order': operation_counter,
        'layer': 'Classification Layer',
        'operation_type': 'MatMul',
        'details': 'Final classification layer',
        'input_description': f"[CLS] token from {previous_output_id}",
        'input_size': [1, d_m],
        'weight_size': [d_m, Num_classes],
        'output_size': [1, Num_classes],
        'dependencies': [previous_output_id],
    })
    if verbose:
        # print("\nNote: Softmax activation applied after classification (typically negligible in computational cost)\n")
        print(f"- softmax: ({Num_classes})")
    operation_counter += 1

    # Return operations and total FLOPs (FLOPs calculation can be added if needed)
    total_flops = 0  # Placeholder for FLOPs calculation

    return {
        'operations': operations,
        'total_flops': total_flops
    }

    
    # Return operations and total FLOPs
    # return {
    #     'operations': operations,
    #     'total_flops': total_flops,
    #     'variables': {
    #         'len_seq': len_seq,
    #         'd_m': d_m,
    #         'd_fft': d_fft,
    #         'd_q': d_q,
    #         'd_k': d_k,
    #         'd_v': d_v,
    #         'd_ff': d_ff,
    #         'Num_classes': Num_classes,
    #         # Add other variables as needed
    #     }
    # }

def generate_workload_from_operations(operations):
    """
    Generates a workload suitable for the emulator from the list of operations.

    Args:
        operations (list): List of operation dictionaries.

    Returns:
        list: Workload for the emulator.
    """
    workload = []
    for op in operations:
        if op['operation_type'] == 'MatMul':
            input_size = op.get('input_size')
            weight_size = op.get('weight_size')
            # For MatMul, we need M, K, N
            if input_size is None or weight_size is None:
                continue  # Skip if sizes are not provided
            if len(input_size) != 2 or len(weight_size) != 2:
                continue  # Skip if dimensions are not as expected
            M, K_input = input_size
            K_weight, N = weight_size
            if K_input != K_weight:
                continue  # Skip if inner dimensions do not match
            # Append operation to workload
            workload.append({
                'row_a': M,
                'col_a': K_input,
                'col_b': N
            })
    return workload

def calculate_albert_operations(
    vocab_size=30000,         # Vocabulary size for ALBERT (used for factorized embeddings)
    d_e=128,                  # Factorized embedding dimension
    d_m=768,                  # Hidden dimension
    Num_encoders=12,          # Number of encoder layers
    Num_heads=12,             # Number of attention heads
    d_ff=3072,                # Dimension in FFN (4 * d_m for ALBERT-Base)
    Num_classes=2,            # Number of output classes (e.g., for classification)
    max_seq_len=512,          # Maximum sequence length
    shared_parameters=True,   # Whether to share parameters across layers
    verbose=True              # Control verbosity of the function
):
    """
    Calculate the workload operations for the ALBERT architecture.

    :param vocab_size: The size of the vocabulary.
    :param d_e: The embedding dimension (factorized).
    :param d_m: The hidden dimension.
    :param Num_encoders: The number of encoder layers (if parameters are shared, this indicates repetition).
    :param Num_heads: The number of attention heads.
    :param d_ff: The intermediate dimension for the FFN.
    :param Num_classes: Number of output classes for classification.
    :param max_seq_len: Maximum sequence length.
    :param shared_parameters: Boolean indicating parameter sharing across layers.
    :param verbose: Bool indicating whether to print details.
    :return: A dictionary containing operations and total FLOPs (placeholder).
    """
    operations = []
    operation_counter = 1
    
    # Each head dimension for Q, K, V
    head_dim = d_m // Num_heads

    if verbose:
        print("=== ALBERT Network Computational Analysis ===\n")
    
    # Phase 1: Embedding Layer with Factorization
    if verbose:
        print("### Phase 1: Embedding Layer ###")
        print(f"Vocabulary size: {vocab_size}")
        print(f"Factorized embedding dimension: {d_e}")
        print(f"Hidden dimension: {d_m}")
        print(f"Input to the embedding layer: sequence of token IDs of length {max_seq_len}")
        print(f"Output: sequence of hidden states of size ({max_seq_len}, {d_m})\n")

    # Embedding from vocab_size to factorized dimension d_e
    op_id_embed_factor = "Embedding_Factorized"
    operations.append({
        'operation_id': op_id_embed_factor,
        'execution_order': operation_counter,
        'layer': 'Embedding Layer',
        'operation_type': 'EmbeddingLookup',
        'details': 'Lookup in the factorized embedding matrix',
        'input_description': 'Sequence of token IDs',
        'input_size': [max_seq_len],  # Sequence length
        'weight_size': [vocab_size, d_e],
        'output_size': [max_seq_len, d_e],
        'dependencies': [],
    })
    if verbose:
        print(f"Operation: {op_id_embed_factor}")
        print(f"  Type: EmbeddingLookup")
        print(f"  Input Size: Token IDs of length {max_seq_len}")
        print(f"  Weight Size: (vocab_size={vocab_size}, d_e={d_e})")
        print(f"  Output Size: (max_seq_len={max_seq_len}, d_e={d_e})")

    operation_counter += 1
    
    # Projection from factorized embedding dimension to hidden dimension
    op_id_embed_project = "Embedding_Projection"
    operations.append({
        'operation_id': op_id_embed_project,
        'execution_order': operation_counter,
        'layer': 'Embedding Layer',
        'operation_type': 'MatMul',
        'details': 'Projection from factorized embedding dimension to hidden dimension',
        'input_description': op_id_embed_factor,
        'input_size': [max_seq_len, d_e],
        'weight_size': [d_e, d_m],
        'output_size': [max_seq_len, d_m],
        'dependencies': [op_id_embed_factor],
    })
    if verbose:
        print(f"\nOperation: {op_id_embed_project}")
        print(f"  Type: MatMul (Embedding Projection)")
        print(f"  MatMul: ({max_seq_len}, {d_e}) x ({d_e}, {d_m}) => ({max_seq_len}, {d_m})")

    operation_counter += 1

    # Token + Position + Segment embeddings addition and layer normalization
    op_id_embed_add_norm = "Embedding_Add_LayerNorm"
    operations.append({
        'operation_id': op_id_embed_add_norm,
        'execution_order': operation_counter,
        'layer': 'Embedding Layer',
        'operation_type': 'Add_LayerNorm',
        'details': 'Add position and segment embeddings, followed by LayerNorm',
        'input_description': [op_id_embed_project],
        'input_size': [max_seq_len, d_m],
        'output_size': [max_seq_len, d_m],
        'dependencies': [op_id_embed_project],
    })
    if verbose:
        print(f"\nOperation: {op_id_embed_add_norm}")
        print(f"  Type: Add + LayerNorm")
        print(f"  Input Size: ({max_seq_len}, {d_m})")
        print(f"  Output Size: ({max_seq_len}, {d_m})")

    operation_counter += 1

    # Phase 2: Encoder Blocks
    if verbose:
        print("\n### Phase 2: Encoder Blocks ###")
        print(f"Number of encoder layers: {Num_encoders}")
        print(f"Parameter sharing: {shared_parameters}")
        print(f"Hidden dimension: {d_m}")
        print(f"Number of attention heads: {Num_heads}")

    previous_output_id = op_id_embed_add_norm
    encoder_dependency = previous_output_id
    
    # If parameters are shared across all layers, we represent the repeated computations the same number of times
    # but note that parameters are shared in the details.
    for layer_idx in range(1, Num_encoders + 1):
    # for layer_idx in range(1, 1 + 1):
        if verbose:
            print(f"\n#### Encoder Layer {layer_idx} ####")
        # Multi-Head Attention (MHA)
        if verbose:
            print(f"- Multi-Head Attention (Layer {layer_idx}):")

        qkv_operation_ids = []
        for head_idx in range(Num_heads):
            # Q calculation
            op_id_q = f"Layer{layer_idx}_Head{head_idx+1}_Q"
            operations.append({
                'operation_id': op_id_q,
                'execution_order': operation_counter,
                'layer': f'Encoder Layer {layer_idx}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'Q',
                    'shared_parameters': shared_parameters
                },
                'input_description': previous_output_id,
                'input_size': [max_seq_len, d_m],
                'weight_size': [d_m, head_dim],
                'output_size': [max_seq_len, head_dim],
                'dependencies': [previous_output_id],
            })
            # K calculation
            op_id_k = f"Layer{layer_idx}_Head{head_idx+1}_K"
            operations.append({
                'operation_id': op_id_k,
                'execution_order': operation_counter,
                'layer': f'Encoder Layer {layer_idx}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'K',
                    'shared_parameters': shared_parameters
                },
                'input_description': previous_output_id,
                'input_size': [max_seq_len, d_m],
                'weight_size': [d_m, head_dim],
                'output_size': [max_seq_len, head_dim],
                'dependencies': [previous_output_id],
            })
            # V calculation
            op_id_v = f"Layer{layer_idx}_Head{head_idx+1}_V"
            operations.append({
                'operation_id': op_id_v,
                'execution_order': operation_counter,
                'layer': f'Encoder Layer {layer_idx}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'V',
                    'shared_parameters': shared_parameters
                },
                'input_description': previous_output_id,
                'input_size': [max_seq_len, d_m],
                'weight_size': [d_m, head_dim],
                'output_size': [max_seq_len, head_dim],
                'dependencies': [previous_output_id],
            })
            qkv_operation_ids.extend([op_id_q, op_id_k, op_id_v])
            if verbose:
                print(f"  - Head {head_idx + 1}: Q matmul: ({max_seq_len}, {d_m}) x ({d_m}, {head_dim}) => ({max_seq_len}, {head_dim})")
                print(f"  - Head {head_idx + 1}: K matmul: ({max_seq_len}, {d_m}) x ({d_m}, {head_dim}) => ({max_seq_len}, {head_dim})")
                print(f"  - Head {head_idx + 1}: V matmul: ({max_seq_len}, {d_m}) x ({d_m}, {head_dim}) => ({max_seq_len}, {head_dim})")

        operation_counter += 1  # Q, K, V computations can be parallel for each head

        # Attention scores and outputs for each head
        attn_output_ids = []
        for head_idx in range(Num_heads):
            op_id_attn_scores = f"Layer{layer_idx}_Head{head_idx+1}_AttentionScores"
            op_id_attn_output = f"Layer{layer_idx}_Head{head_idx+1}_AttentionOutput"
            op_id_q = f"Layer{layer_idx}_Head{head_idx+1}_Q"
            op_id_k = f"Layer{layer_idx}_Head{head_idx+1}_K"
            op_id_v = f"Layer{layer_idx}_Head{head_idx+1}_V"

            # Attention scores: QK^T
            operations.append({
                'operation_id': op_id_attn_scores,
                'execution_order': operation_counter,
                'layer': f'Encoder Layer {layer_idx}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'AttentionScores',
                    'shared_parameters': shared_parameters
                },
                'input_description': [op_id_q, op_id_k],
                'input_size': {
                    'Q': [max_seq_len, head_dim],
                    'K': [max_seq_len, head_dim]
                },
                'weight_size': None,  # Not applicable for QK^T
                'output_size': [max_seq_len, max_seq_len],
                'dependencies': [op_id_q, op_id_k],
            })
            # Attention output: Softmax(QK^T) * V
            operations.append({
                'operation_id': op_id_attn_output,
                'execution_order': operation_counter,
                'layer': f'Encoder Layer {layer_idx}',
                'operation_type': 'MatMul',
                'details': {
                    'head': head_idx + 1,
                    'type': 'AttentionOutput',
                    'shared_parameters': shared_parameters
                },
                'input_description': [op_id_attn_scores, op_id_v],
                'input_size': {
                    'AttentionScores': [max_seq_len, max_seq_len],
                    'V': [max_seq_len, head_dim]
                },
                'weight_size': None,  # Not applicable for the multiplication after softmax
                'output_size': [max_seq_len, head_dim],
                'dependencies': [op_id_attn_scores, op_id_v],
            })
            attn_output_ids.append(op_id_attn_output)
            if verbose:
                print(f"  - Head {head_idx + 1}: Attention scores matmul: ({max_seq_len}, {head_dim}) x ({max_seq_len}, {head_dim}) => ({max_seq_len}, {max_seq_len})")
                print(f"  - Head {head_idx + 1}: Attention output matmul: ({max_seq_len}, {max_seq_len}) x ({max_seq_len}, {head_dim}) => ({max_seq_len}, {head_dim})")

        operation_counter += 1  # Attention computations for each head

        # Concatenate the outputs from all heads
        op_id_concat = f"Layer{layer_idx}_MHA_Concat"
        operations.append({
            'operation_id': op_id_concat,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'Concatenation',
            'details': {
                'type': 'MultiHeadConcatenation',
                'shared_parameters': shared_parameters
            },
            'input_description': attn_output_ids,
            'input_size': [[max_seq_len, head_dim] for _ in range(Num_heads)],
            'output_size': [max_seq_len, d_m],
            'dependencies': attn_output_ids,
        })
        if verbose:
            print("\n  - Multi-head attention outputs are concatenated along the last dimension")
            for idx, attn_op_id in enumerate(attn_output_ids):
                print(f"    * Input from head {idx+1}: ({max_seq_len}, {head_dim})")
            print(f"    * Output Size after concatenation: ({max_seq_len}, {d_m})")

        operation_counter += 1

        # Final linear projection after attention
        op_id_proj = f"Layer{layer_idx}_MHA_Output_Projection"
        operations.append({
            'operation_id': op_id_proj,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'MatMul',
            'details': {
                'type': 'OutputProjection',
                'shared_parameters': shared_parameters
            },
            'input_description': op_id_concat,
            'input_size': [max_seq_len, d_m],
            'weight_size': [d_m, d_m],
            'output_size': [max_seq_len, d_m],
            'dependencies': [op_id_concat],
        })
        if verbose:
            print(f"\n  - Output Projection MatMul:")
            print(f"    * Input Size: ({max_seq_len}, {d_m})")
            print(f"    * Weight Size: ({d_m}, {d_m})")
            print(f"    * Output Size: ({max_seq_len}, {d_m})")

        operation_counter += 1

        # Residual Connection and LayerNorm after MHA
        op_id_res_ln = f"Layer{layer_idx}_MHA_Residual_LayerNorm"
        operations.append({
            'operation_id': op_id_res_ln,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'Residual_LayerNorm',
            'details': 'Residual connection and LayerNorm after MHA',
            'input_description': [previous_output_id, op_id_proj],
            'input_size': [max_seq_len, d_m],
            'output_size': [max_seq_len, d_m],
            'dependencies': [previous_output_id, op_id_proj],
        })
        if verbose:
            print("\n  - Residual Connection and LayerNorm after MHA")
            print(f"    * Input 1: {previous_output_id} ({max_seq_len}, {d_m})")
            print(f"    * Input 2: {op_id_proj} ({max_seq_len}, {d_m})")
            print(f"    * Output Size: ({max_seq_len}, {d_m})")

        operation_counter += 1
        previous_output_id = op_id_res_ln  # Update previous output

        # Feed-Forward Network (FFN)
        if verbose:
            print("\n- Feed-Forward Network (FFN):")

        # First Linear Layer in FFN
        op_id_ffn1 = f"Layer{layer_idx}_FFN_Layer1"
        operations.append({
            'operation_id': op_id_ffn1,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'MatMul',
            'details': 'First Linear Layer in FFN',
            'input_description': op_id_res_ln,
            'input_size': [max_seq_len, d_m],
            'weight_size': [d_m, d_ff],
            'output_size': [max_seq_len, d_ff],
            'dependencies': [op_id_res_ln],
        })
        if verbose:
            print(f"  * First Linear Layer MatMul:")
            print(f"    - Input Size: ({max_seq_len}, {d_m})")
            print(f"    - Weight Size: ({d_m}, {d_ff})")
            print(f"    - Output Size: ({max_seq_len}, {d_ff})")

        operation_counter += 1

        # GeLU activation after first FFN layer
        op_id_gelu1 = f"Layer{layer_idx}_FFN_GeLU1"
        operations.append({
            'operation_id': op_id_gelu1,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'Activation',
            'details': 'GeLU activation after first FFN layer',
            'input_description': op_id_ffn1,
            'input_size': [max_seq_len, d_ff],
            'output_size': [max_seq_len, d_ff],
            'dependencies': [op_id_ffn1],
        })
        if verbose:
            print("  * GeLU activation after first FFN layer:")
            print(f"    - Input/Output Size: ({max_seq_len}, {d_ff})")

        operation_counter += 1

        # Second Linear Layer in FFN
        op_id_ffn2 = f"Layer{layer_idx}_FFN_Layer2"
        operations.append({
            'operation_id': op_id_ffn2,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'MatMul',
            'details': 'Second Linear Layer in FFN',
            'input_description': op_id_gelu1,
            'input_size': [max_seq_len, d_ff],
            'weight_size': [d_ff, d_m],
            'output_size': [max_seq_len, d_m],
            'dependencies': [op_id_gelu1],
        })
        if verbose:
            print(f"  * Second Linear Layer MatMul:")
            print(f"    - Input Size: ({max_seq_len}, {d_ff})")
            print(f"    - Weight Size: ({d_ff}, {d_m})")
            print(f"    - Output Size: ({max_seq_len}, {d_m})")

        operation_counter += 1

        # GeLU activation after second FFN layer (ALBERT uses GeLU here)
        op_id_gelu2 = f"Layer{layer_idx}_FFN_GeLU2"
        operations.append({
            'operation_id': op_id_gelu2,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'Activation',
            'details': 'GeLU activation after second FFN layer',
            'input_description': op_id_ffn2,
            'input_size': [max_seq_len, d_m],
            'output_size': [max_seq_len, d_m],
            'dependencies': [op_id_ffn2],
        })
        if verbose:
            print("  * GeLU activation after second FFN layer:")
            print(f"    - Input/Output Size: ({max_seq_len}, {d_m})")

        operation_counter += 1

        # Residual Connection and LayerNorm after FFN
        op_id_res_ln_ffn = f"Layer{layer_idx}_FFN_Residual_LayerNorm"
        operations.append({
            'operation_id': op_id_res_ln_ffn,
            'execution_order': operation_counter,
            'layer': f'Encoder Layer {layer_idx}',
            'operation_type': 'Residual_LayerNorm',
            'details': 'Residual connection and LayerNorm after FFN',
            'input_description': [op_id_res_ln, op_id_gelu2],
            'input_size': [max_seq_len, d_m],
            'output_size': [max_seq_len, d_m],
            'dependencies': [op_id_res_ln, op_id_gelu2],
        })
        if verbose:
            print("  * Residual Connection and LayerNorm after FFN:")
            print(f"    - Input 1: {op_id_res_ln} ({max_seq_len}, {d_m})")
            print(f"    - Input 2: {op_id_gelu2} ({max_seq_len}, {d_m})")
            print(f"    - Output Size: ({max_seq_len}, {d_m})")

        operation_counter += 1
        previous_output_id = op_id_res_ln_ffn  # Update previous output
        encoder_dependency = previous_output_id

    # Phase 4: Classification Layer
    if verbose:
        print("\n### Phase 4: Classification Layer ###")
        print("- Using [CLS] token output")
        print(f"- Input vector size: (1, {d_m})")
        print(f"- Classification weight matrix size: ({d_m}, {Num_classes})")
        print(f"- MatMul: (1, {d_m}) x ({d_m}, {Num_classes}) => Output: (1, {Num_classes})")

    # Classification operation
    op_id_cls = 'Classification_MatMul'
    operations.append({
        'operation_id': op_id_cls,
        'execution_order': operation_counter,
        'layer': 'Classification Layer',
        'operation_type': 'MatMul',
        'details': 'Final classification layer (SOP tasks in ALBERT uses different tasks but we generalize to classification)',
        'input_description': f"[CLS] token from {previous_output_id}",
        'input_size': [1, d_m],
        'weight_size': [d_m, Num_classes],
        'output_size': [1, Num_classes],
        'dependencies': [previous_output_id],
    })
    if verbose:
        print(f"\nOperation: {op_id_cls}")
        print(f"  Type: MatMul (Final classification layer)")
        print(f"  MatMul: (1, {d_m}) x ({d_m}, {Num_classes}) => (1, {Num_classes})")

    operation_counter += 1

    # Return operations and total FLOPs (FLOPs calculation can be added if needed)
    total_flops = 0  # Placeholder for FLOPs calculation

    if verbose:
        print("\n--- End of ALBERT Workload Operations ---")

    return {
        'operations': operations,
        'total_flops': total_flops
    }

# Example usage
if __name__ == "__main__":
    # results = calculate_tsd_operations(verbose=True)
    results = calculate_albert_operations(verbose=True)
    
    # Optionally, save operations to a file or process further
    # For example, to save to a JSON file:
    # import json
    # with open('tsd_operations.json', 'w') as f:
    #     json.dump(results['operations'], f, indent=4)

    # operations = results['operations']
    # workload = generate_workload_from_operations(operations)

    # print("Generated Workload:")
    # for idx, op in enumerate(workload):
    #     print(f"Operation {idx + 1}: {op}")

    # print(f"Total number of operations: {len(workload)}")
    # print(f"Total number of operations in the original list: {len(operations)}")
    # print("Workload generated successfully!")

    # print also original operations
    operations = results['operations']
    print("Original operations:")
    for idx, op in enumerate(operations):
        print(f"Operation {idx + 1}: {op}")
    print(f"Total number of operations: {len(operations)}")


