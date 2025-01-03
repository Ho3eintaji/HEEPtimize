// Author: Francesco Poluzzi

#include <stdlib.h>
#include <stdio.h>
#include "heepatia.h"
#include "csr.h"
#include "fast_intr_ctrl.h"
#include "dma_sdk.h"
#include "carus.h"
#include "data.h"
#include "carus_dma_functions.h"

int16_t R_Matrix[A_ROWS*A_COLS] __attribute__((section(".xheep_data_interleaved"))) = {0};
#define CARUS_INSTANCE 0
#define DMA_CHANNEL 0

int check_matrix(int16_t *A, int16_t *B, int rows, int cols, int tile_cols){
    for(int i=0; i<rows; i++){
        for(int j=0; j<tile_cols; j++){
            if(A[i*cols + j] != B[i*cols + j]){
                printf("Error: A[%d,%d] = %x, R_matrix[%d,%d] = %x\n", i, j, A[i*cols + j], i, j, B[i*cols + j]);
                return 1;
            }
        }
    }
    return 0;
}

void reset_matrix(int16_t *matrix, int rows, int cols){
    for(int i=0; i<rows; i++){
        for(int j=0; j<cols; j++){
            matrix[i*cols + j] = 0;
        }
    }
    // write 0 all over Carus
    for(int i=0; i<32; i++){
        int32_t *row_ptr = (int32_t *) carus_vrf(CARUS_INSTANCE, i);
        for(int j=0; j<A_COLS; j++){
            row_ptr[j] = 0;
        }
    }
}

int main (void){
    
    // Initialize the DMA
    dma_sdk_init();
    // Initialize NM-Carus
    if (carus_init(CARUS_INSTANCE) != 0)return 1;

    /////////////////////////////
    // Test the DMA functions //
    ///////////////////////////

    // matrix flattened in a vector register
    carus_write_flatten_matrix(A_ROWS*A_COLS, A, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    carus_read_flatten_matrix(A_ROWS*A_COLS, R_Matrix, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    if(check_matrix(A, R_Matrix, A_ROWS, A_COLS, A_COLS) != 0){
        printf("Error: flatten matrix\n");
        return 1;
    }
    reset_matrix(R_Matrix, A_ROWS, A_COLS);

    // regolar matrix move (1 row per vector register)
    carus_write_matrix(A_ROWS, A_COLS, A, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    carus_read_matrix(A_ROWS, A_COLS, R_Matrix, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    if(check_matrix(A, R_Matrix, A_ROWS, A_COLS, A_COLS) != 0){
        printf("Error: regolar matrix move\n");
        return 1;
    }    
    reset_matrix(R_Matrix, A_ROWS, A_COLS);

    // transpose matrix move (1 column per vector register)
    carus_write_matrix_transpose(A_ROWS, A_COLS, A, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    carus_read_matrix_transpose(A_ROWS, A_COLS, R_Matrix, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    if(check_matrix(A, R_Matrix, A_ROWS, A_COLS, A_COLS) != 0){
        printf("Error: transpose matrix move\n");
        return 1;
    }  
    reset_matrix(R_Matrix, A_ROWS, A_COLS);  

    // tiled matrix move
    carus_write_matrix_tiled(A_ROWS, TILE_COLS, A_COLS, A, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    carus_read_matrix_tiled(A_ROWS, TILE_COLS, A_COLS, R_Matrix, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    if(check_matrix(A, R_Matrix, A_ROWS, A_COLS, TILE_COLS) != 0){
        printf("Error: tiled matrix move\n");
        return 1;
    }
    reset_matrix(R_Matrix, A_ROWS, A_COLS);

    // tiled and transposed matrix move
    carus_write_matrix_tiled_transpose(A_ROWS, TILE_COLS, A_COLS, A, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    carus_read_matrix_tiled_transpose(A_ROWS, TILE_COLS, A_COLS, R_Matrix, 0, 0, CARUS_INSTANCE, DMA_CHANNEL);
    DMA_WAIT(DMA_CHANNEL);
    if(check_matrix(A, R_Matrix, A_ROWS, A_COLS, TILE_COLS) != 0){
        printf("Error: tiled matrix move\n");
        return 1;
    }
    reset_matrix(R_Matrix, A_ROWS, A_COLS);


    
    printf("All tests passed :)\n");
}
