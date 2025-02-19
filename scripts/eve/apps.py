import json
import argparse
import matplotlib.pyplot as plt
import numpy as np
import typing

# Define the data type variable
DATA_TYPE_TSD = "int32"
DATA_TYPE_SeizConv2D = "int32"
DATA_TYPE_LCT = "int32"

TSD_NUM_CHANNELS = 20
TSD_NUM_TIMESTEPS = 15
TSD_FFT_SIZE = 512

TSD = [
    # # below ones are applied while STFT preprocess for
    # # N_ch = 20, N_timesteps = 15, FFT_size = 512
    # {
    #     "kernel": "hanning",
    #     "shape": "256",
    #     "repeat": TSD_NUM_CHANNELS*TSD_NUM_TIMESTEPS,
    #     "dataType": "int32"
    # },
    # {
    #     "kernel": "zeropad",
    #     "shape": "256",
    #     "repeat": TSD_NUM_CHANNELS*TSD_NUM_TIMESTEPS,
    #     "dataType": "int32"
    # },
    # {
    #     "kernel": "fft",
    #     "shape": "512",
    #     "repeat": TSD_NUM_CHANNELS*TSD_NUM_TIMESTEPS,
    #     "dataType": "fxp32"
    # },
    # {
    #     "kernel": "compute_log_amp_approx",
    #     "shape": "80",
    #     "repeat": 2*TSD_NUM_CHANNELS*TSD_NUM_TIMESTEPS,
    #     "dataType": "fxp32"
    # },

    # Rest of system
    {
      "kernel": "norm",
      "shape": "120x400",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "matmul",
      "shape": "120x400x16",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "1920",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "norm",
      "shape": "120x16",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "clsConcatenate",
      "shape": "120x16",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      # "kernel": "posEmbedding",
      "kernel": "add",
      "shape": "1936",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "norm",
      "shape": "121x16",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "matmul",
      "shape": "121x16x4",
      "repeat": 48,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "484",
      "repeat": 48,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "transpose",
      "shape": "121x4",
      "repeat": 16,
      "dataType": "int32"
    },
    {
      "kernel": "mm_scale",
      "shape": "121x4",
      "repeat": 16,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "121x4x121",
      "repeat": 16,
      "dataType": DATA_TYPE_TSD
    },
    # {
    #   "kernel": "softmax",
    #   "shape": "121x121",
    #   "repeat": 16,
    #   "dataType": DATA_TYPE_TSD
    # },
    {
      "kernel": "matmul",
      "shape": "121x121x4",
      "repeat": 16,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "mh_transpose",
      "shape": "121x4x4",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "121x16x16",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "1936",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "1936",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "norm",
      "shape": "121x16",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "matmul",
      "shape": "121x16x4",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "484",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "gelu",
      "shape": "121x4",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "121x4x16",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "1936",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": "1936",
      "repeat": 4,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "norm",
      "shape": "121x16",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "matmul",
      "shape": "1x16x16",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    },
    {
      "kernel": "add",
      "shape": f"{16}",
      "repeat": 1,
      "dataType": DATA_TYPE_TSD
    }
]


def create_SeizConv2D():
    wl = []
    
    # {
    #   "kernel": "conv2d",
    #   "shape": "25x1000_f7",
    #   "repeat": 1,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "relu",
    #   "shape": "25x1000",
    #   "repeat": 1,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "norm",
    #   "shape": "25x1000",
    #   "repeat": 1,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "maxpool2d",
    #   "shape": "25x1000_size2_stride1",
    #   "repeat": 1,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "conv2d",
    #   "shape": "12x500_f3",
    #   "repeat": 2,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "relu",
    #   "shape": "12x500",
    #   "repeat": 2,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "norm",
    #   "shape": "12x500",
    #   "repeat": 2,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "maxpool2d",
    #   "shape": "12x500_size2_stride2",
    #   "repeat": 2,
    #   "dataType": DATA_TYPE_SeizConv2D
    # },
    # {
    #   "kernel": "matmul",
    #   "shape": "1x1500x2",
    #   "repeat": 1,
    #   "dataType": DATA_TYPE_SeizConv2D
    # }

    wl.append({
      "kernel": "conv2d",
      "shape": "25x1000_f7",
      "repeat": 1,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "relu",
      "shape": "25x1000",
      "repeat": 1,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "norm",
      "shape": "25x1000",
      "repeat": 1,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "maxpool2d",
      "shape": "25x1000_size2_stride1",
      "repeat": 1,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "conv2d",
      "shape": "12x500_f3",
      "repeat": 2,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "relu",
      "shape": "12x500",
      "repeat": 2,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "norm",
      "shape": "12x500",
      "repeat": 2,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "maxpool2d",
      "shape": "12x500_size2_stride2",
      "repeat": 2,
      "dataType": DATA_TYPE_SeizConv2D
    })
    wl.append({
      "kernel": "matmul",
      "shape": "1x1500x2",
      "repeat": 1,
      "dataType": DATA_TYPE_SeizConv2D
    })  
    
    return wl





