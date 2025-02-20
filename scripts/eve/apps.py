import json
import argparse
import matplotlib.pyplot as plt
import numpy as np
import typing

def create_workload_layer(kernel, shape, repeat=1, data_type="int32"):
    """Helper function to create a workload layer dictionary."""
    return {
        "kernel": kernel,
        "shape": shape,
        "repeat": repeat,
        "dataType": data_type
    }

def create_SeizConv2D(data_type="int32"):
    """Generate SeizConv2D workload."""
    layers = [
        ("conv2d", "25x1000_f7", 1),
        ("relu", "25x1000", 1),
        ("norm", "25x1000", 1),
        ("maxpool2d", "25x1000_size2_stride1", 1),
        ("conv2d", "12x500_f3", 2),
        ("relu", "12x500", 2),
        ("norm", "12x500", 2),
        ("maxpool2d", "12x500_size2_stride2", 2),
        ("matmul", "1x1500x2", 1),
    ]
    return [create_workload_layer(k, s, r, data_type) for k, s, r in layers]

def create_multi_head_attention_block(seq_len=128, embed_dim=512, num_heads=8, head_dim=64, data_type="fxp32"):
    """
    Creates a block for Multi-Head Self-Attention.
      Q, K, V = Matmul + (optionally add bias)
      K^T or separate matmul for QK^T
      mm_scale
      softmax
      matmul with V
      'mh_transpose' or 'reshape' to combine heads
      final projection matmul
    We'll also add the 'add' for bias if needed.

    This block by itself doesn't do the 'add + norm' for residual connections.
    That is typically done in the parent 'TransformerEncoderBlock'.
    """
    block = []

    # Q, K, V Matmul + Bias
    for qkv in ["Q", "K", "V"]:
        block.extend([
            create_workload_layer("matmul", f"{seq_len}x{embed_dim}x{head_dim}", num_heads, data_type),
            create_workload_layer("add", f"{seq_len*head_dim}", num_heads, data_type),
        ])

    # Transpose K
    block.append(create_workload_layer("transpose", f"{seq_len}x{head_dim}", num_heads, "int32"))

    # QK^T Matmul
    block.append(create_workload_layer("matmul", f"{seq_len}x{head_dim}x{seq_len}", num_heads, data_type))

    # Scaling
    block.append(create_workload_layer("mm_scale", f"{seq_len}x{seq_len}", num_heads, "int32"))

    # Matmul with V
    block.append(create_workload_layer("matmul", f"{seq_len}x{seq_len}x{head_dim}", num_heads, data_type))

    # Merge heads
    block.append(create_workload_layer("mh_transpose", f"{seq_len}x{num_heads}x{head_dim}", 1, "int32"))

    # Final projection
    block.extend([
        create_workload_layer("matmul", f"{seq_len}x{embed_dim}x{embed_dim}", 1, data_type),
        create_workload_layer("add", f"{seq_len*embed_dim}", 1, data_type),
    ])

    return block

def create_feed_forward_block(seq_len=128, embed_dim=512, ff_dim=2048, data_type="fxp32"):
    """
    Creates a Feed Forward block for the Transformer Encoder.
    Typical feed-forward sub-layer:
      - matmul -> add bias -> activation (e.g., GELU)
      - matmul -> add bias
    """
    return [
        create_workload_layer("matmul", f"{seq_len}x{embed_dim}x{ff_dim}", 1, data_type),
        create_workload_layer("add", f"{seq_len*ff_dim}", 1, data_type),
        create_workload_layer("gelu", f"{seq_len}x{ff_dim}", 1, "int32"),
        create_workload_layer("matmul", f"{seq_len}x{ff_dim}x{embed_dim}", 1, data_type),
        create_workload_layer("add", f"{seq_len*embed_dim}", 1, data_type),
    ]

def create_transformer_encoder_block(
    seq_len=128,
    embed_dim=512,
    num_heads=8,
    head_dim=64,
    ff_dim=2048,
    data_type="fxp32"
):
    """
    A single Transformer Encoder block:
      1) Norm
      2) MHA
      3) Residual Add
      4) Norm
      5) FeedForward
      6) Residual Add
      (Optionally final Norm, depends on architecture)

    """
    # Norm before Multi-Head Attention
    block = [create_workload_layer("norm", f"{seq_len}x{embed_dim}", 1, data_type)]
    # Multi-Head Attention Block
    block.extend(create_multi_head_attention_block(seq_len, embed_dim, num_heads, head_dim, data_type))
    # Residual Add
    block.append(create_workload_layer("add", f"{seq_len*embed_dim}", 1, data_type))
    # Norm before Feed Forward
    block.append(create_workload_layer("norm", f"{seq_len}x{embed_dim}", 1, data_type))
    # Feed Forward Block
    block.extend(create_feed_forward_block(seq_len, embed_dim, ff_dim, data_type))
    # Residual Add
    block.append(create_workload_layer("add", f"{seq_len*embed_dim}", 1, data_type))
    return block
  
