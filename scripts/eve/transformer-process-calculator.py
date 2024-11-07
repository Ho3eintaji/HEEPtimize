# TSD Network Detailed Computational Analysis Script

import math
import json

def parse_size_string(size_str, variables):
    """
    Parses a size string like '(len_seq x d_m)' into a tuple of integers.

    Args:
        size_str (str): The size string to parse.
        variables (dict): A dictionary mapping variable names to their integer values.

    Returns:
        tuple: A tuple of integers representing the dimensions.
    """
    size_str = size_str.strip('()')
    dims = size_str.split('x')
    dims = [dim.strip() for dim in dims]
    parsed_dims = []
    for dim in dims:
        try:
            value = eval(dim, {}, variables)
            parsed_dims.append(value)
        except:
            # Handle cases where the dimension is a number
            parsed_dims.append(int(dim))
    return tuple(parsed_dims)

def calculate_tsd_operations(
    T=12,                     # Duration of EEG signal in seconds
    F_sampling=256,           # Sampling frequency in Hz
    C=20,                     # Number of EEG channels
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
    Num_classes=2             # Number of output classes
):
    # Initialize a list to store operations
    operations = []

    print("=== TSD Network Computational Analysis ===\n")
    
    # Phase 1: Preprocessing (STFT)
    print("### Phase 1: Preprocessing (STFT) ###")
    N_samples = T * F_sampling
    N_STFTs = C * S
    len_seq = C * S * Num_bands
    samples_per_segment = int(N_samples / S)
    print(f"- Total samples per channel: {N_samples}")
    print(f"- Number of channels (C): {C}")
    print(f"- Number of splits (S): {S}")
    print(f"- Samples per segment: {samples_per_segment}")
    print(f"- Number of STFTs to perform: {N_STFTs}")
    print(f"- Number of frequency bands per STFT: {Num_bands}")
    print(f"- Total STFT outputs (len_seq): {len_seq}")
    print(f"- STFT output per frequency band: vector of size {d_fft}")
    print(f"- STFT output matrix size: ({len_seq} x {d_fft})\n")
    
    # Store STFT operations
    # print("- STFT Operations:")
    for channel_idx in range(1, C + 1):
        for split_idx in range(1, S + 1):
            for band_idx in range(1, Num_bands + 1):
                operations.append({
                    'layer': 'Preprocessing',
                    'operation': 'STFT',
                    'details': f'STFT on Channel {channel_idx}, Segment {split_idx}, Band {band_idx}',
                    'input_size': f"({samples_per_segment} samples)",
                    'output_size': f"({d_fft} coefficients)",
                    'repeats': 1
                })
    # print(f"- Total number of STFT operations: {len_seq}\n")

    # Phase 2: Embedding Layer
    print("### Phase 2: Embedding Layer ###")
    print(f"- Input to embedding layer: matrix of size ({len_seq} x {d_fft})")
    print(f"- Embedding weight matrix size: ({d_fft} x {d_m})")
    print(f"- MatMul: ({len_seq} x {d_fft}) x ({d_fft} x {d_m}) => Output: ({len_seq} x {d_m})\n")
    
    # Store Embedding MatMul operation
    operations.append({
        'layer': 'Embedding Layer',
        'operation': 'MatMul',
        'details': 'Embedding projection',
        'input_size': f"({len_seq} x {d_fft})",
        'weight_size': f"({d_fft} x {d_m})",
        'output_size': f"({len_seq} x {d_m})"
    })
    
    len_seq += 1  # Adding [CLS] token
    print(f"- Sequence length after adding [CLS] token: {len_seq}\n")
    
    # Phase 3: Transformer Encoder Blocks
    print("### Phase 3: Transformer Encoder Blocks ###")
    for encoder_idx in range(Num_encoders):
        print(f"\n#### Encoder Block {encoder_idx + 1} ####")
        
        # Multi-Head Attention (MHA)
        print("- Multi-Head Attention (MHA):")
        print(f"  * Input matrix size: ({len_seq} x {d_m})")
        
        # For each head (operations are similar)
        print(f"  * Number of heads: {Num_heads}")
        print("  * Per-head operations:")
        
        # Q, K, V calculations (same for each head)
        print(f"    - Q MatMul: ({len_seq} x {d_m}) x ({d_m} x {d_q}) => ({len_seq} x {d_q})")
        print(f"    - K MatMul: ({len_seq} x {d_m}) x ({d_m} x {d_k}) => ({len_seq} x {d_k})")
        print(f"    - V MatMul: ({len_seq} x {d_m}) x ({d_m} x {d_v}) => ({len_seq} x {d_v})")
        
        # Store Q, K, V operations (once, since they are repeated per head)
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'MatMul',
            'details': 'Q, K, V calculations per head',
            'input_size': f"({len_seq} x {d_m})",
            'weight_size': f"({d_m} x {d_q}), ({d_m} x {d_k}), ({d_m} x {d_v})",
            'output_size': f"({len_seq} x {d_q}), ({len_seq} x {d_k}), ({len_seq} x {d_v})",
            'repeats': Num_heads * 3  # Q, K, V for each head
        })
        
        # Attention scores and outputs
        print(f"    - Attention scores MatMul: ({len_seq} x {d_q}) x ({len_seq} x {d_k})^T => ({len_seq} x {len_seq})")
        print(f"    - Attention output MatMul: ({len_seq} x {len_seq}) x ({len_seq} x {d_v}) => ({len_seq} x {d_v})")
        
        # Store Attention operations
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'MatMul',
            'details': 'Attention scores and outputs per head',
            'input_size': f"({len_seq} x {d_q}), ({len_seq} x {d_k})^T",
            'output_size': f"({len_seq} x {len_seq}), ({len_seq} x {d_v})",
            'repeats': Num_heads * 2  # Scores and output per head
        })
        
        # Final linear projection
        concat_dim = d_v * Num_heads
        print(f"  * Final projection MatMul: ({len_seq} x {concat_dim}) x ({concat_dim} x {d_m}) => ({len_seq} x {d_m})")
        
        # Store Final projection operation
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'MatMul',
            'details': 'Final linear projection after concatenation',
            'input_size': f"({len_seq} x {concat_dim})",
            'weight_size': f"({concat_dim} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        
        # Residual Connection and Layer Normalization after MHA
        print("  * Residual connection and LayerNorm after MHA")
        # Store Residual and LayerNorm
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'Element-wise Add',
            'details': 'Residual connection after MHA',
            'input_size': f"({len_seq} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'LayerNorm',
            'details': 'Layer normalization after MHA',
            'input_size': f"({len_seq} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        
        # Feed-Forward Network (FFN)
        print("\n- Feed-Forward Network (FFN):")
        print(f"  * First Linear Layer MatMul: ({len_seq} x {d_m}) x ({d_m} x {d_ff}) => ({len_seq} x {d_ff})")
        print(f"  * GeLU activation applied to ({len_seq} x {d_ff})")
        print(f"  * Second Linear Layer MatMul: ({len_seq} x {d_ff}) x ({d_ff} x {d_m}) => ({len_seq} x {d_m})")
        print(f"  * GeLU activation applied to ({len_seq} x {d_m})")
        
        # Store FFN operations
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'MatMul',
            'details': 'First Linear Layer in FFN',
            'input_size': f"({len_seq} x {d_m})",
            'weight_size': f"({d_m} x {d_ff})",
            'output_size': f"({len_seq} x {d_ff})"
        })
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'Activation',
            'details': 'GeLU activation after first FFN layer',
            'input_size': f"({len_seq} x {d_ff})",
            'output_size': f"({len_seq} x {d_ff})"
        })
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'MatMul',
            'details': 'Second Linear Layer in FFN',
            'input_size': f"({len_seq} x {d_ff})",
            'weight_size': f"({d_ff} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'Activation',
            'details': 'GeLU activation after second FFN layer',
            'input_size': f"({len_seq} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        
        # Residual Connection and Layer Normalization after FFN
        print("  * Residual connection and LayerNorm after FFN")
        # Store Residual and LayerNorm
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'Element-wise Add',
            'details': 'Residual connection after FFN',
            'input_size': f"({len_seq} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
        operations.append({
            'layer': f'Encoder Block {encoder_idx + 1}',
            'operation': 'LayerNorm',
            'details': 'Layer normalization after FFN',
            'input_size': f"({len_seq} x {d_m})",
            'output_size': f"({len_seq} x {d_m})"
        })
    
    # Phase 4: Classification Layer
    print("\n### Phase 4: Classification Layer ###")
    print("- Using [CLS] token output")
    print(f"- Input vector size: (1 x {d_m})")
    print(f"- Classification weight matrix size: ({d_m} x {Num_classes})")
    print(f"- MatMul: (1 x {d_m}) x ({d_m} x {Num_classes}) => Output: (1 x {Num_classes})")
    
    # Store Classification operation
    operations.append({
        'layer': 'Classification Layer',
        'operation': 'MatMul',
        'details': 'Final classification layer',
        'input_size': f"(1 x {d_m})",
        'weight_size': f"({d_m} x {Num_classes})",
        'output_size': f"(1 x {Num_classes})"
    })
    
    print("\nNote: Softmax activation applied after classification (typically negligible in computational cost)\n")
    
    # Summary of Total FLOPs
    print("=== Summary ===")
    total_flops = 0

    # STFT FLOPs
    N_fft = samples_per_segment
    flops_per_stft = N_fft * math.log2(N_fft)
    total_stft_flops = len_seq * flops_per_stft
    total_flops += total_stft_flops
    print(f"Estimated FLOPs for all STFT operations: {int(total_stft_flops)}")

    # Embedding Layer FLOPs
    embedding_flops = 2 * (len_seq - 1) * d_fft * d_m
    total_flops += embedding_flops
    print(f"Total FLOPs for Embedding Layer: {embedding_flops}")

    # Encoder Blocks FLOPs
    encoder_flops = 0
    for encoder_idx in range(Num_encoders):
        # MHA FLOPs per encoder
        mha_flops = Num_heads * (
            2 * len_seq * d_m * d_q * 3 +          # Q, K, V
            2 * len_seq * d_q * len_seq +          # Attention scores
            2 * len_seq * len_seq * d_v            # Attention output
        ) + 2 * len_seq * (d_v * Num_heads) * d_m  # Final projection

        # Residual connections and LayerNorm after MHA
        residual_mha_flops = 2 * len_seq * d_m  # Addition
        layernorm_mha_flops = 2 * len_seq * d_m  # Assuming simple scaling and shifting

        # FFN FLOPs per encoder
        ffn_flops = (2 * len_seq * d_m * d_ff +     # First linear layer
                     2 * len_seq * d_ff +           # GeLU activation after first FFN layer
                     2 * len_seq * d_ff * d_m +     # Second linear layer
                     2 * len_seq * d_m)             # GeLU activation after second FFN layer

        # Residual connections and LayerNorm after FFN
        residual_ffn_flops = 2 * len_seq * d_m  # Addition
        layernorm_ffn_flops = 2 * len_seq * d_m  # LayerNorm

        # Sum up FLOPs for this encoder
        encoder_block_flops = (mha_flops + residual_mha_flops + layernorm_mha_flops +
                               ffn_flops + residual_ffn_flops + layernorm_ffn_flops)
        encoder_flops += encoder_block_flops
        print(f"Total FLOPs for Encoder Block {encoder_idx + 1}: {encoder_block_flops}")

    total_flops += encoder_flops

    # Classification Layer FLOPs
    classification_flops = 2 * d_m * Num_classes
    total_flops += classification_flops
    print(f"Total FLOPs for Classification Layer: {classification_flops}")

    print(f"Overall Total FLOPs for the network: {total_flops}\n")
    
    # Return operations and total FLOPs
    return {
        'operations': operations,
        'total_flops': total_flops,
        'variables': {
            'len_seq': len_seq,
            'd_m': d_m,
            'd_fft': d_fft,
            'd_q': d_q,
            'd_k': d_k,
            'd_v': d_v,
            'd_ff': d_ff,
            'Num_classes': Num_classes,
            # Add other variables as needed
        }
    }

