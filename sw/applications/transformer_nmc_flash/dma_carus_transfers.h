// Author: Francesco Poluzzi

#ifndef DMA_CARUS_TRANSFERS_H_
#define DMA_CARUS_TRANSFERS_H_

#include <stdio.h>
#include "dma_sdk.h"

#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define CARUS_INSTANCE_0 0
#define CARUS_INSTANCE_1 1

#define CARUS_MATMUL_FIXED_SIZE 96 // update when the kernel size changes
#define CARUS_MOVE_SHIFT_K_T_TRANSFORMER_SIZE 32
#define CARUS_MATMUL_FIXED_ACCUMULATE_SIZE 88

#define CARUS_MATMUL_FIXED_OFFSET 0
#define CARUS_MOVE_K_TRANSPOSED_OFFSET CARUS_MATMUL_FIXED_SIZE
#define CARUS_MATMUL_FIXED_ACCUMULATE_OFFSET CARUS_MOVE_K_TRANSPOSED_OFFSET+CARUS_MOVE_SHIFT_K_T_TRANSFORMER_SIZE

#define MAX_A_COLS 16
#define MAX_B_ROWS 16

void __attribute__((noinline)) load_carus_matmul_fixed(uint8_t carus_instance, uint32_t offset);
void __attribute__((noinline)) load_carus_custom_kernel_selfatt(uint8_t carus_instance, uint32_t offset);
void  initialize_interrupts();
int system_initialization();

dma_config_flags_t run_dma_trans(dma_trans_t *trans);

/*
Prepare a DMA transaction for moving transposed matrix A to Carus VREG
rows and cols are the dimensions of the matrix not transposed.
This matrix A is the matrix B in the  non-transposed matrix multiplication.
This function is meant to be used for the matmul kernels of carus.
Matrix A will be disposed in VREG[0] flattened.
*/
dma_config_flags_t carus_dma_transpose_matmul_A(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel);

/*
Prepare a DMA transaction for moving (non-transposed) matrix A to Carus VREG
rows and cols are the dimensions of the matrix.
This function is meant to be used for the matmul kernels of carus.
Matrix A will be disposed in VREG[0] flattened.
*/
dma_config_flags_t carus_dma_move_matmul_A_flattened(size_t matrix_size, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel);

/*
Prepare a DMA transaction for moving transposed matrix B to Carus VREG
rows and cols are the dimensions of the matrix not transposed.
This matrix B is the matrix A in the non-transposed matrix multiplication
This function is meant to be used for the matmul kernels of carus.
Matrix B will be disposed from VREG[1] to VREG[17] in rows.
*/
dma_config_flags_t carus_dma_transpose_matmul_B(size_t rows, size_t cols, int16_t *ptr, int8_t carus_instance, int8_t dma_channel);

/*
Prepare a DMA transaction for moving matmul result from Carus VREG to memory while performing the transposition
rows and cols are the dimensions of the matrix not transposed.
*/
dma_config_flags_t carus_dma_transpose_matmul_R(size_t rows, size_t cols, int16_t *ptr, int8_t vreg_offset, int8_t carus_instance, int8_t dma_channel);



dma_config_flags_t carus_dma_move_matmul_R(size_t rows, size_t cols, int16_t *ptr, int8_t carus_instance, int8_t dma_channel);

dma_config_flags_t carus_dma_move_matmul_A_vertical_tile(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel);
dma_config_flags_t carus_dma_move_matmul_B_horizontal_tile(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t carus_instance, int8_t dma_channel);

#endif /* DMA_TRANSFERS_H_ */