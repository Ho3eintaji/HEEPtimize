# TSD Network Detailed Computational Analysis Script

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
    Num_classes=2             # Number of output classes
):
    # Initialize a list to store operations
    operations = []

    print("=== TSD Network Computational Analysis ===\n")
    
    # Phase 1: Preprocessing
    print("### Phase 1: Preprocessing ###")
    N_samples = T * F_sampling
    N_STFTs = C * S
    len_seq = C * S * Num_bands
    print(f"- Total samples per channel: {N_samples}")
    print(f"- Total STFTs to perform: {N_STFTs}")
    print(f"- Number of frequency bands per STFT: {Num_bands}")
    print(f"- Total STFT outputs (len_seq): {len_seq}")
    print("\nDetails of STFT processing:")
    print(f"* Each STFT is performed on {N_samples / S} samples per channel segment.")
    print(f"* STFT outputs a vector of size {d_fft} per frequency band.")
    print(f"* Total STFT operations (not matmuls): {N_STFTs * Num_bands}")
    print(f"* STFT output matrix size: ({len_seq} x {d_fft})\n")
    
    # Store STFT operations
    operations.append({
        'layer': 'Preprocessing',
        'operation': 'STFT',
        'details': 'FFT operations per frequency band',
        'input_size': f"{int(N_samples / S)} samples per segment",
        'output_size': f"{d_fft} per frequency band",
        'total_operations': N_STFTs * Num_bands
    })
    
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
        
        # Feed-Forward Network (FFN)
        print("\n- Feed-Forward Network (FFN):")
        print(f"  * First Linear Layer MatMul: ({len_seq} x {d_m}) x ({d_m} x {d_ff}) => ({len_seq} x {d_ff})")
        print(f"  * GeLU activation applied to ({len_seq} x {d_ff})")
        print(f"  * Second Linear Layer MatMul: ({len_seq} x {d_ff}) x ({d_ff} x {d_m}) => ({len_seq} x {d_m})")
        
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
            'details': 'GeLU activation',
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
        
        # Note: Residual connections and layer normalization are omitted for brevity

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
    
    # Summary of Total FLOPs (calculated but not detailed per operation)
    total_flops = 0
    embedding_flops = 2 * (len_seq - 1) * d_fft * d_m
    total_flops += embedding_flops
    
    encoder_flops = 0
    for encoder_idx in range(Num_encoders):
        # MHA FLOPs per encoder
        mha_flops = Num_heads * (
            2 * len_seq * d_m * d_q * 3 +          # Q, K, V
            2 * len_seq * d_q * len_seq +          # Attention scores
            2 * len_seq * len_seq * d_v            # Attention output
        ) + 2 * len_seq * (d_v * Num_heads) * d_m  # Final projection
        
        # FFN FLOPs per encoder
        ffn_flops = 2 * len_seq * d_m * d_ff + 2 * len_seq * d_ff * d_m
        encoder_flops += mha_flops + ffn_flops
    
    total_flops += encoder_flops
    classification_flops = 2 * d_m * Num_classes
    total_flops += classification_flops
    
    # Summary
    print("=== Summary ===")
    print(f"Total FLOPs for Embedding Layer: {embedding_flops}")
    print(f"Total FLOPs for Encoder Blocks: {encoder_flops}")
    print(f"Total FLOPs for Classification Layer: {classification_flops}")
    print(f"Overall Total FLOPs for the network: {total_flops}\n")
    
    # Return operations and total FLOPs
    return {
        'operations': operations,
        'total_flops': total_flops
    }

# Example usage
if __name__ == "__main__":
    results = calculate_tsd_operations()
    
    # Optionally, save operations to a file or process further
    # For example, to save to a JSON file:
    # import json
    # with open('tsd_operations.json', 'w') as f:
    #     json.dump(results['operations'], f, indent=4)
