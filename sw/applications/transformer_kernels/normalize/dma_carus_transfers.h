// Author: Francesco Poluzzi

#ifndef DMA_CARUS_TRANSFERS_H_
#define DMA_CARUS_TRANSFERS_H_

#include <stdio.h>
#include "dma_sdk.h"
#include "defines.h"

#define CARUS_VREG_SIZE 2048 // Size of a vector register in bytes
#define CARUS_INSTANCE_0 0
#define CARUS_INSTANCE_1 1
#define CARUS_ADD_INSTANCE SINGLE_CARUS_INSTANCE // instance for the add kernel (only one instance is used)

 // Kernels' sizes. Update these when the kernel size changes
#define CARUS_MATMUL_FIXED_SIZE 96
#define CARUS_MOVE_SHIFT_K_T_TRANSFORMER_SIZE 32
#define CARUS_BATCHNORM_MULTIVECTOR_SIZE 76
#define CARUS_ADD_SIZE 60
#define CARUS_MATMUL_FIXED_ACCUMULATE_SIZE 88
#define CARUS_MATMUL_TRANSFORMER_FOLD_SIZE 40

// Kernels' offsets in Carus code memory
#define CARUS_MATMUL_FIXED_OFFSET 0
#define CARUS_MATMUL_FIXED_ACCUMULATE_OFFSET CARUS_MATMUL_FIXED_SIZE
#define CARUS_MATMUL_TRANSFORMER_FOLD_OFFSET CARUS_MATMUL_FIXED_ACCUMULATE_OFFSET+CARUS_MATMUL_FIXED_ACCUMULATE_SIZE
#define CARUS_MOVE_K_TRANSPOSED_OFFSET CARUS_MATMUL_FIXED_SIZE
#define CARUS_BATCHNORM_MULTIVECTOR_OFFSET CARUS_MOVE_K_TRANSPOSED_OFFSET+CARUS_MOVE_SHIFT_K_T_TRANSFORMER_SIZE
#define CARUS_ADD_OFFSET CARUS_BATCHNORM_MULTIVECTOR_OFFSET+CARUS_BATCHNORM_MULTIVECTOR_SIZE

// maxumum sizes for the matmul kernel
#define MAX_A_ROWS 15
#define MAX_A_COLS 16
#define MAX_B_ROWS 16
#define MAX_B_COLS CARUS_VREG_SIZE/4
// vectors for batchnorm kernel
#define MEAN_VECTOR 28
#define VAR_VECTOR 29
#define GAMMA_VECTOR 30
#define BETA_VECTOR 31

/**
 * @brief Load Carus matrix multiplication kernel with fixed-point values.
 * 
 * This function loads the Carus matrix multiplication kernel with fixed-point values
 * for a given Carus instance and offset.
 * 
 * @param carus_instance The Carus instance to load.
 * @param offset The offset to apply.
 */
void __attribute__((noinline)) load_carus_matmul_fixed(uint8_t carus_instance, uint32_t offset);

/**
 * @brief Load Carus custom kernel for self-attention.
 * 
 * This function loads the Carus custom kernel for self-attention for a given Carus instance and offset.
 * 
 * @param carus_instance The Carus instance to load.
 * @param offset The offset to apply.
 */
void __attribute__((noinline)) load_carus_custom_kernel_selfatt(uint8_t carus_instance, uint32_t offset);

/**
 * @brief Load Carus batch normalization kernel.
 * 
 * This function loads the Carus batch normalization kernel for a given Carus instance and offset.
 * 
 * @param carus_instance The Carus instance to load.
 * @param offset The offset to apply.
 */
void __attribute__((noinline)) load_carus_batchnorm(uint8_t carus_instance, uint32_t offset);

/**
 * @brief Load Carus addition kernel.
 * 
 * This function loads the Carus addition kernel for a given Carus instance and offset.
 * 
 * @param carus_instance The Carus instance to load.
 * @param offset The offset to apply.
 */
void __attribute__((noinline)) load_carus_add(uint8_t carus_instance, uint32_t offset);

/**
 * @brief Initializes the system for the "transformer_nmc" application. Initilizes DMA, SPI, Carus, and the interrupts.
 *        Also, loads the initial kernels into the Carus instances.
 * 
 */
int system_initialization();

dma_config_flags_t run_dma_trans(dma_trans_t *trans);

dma_config_flags_t carus_dma_move_input_flattended_no_signext(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel);
dma_config_flags_t carus_dma_move_output_flattended_no_signext(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a flattened matrix to Carus using DMA.
 * 
 * @param matrix_size Size of the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param vreg Virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_flatten_matrix(size_t matrix_size, int16_t *ptr, int16_t starting_index, int8_t vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a transposed flattened matrix to Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param vreg Virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_flatten_transpose_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int8_t vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a matrix to Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a transposed matrix to Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_matrix_transpose(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a tiled matrix to Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param tile_cols Number of columns in the tile.
 * @param matrix_cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_matrix_tiled(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a transposed tiled matrix to Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param tile_cols Number of columns in the tile.
 * @param matrix_cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_matrix_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Writes a transposed tiled matrix to Carus using DMA, flattening it into a vector register.
 * 
 * @param rows Number of rows in the matrix.
 * @param tile_cols Number of columns in the tile.
 * @param matrix_cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_write_matrix_flatten_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Reads a flattened matrix from Carus using DMA.
 * 
 * @param matrix_size Size of the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param vreg Virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_read_flatten_matrix(size_t matrix_size, int16_t *ptr, int16_t starting_index, int16_t vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Reads a matrix from Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_read_matrix(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Reads a transposed matrix from Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_read_matrix_transpose(size_t rows, size_t cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Reads a tiled matrix from Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param tile_cols Number of columns in the tile.
 * @param matrix_cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_read_matrix_tiled(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

/**
 * @brief Reads a transposed tiled matrix from Carus using DMA.
 * 
 * @param rows Number of rows in the matrix.
 * @param tile_cols Number of columns in the tile.
 * @param matrix_cols Number of columns in the matrix.
 * @param ptr Pointer to the matrix data.
 * @param starting_index Starting index in the Carus VRF.
 * @param starting_vreg Starting virtual register index.
 * @param carus_instance Carus instance index.
 * @param dma_channel DMA channel to use.
 * @return dma_config_flags_t DMA configuration flags.
 */
dma_config_flags_t carus_read_matrix_tiled_transpose(size_t rows, size_t tile_cols, size_t matrix_cols, int16_t *ptr, int16_t starting_index, int16_t starting_vreg, int8_t carus_instance, int8_t dma_channel);

#endif /* DMA_TRANSFERS_H_ */