def create_multi_head_attention_block(
    seq_len=128,
    embed_dim=512,
    num_heads=8,
    head_dim=64,
    data_type="fxp32"
):
    """
    Creates a block for Multi-Head Self-Attention:
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

    # 1) Q, K, V projections (for each head, or you might do a single big matmul and reshape)
    # We'll do one matmul for Q repeated num_heads times, similarly for K, V (just an example).
    # In reality, you might do one big MatMul -> [batch, seq_len, 3*embed_dim], then split.
    
    # Q
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{embed_dim}x{head_dim}",  # or embed_dim->(embed_dim/num_heads)
        "repeat": num_heads,
        "dataType": data_type
    })
    block.append({
        "kernel": "add",  # bias
        "shape": f"{seq_len*head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })

    # K
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{embed_dim}x{head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })
    block.append({
        "kernel": "add",  # bias
        "shape": f"{seq_len*head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })

    # V
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{embed_dim}x{head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })
    block.append({
        "kernel": "add",
        "shape": f"{seq_len*head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })

    # 2) K transpose or QK^T
    block.append({
        "kernel": "transpose",
        "shape": f"{seq_len}x{head_dim}",
        "repeat": num_heads,
        "dataType": "int32"
    })

    # 3) QK^T matmul
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{head_dim}x{seq_len}",
        "repeat": num_heads,
        "dataType": data_type
    })

    # 4) scale the QK^T by sqrt(d_k) or do mm_scale
    block.append({
        "kernel": "mm_scale",
        # "shape": f"{seq_len}x{seq_len}",
        #TODO: this is obviously wrong! fix it
        "shape": f"{seq_len}x{head_dim}",
        "repeat": num_heads,
        "dataType": "int32"
    })

    # # 5) softmax
    # block.append({
    #     "kernel": "softmax",
    #     "shape": f"{seq_len}x{seq_len}",
    #     "repeat": num_heads,
    #     "dataType": data_type
    # })

    # 6) matmul with V
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{seq_len}x{head_dim}",
        "repeat": num_heads,
        "dataType": data_type
    })

    # 7) 'mh_transpose' or reshape to put heads back together
    block.append({
        "kernel": "mh_transpose",  # e.g. combine heads
        "shape": f"{seq_len}x{num_heads}x{head_dim}",
        "repeat": 1,  # or num_heads, depends on how you do it
        "dataType": "int32"
    })

    # 8) Final projection after attention
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{embed_dim}x{embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })
    block.append({
        "kernel": "add", #bias
        "shape": f"{seq_len*embed_dim}", 
        "repeat": 1,
        "dataType": data_type
    })

    return block


def create_feed_forward_block(
    seq_len=128,
    embed_dim=512,
    ff_dim=2048,
    data_type="fxp32"
):
    """
    Typical feed-forward sub-layer:
      - matmul -> add bias -> activation (e.g., GELU)
      - matmul -> add bias
    """
    block = []

    # 1) FC1
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{embed_dim}x{ff_dim}",
        "repeat": 1,
        "dataType": data_type
    })
    block.append({
        "kernel": "add",  # bias
        "shape": f"{seq_len*ff_dim}",
        "repeat": 1,
        "dataType": data_type
    })
    block.append({
        "kernel": "gelu",
        "shape": f"{seq_len}x{ff_dim}",
        "repeat": 1,
        "dataType": "int32"
    })

    # 2) FC2
    block.append({
        "kernel": "matmul",
        "shape": f"{seq_len}x{ff_dim}x{embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })
    block.append({
        "kernel": "add",  # bias
        "shape": f"{seq_len*embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    return block


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
    block = []

    # --- Pre-Attention Norm
    block.append({
        "kernel": "norm",
        "shape": f"{seq_len}x{embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    # --- Multi-Head Attention
    mha_ops = create_multi_head_attention_block(
        seq_len=seq_len,
        embed_dim=embed_dim,
        num_heads=num_heads,
        head_dim=head_dim,
        data_type=data_type
    )
    block.extend(mha_ops)

    # --- Residual Add (output of MHA + input)
    block.append({
        "kernel": "add",
        "shape": f"{seq_len*embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    # --- Post-Attention Norm
    block.append({
        "kernel": "norm",
        "shape": f"{seq_len}x{embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    # --- Feed Forward
    ff_ops = create_feed_forward_block(
        seq_len=seq_len,
        embed_dim=embed_dim,
        ff_dim=ff_dim,
        data_type=data_type
    )
    block.extend(ff_ops)

    # --- Residual Add
    block.append({
        "kernel": "add",
        "shape": f"{seq_len*embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    # --- (Optional) Post-FF Norm 
    # block.append({
    #     "kernel": "norm",
    #     "shape": f"{seq_len}x{embed_dim}",
    #     "repeat": 1,
    #     "dataType": data_type
    # })

    return block
  
def create_TSD_stft_preprocess(
    N_ch=20, N_timestep=15, size_fft=512
):
    """
    TSD STFT preprocess
    """
    wl = []

    wl.append({
        "kernel": "hanning",
        "shape": "256",
        "repeat": N_ch*N_timestep,
        "dataType": "int32"
    })
    wl.append({
        "kernel": "zeropad",
        "shape": "256",
        "repeat": N_ch*N_timestep,
        "dataType": "int32"
    })
    wl.append({
        "kernel": "fft",
        "shape": "512",
        "repeat": N_ch*N_timestep,
        "dataType": "fxp32"
    })
    wl.append({
        "kernel": "compute_log_amp_approx",
        "shape": "80",
        "repeat": 2*N_ch*N_timestep,
        "dataType": "fxp32"
    })  

    return wl


def create_TSD():
    """
    TSD 
    """
    wl = []

    num_layers=4
    seq_len=121
    embed_dim=16
    num_heads=4
    head_dim= int(embed_dim/num_heads)
    ff_dim=4
    data_type="int32"

    # # below ones are applied while STFT preprocess for
    # wl.extend(create_TSD_stft_preprocess(N_ch=20, N_timestep=15, size_fft=512))

    # Rest of system
    wl.append({
      "kernel": "norm",
      "shape": "120x400",
      "repeat": 1,
      "dataType": data_type
    })
    wl.append({
      "kernel": "matmul",
      "shape": "120x400x16",
      "repeat": 1,
      "dataType": data_type
    })
    wl.append({
      "kernel": "add",
      "shape": "1920",
      "repeat": 1,
      "dataType": data_type
    })
    wl.append({
      "kernel": "norm",
      "shape": "120x16",
      "repeat": 1,
      "dataType": data_type
    })
    wl.append({
      "kernel": "clsConcatenate",
      "shape": "120x16",
      "repeat": 1,
      "dataType": "int32"
    })
    wl.append({
      # "kernel": "posEmbedding",
      "kernel": "add",
      "shape": "1936",
      "repeat": 1,
      "dataType": data_type
    })

    for layer_idx in range(num_layers):
      encoder_block_ops = create_transformer_encoder_block(seq_len, embed_dim, num_heads, head_dim, ff_dim, data_type)
      wl.extend(encoder_block_ops)

    # Example final classification block
    # Typically you'd take the "CLS" token from the last layer or do a pooling.
    # This is just an example matmul -> add -> softmax
    wl.append({
        "kernel": "norm",
        "shape": f"{seq_len}x{embed_dim}",  
        "repeat": 1,
        "dataType": data_type
    })
    wl.append({
        "kernel": "matmul",
        "shape": f"1x{embed_dim}x{embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })
    wl.append({
        "kernel": "add",
        "shape": f"{1*embed_dim}",
        "repeat": 1,
        "dataType": data_type
    })

    return wl

    

def create_LCT():
    """
    LCT
    """
    wl = []

    # Conv2D (3×3, 32) on I(18, 256, 1) --> output: (16, 254, 32)
    wl.append({
      "kernel": "conv2d",
      "shape": "18x256_f3",
      "repeat": 32,
      "dataType": DATA_TYPE_LCT
    })
    # RELU on I(16, 254, 32) --> output: (16, 254, 32)
    wl.append({ 
        "kernel": "relu",
        "shape": "18x256",
        "repeat": 32,
        "dataType": DATA_TYPE_LCT
    })
    # MaxPool2d (3×3, stride 2) on I(16, 254, 32) --> output: (8, 127, 32)
    wl.append({
        "kernel": "maxpool2d",
        "shape": "16x254_size3_stride2",
        "repeat": 32,
        "dataType": DATA_TYPE_LCT
    })
    # Conv2D (3×3, 128) on I(8, 127, 32) --> output: (6, 125, 128)
    wl.append({
        "kernel": "conv2d",
        "shape": "8x127_f3",
        "repeat": 128*32,
        "dataType": DATA_TYPE_LCT
    })
    # RELU on I(6, 125, 128) --> output: (6, 125, 128)
    wl.append({
        "kernel": "relu",
        "shape": "6x125",
        "repeat": 128,
        "dataType": DATA_TYPE_LCT
    })
    # MaxPool2d (3×3, stride 2) on I(6, 125, 128)--> output: (3, 63, 128)
    wl.append({
        "kernel": "maxpool2d",
        "shape": "6x125_size3_stride2",
        "repeat": 128,
        "dataType": DATA_TYPE_LCT
    })
    # (Flatten (3*63,128) and) posEembedding (addition) on I(189, 128) --> output: (189, 128)
    wl.append({
      "kernel": "add",
      "shape": "24192",
      "repeat": 1,
      "dataType": DATA_TYPE_LCT
    })

    num_layers=4
    seq_len=189
    embed_dim=128
    num_heads=2
    head_dim= int(embed_dim/num_heads)
    ff_dim=20
    data_type="int32"

    for layer_idx in range(num_layers):
      encoder_block_ops = create_transformer_encoder_block(seq_len, embed_dim, num_heads, head_dim, ff_dim, data_type)
      wl.extend(encoder_block_ops)

    

    return wl


if __name__ == "__main__":
    # Example usage:
    TSD2 = create_TSD()

    # Print them out or do something else with them
    for idx, op in enumerate(TSD2):
        print(f"{idx+1}. {op}")
    print("=====================================")
    for idx, op in enumerate(TSD):
        print(f"{idx+1}. {op}")
