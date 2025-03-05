#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>

// Macros
// ------
#define ARG0 2 // kernel argument 0: rows of A
#define ARG1 16 // kernel argument 1: columns of A
#define VL 2 // vector length: columns of B
#define ELEM_SIZE 4 // element size in bytes
#define data_t int32_t // element data type

// Input matrix size
#define A_SIZE 128
#define A_ROWS 2
#define A_COLS 16
#define B_SIZE 128
#define B_ROWS 16
#define B_COLS 2

// Output matrix size
#define R_SIZE 16
#define R_ROWS 2
#define R_COLS 2

// Input matrices
// --------------
int32_t A [] __attribute__((section(".xheep_data_flash_only"))) = {
    0x00000000, 0x00000001, 0x00000000, 0x00000002, 0x00000003, 0x00000001, 0x00000001, 0x00000002, 0x00000002, 0x00000001, 0x00000003, 0x00000001, 0x00000003, 0x00000002, 0x00000002, 0x00000000,
    0x00000001, 0x00000002, 0x00000003, 0x00000002, 0x00000003, 0x00000003, 0x00000002, 0x00000002, 0x00000003, 0x00000000, 0x00000003, 0x00000000, 0x00000003, 0x00000001, 0x00000000, 0x00000003
};

int32_t B [] __attribute__((section(".xheep_data_flash_only"))) = {
    0x00000002, 0x00000003,
    0x00000002, 0x00000002,
    0x00000003, 0x00000001,
    0x00000001, 0x00000002,
    0x00000002, 0x00000000,
    0x00000001, 0x00000000,
    0x00000003, 0x00000003,
    0x00000001, 0x00000000,
    0x00000001, 0x00000003,
    0x00000000, 0x00000001,
    0x00000001, 0x00000003,
    0x00000000, 0x00000000,
    0x00000001, 0x00000003,
    0x00000000, 0x00000003,
    0x00000003, 0x00000002,
    0x00000000, 0x00000001
};

// Output matrices
// ---------------
int32_t R [] __attribute__((section(".xheep_data_flash_only"))) = {
    0x0000001e, 0x0000002c,
    0x0000002b, 0x00000035
};

#endif // DATA_H_