def generate_workload_from_operations(operations, variables):
    """
    Generates a workload suitable for the emulator from the list of operations.

    Args:
        operations (list): List of operation dictionaries.
        variables (dict): Dictionary of variables used in operations.

    Returns:
        list: Workload for the emulator.
    """
    workload = []
    for op in operations:
        if op['operation'] == 'MatMul':
            # Parse input and weight sizes
            input_size = op.get('input_size', '')
            weight_size = op.get('weight_size', '')
            repeats = op.get('repeats', 1)

            # For MatMul, we need the dimensions of the matrices
            # Assuming input_size is (M x K) and weight_size is (K x N)
            # The output will be (M x N)
            if weight_size:
                # Use weight_size to get K and N
                weight_dims = parse_size_string(weight_size, variables)
                if len(weight_dims) == 2:
                    K, N = weight_dims
                else:
                    continue  # Skip if dimensions are not as expected
            else:
                continue  # Skip if weight_size is not provided

            if input_size:
                input_dims = parse_size_string(input_size, variables)
                if len(input_dims) == 2:
                    M, K_input = input_dims
                    if K_input != K:
                        # Dimensions do not match, skip this operation
                        continue
                else:
                    continue  # Skip if dimensions are not as expected
            else:
                continue  # Skip if input_size is not provided

            # Create the operation dictionary
            for _ in range(repeats):
                workload.append({
                    'row_a': M,
                    'col_a': K,
                    'col_b': N
                })
    return workload

# Example usage
if __name__ == "__main__":
    results = calculate_tsd_operations()
    
    # Optionally, save operations to a file or process further
    # For example, to save to a JSON file:
    # import json
    # with open('tsd_operations.json', 'w') as f:
    #     json.dump(results['operations'], f, indent=4)

    operations = results['operations']
    variables = results['variables']
    workload = generate_workload_from_operations(operations, variables)

    # print("\n=== Workload for Emulator ===")
    # for idx, task in enumerate(workload):
    #     print(f"Task {idx + 1}: {task}")

