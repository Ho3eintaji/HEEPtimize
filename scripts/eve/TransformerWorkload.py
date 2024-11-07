# TSD Network Detailed Computational Analysis Script

import math
import json

def calculate_tsd_operations(
    T=12,                     # Duration of EEG signal in seconds
    F_sampling=256,           # Sampling frequency in Hz
    C=10,                     # Number of EEG channels
    S=3,                      # Number of splits
    Num_bands=4,              # Number of frequency bands in STFT
    d_fft=128,                # Dimensionality of STFT output per band
    d_m=16,                   # Embedding dimension
    Num_encoders=4,           # Number of encoder blocks
    Num_heads=4,              # Number of attention heads
    d_q=4,                    # Dimension of Q vectors
    d_k=4,                    # Dimension of K vectors
    d_v=4,                    # Dimension of V vectors
    d_ff=4,                   # Dimension in FFN
    Num_classes=2,            # Number of output classes
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
        print(f"- Total samples per channel: {N_samples}")
        print(f"- Number of channels (C): {C}")
        print(f"- Number of splits (S): {S}")
        print(f"- Samples per segment: {samples_per_segment}")
        print(f"- Number of STFTs to perform: {N_STFTs}")
        print(f"- Number of frequency bands per STFT: {Num_bands}")
        print(f"- Total STFT outputs (len_seq): {len_seq}")
        print(f"- STFT output per frequency band: vector of size {d_fft}")
        print(f"- STFT output matrix size: ({len_seq}, {d_fft})\n")
        print("- STFT Operations:")
    
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
        print(f"- Input to embedding layer: matrix of size ({len_seq}, {d_fft})")
        print(f"- Embedding weight matrix size: ({d_fft}, {d_m})")
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
    previous_output_id = 'Embedding_MatMul'  # Initial dependency
    for encoder_idx in range(Num_encoders):
        if verbose:
            print(f"\n#### Encoder Block {encoder_idx + 1} ####")
        encoder_operations = []

        # Multi-Head Attention (MHA)
        if verbose:
            print("- Multi-Head Attention (MHA):")
            print(f"  * Input matrix size: ({len_seq}, {d_m})")
            print(f"  * Number of heads: {Num_heads}")
            print("  * Q, K, V Calculations for each head:")
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
                print(f"    - Head {head_idx + 1}: Q, K, V MatMuls")
        

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
                print(f"    - Head {head_idx + 1}: Attention Score (QK^T) MatMul")
        
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
                print(f"    - Head {head_idx + 1}: Attention Score Softmax")
                
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
                print(f"    - Head {head_idx + 1}: Attention Output (V) MatMul")

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
            print("  * Output Projection MatMul")
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
            print("  * Residual Connection and LayerNorm after MHA")
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
            print("  * First Linear Layer MatMul")
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
            print("  * GeLU activation after first FFN layer")
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
            print("  * Second Linear Layer MatMul")
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
            print("  * GeLU activation after second FFN layer")
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
            print("  * Residual Connection and LayerNorm after FFN")
        operation_counter += 1
        previous_output_id = op_id_res_ln_ffn  # Update previous output

    # Phase 4: Classification Layer
    if verbose:
        print("\n### Phase 4: Classification Layer ###")
        print("- Using [CLS] token output")
        print(f"- Input vector size: (1, {d_m})")
        print(f"- Classification weight matrix size: ({d_m}, {Num_classes})")
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
        print("\nNote: Softmax activation applied after classification (typically negligible in computational cost)\n")
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


# Example usage
if __name__ == "__main__":
    results = calculate_tsd_operations(verbose=False)
    
    # Optionally, save operations to a file or process further
    # For example, to save to a JSON file:
    # import json
    # with open('tsd_operations.json', 'w') as f:
    #     json.dump(results['operations'], f, indent=4)

    operations = results['operations']
    workload = generate_workload_from_operations(operations)

    # print("Generated Workload:")
    # for idx, op in enumerate(workload):
    #     print(f"Operation {idx + 1}: {op}")

    # print(f"Total number of operations: {len(workload)}")
    # print(f"Total number of operations in the original list: {len(operations)}")
    # print("Workload generated successfully!")

    # # print also original operations
    # print("Original operations:")
    # for idx, op in enumerate(operations):
    #     print(f"Operation {idx + 1}: {op}")
    # print(f"Total number of operations: {len(operations)}")