def create_TSD(
  N_ch=20,
  N_timestep=15,
  size_fft=512,
  num_layers=4,
  seq_len=121,
  embed_dim=16,
  num_heads=4,
  head_dim=4,
  ff_dim=4,
  data_type="int32"
):
    """
    TSD 
    """
    wl = []

    # STFT Preprocess
    # wl.extend([
    #     create_workload_layer("hanning", "256", N_ch*N_timestep, "int32"),
    #     create_workload_layer("zeropad", "256", N_ch*N_timestep, "int32"),
    #     create_workload_layer("fft", "512", N_ch*N_timestep, "fxp32"),
    #     create_workload_layer("compute_log_amp_approx", "80", 2*N_ch*N_timestep, "fxp32"),
    # ])

    # After STFT Preprocess and before the Transformer Encoder
    wl.extend([
        create_workload_layer("norm", "120x400", 1, "int32"),
        create_workload_layer("matmul", "120x400x16", 1, "int32"),
        create_workload_layer("add", "1920", 1, "int32"),
        create_workload_layer("norm", "120x16", 1, "int32"),
        create_workload_layer("clsConcatenate", "120x16", 1, "int32"),
        create_workload_layer("add", "1936", 1, "int32"),  # Positional embedding
    ])

    # Add Transformer Encoder blocks
    for _ in range(num_layers):
        wl.extend(create_transformer_encoder_block(seq_len, embed_dim, num_heads, head_dim, ff_dim, data_type))


    # Final classification block
    wl.extend([
        create_workload_layer("norm", f"{seq_len}x{embed_dim}", 1, data_type),
        create_workload_layer("matmul", f"1x{embed_dim}x{embed_dim}", 1, data_type),
        create_workload_layer("add", f"{1*embed_dim}", 1, data_type),
    ])

    return wl
   
def create_LCT(
    num_layers=4,
    seq_len=189,
    embed_dim=128,
    num_heads=2,
    head_dim=64,
    ff_dim=20,
    data_type="int32",
    n_classes=5
):
    """Creates LCT workload."""
    wl = []

    # Initial Conv2D + Activation + Pooling layers
      # Conv2D (3×3, 32) on I(18, 256, 1) --> output: (16, 254, 32)
      # RELU on I(16, 254, 32) --> output: (16, 254, 32
      # MaxPool2d (3×3, stride 2) on I(16, 254, 32) --> output: (8, 127, 32)
      # Conv2D (3×3, 128) on I(8, 127, 32) --> output: (6, 125, 128)
      # RELU on I(6, 125, 128) --> output: (6, 125, 128)
      # MaxPool2d (3×3, stride 2) on I(6, 125, 128)--> output: (3, 63, 128)
    conv_layers = [
        
        ("conv2d", "18x256_f3", 32),
        ("relu", "18x256", 32),
        ("maxpool2d", "16x254_size3_stride2", 32),
        ("conv2d", "8x127_f3", 128 * 32),
        ("relu", "6x125", 128),
        ("maxpool2d", "6x125_size3_stride2", 128),
    ]
    wl.extend([create_workload_layer(k, s, r, data_type) for k, s, r in conv_layers])

    # Positional Embedding (Flattening)
    wl.append(create_workload_layer("add", "24192", 1, data_type))

    # Transformer Encoder Blocks
    for _ in range(num_layers):
        wl.extend(create_transformer_encoder_block(seq_len, embed_dim, num_heads, head_dim, ff_dim, data_type))

    # Sequential Pooling
    seq_pooling = [
        ("matmul", f"{seq_len}x{embed_dim}x1", 1),
        ("add", f"{seq_len}x1", 1),
        ("softmax", f"{seq_len}x1", 1),
        ("transpose", f"1x{seq_len}", 1, "int32"),
        ("matmul", f"1x{seq_len}x{embed_dim}", 1),
    ]
    wl.extend([create_workload_layer(k, s, r, data_type if d is None else d) for k, s, r, *d in seq_pooling])

    # Classification Layer
    wl.append(create_workload_layer("matmul", f"1x{embed_dim}x{n_classes}", 1, data_type))

    return wl


if __name__ == "__main__":

    TSD = create_TSD(N_ch=20, N_timestep=15, size_fft=512, num_layers=4, seq_len=121, embed_dim=16, num_heads=4, head_dim=16//4, ff_dim=4, data_type="int32")

    # Print them out or do something else with them
    for idx, op in enumerate(TSD):
        print(f"{idx+1}. {op}")
