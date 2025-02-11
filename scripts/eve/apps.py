import json
import argparse
import matplotlib.pyplot as plt
import numpy as np


TSD = [
    {
      "kernel": "norm",
      "shape": "120x400",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "120x400x16",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "120x16",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      "kernel": "norm",
      "shape": "120x16",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      "kernel": "clsConcatenate",
      "shape": "120x16",
      "repeat": 1,
      "dataType": "int32"
    },
    # {
    #   "kernel": "posEmbedding",
    #   "shape": "121x16",
    #   "repeat": 1,
    #   "dataType": "int32"
    # },
    {
      "kernel": "norm",
      "shape": "120x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "121x16x4",
      "repeat": 48,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x4",
      "repeat": 48,
      "dataType": "int32"
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
      "dataType": "int32"
    },
    # {
    #   "kernel": "softmax",
    #   "shape": "121x121",
    #   "repeat": 16,
    #   "dataType": "int32"
    # },
    {
      "kernel": "matmul",
      "shape": "121x121x4",
      "repeat": 16,
      "dataType": "int32"
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
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "norm",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "121x16x4",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x4",
      "repeat": 4,
      "dataType": "int32"
    },
    # {
    #   "kernel": "gelu",
    #   "shape": "121x4",
    #   "repeat": 4,
    #   "dataType": "int32"
    # },
    {
      "kernel": "matmul",
      "shape": "121x4x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "norm",
      "shape": "121x16",
      "repeat": 4,
      "dataType": "int32"
    },
    {
      "kernel": "matmul",
      "shape": "1x16x16",
      "repeat": 1,
      "dataType": "int32"
    },
    {
      "kernel": "add",
      "shape": "1x16",
      "repeat": 1,
      "dataType": "int32"
    }
]

EpilepticSeizureConv2DArchitecture = [
    {
      "kernel": "conv2d",
      "shape": "25x1000_f7",
      "repeat": 1,
      "dataType": "fxp32"
    },
    {
      "kernel": "relu",
      "shape": "25x1000",
      "repeat": 1,
      "dataType": "fxp32"
    },
    {
      "kernel": "norm",
      "shape": "25x1000",
      "repeat": 1,
      "dataType": "fxp32"
    },
    {
      "kernel": "maxpool",
      "shape": "25x1000_size2_stride1",
      "repeat": 1,
      "dataType": "fxp32"
    },
    {
      "kernel": "conv2d",
      "shape": "12x500_f3",
      "repeat": 2,
      "dataType": "fxp32"
    },
    {
      "kernel": "relu",
      "shape": "12x500",
      "repeat": 2,
      "dataType": "fxp32"
    },
    {
      "kernel": "norm",
      "shape": "12x500",
      "repeat": 2,
      "dataType": "fxp32"
    },
    {
      "kernel": "maxpool",
      "shape": "12x500_size2_stride2",
      "repeat": 2,
      "dataType": "fxp32"
    },
    {
      "kernel": "gap",
      "shape": "6x250",
      "repeat": 1,
      "dataType": "fxp32"
    },
    {
      "kernel": "matmul",
      "shape": "1x1500x2",
      "repeat": 1,
      "dataType": "fxp32"
    }
]

LCT_conv = [
    # Conv2D (3×3, 32) on I(18, 256, 1) --> output: (16, 254, 32)
    {
      "kernel": "conv2d",
      "shape": "18x256_f3",
      "repeat": 32,
      "dataType": "fxp32"
    },
    # RELU on I(16, 254, 32) --> output: (16, 254, 32)
    {
        "kernel": "relu",
        "shape": "18x256",
        "repeat": 32,
        "dataType": "fxp32"
    },
    # MaxPool2d (3×3, stride 2) on I(16, 254, 32) --> output: (8, 127, 32)
    {
        "kernel": "maxpool2d",
        "shape": "16x254_size3_stride2",
        "repeat": 32,
        "dataType": "fxp32"
    },
    # Conv2D (3×3, 128) on I(8, 127, 32) --> output: (6, 125, 128)
    {
        "kernel": "conv2d",
        "shape": "8x127_f3",
        "repeat": 128*32,
        "dataType": "fxp32"
    },
    # RELU on I(6, 125, 128) --> output: (6, 125, 128)
    {
        "kernel": "relu",
        "shape": "6x125",
        "repeat": 128,
        "dataType": "fxp32"
    },
    # MaxPool2d (3×3, stride 2) on I(6, 125, 128)--> output: (3, 63, 128)
    {
        "kernel": "maxpool2d",
        "shape": "6x125_size3_stride2",
        "repeat": 128,
        "dataType": "fxp32"
    },
    # (Flatten (3*63,128) and) posEembedding (addition) on I(189, 128) --> output: (189, 128)
    {
      "kernel": "posEmbedding",
      "shape": "189x128",
      "repeat": 1,
      "dataType": "int32"
    }
]


    