#ifndef CARUS_MATMUL_H_
#define CARUS_MATMUL_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_MATMUL_A_VREG 0 // v0 (flattened)
#define CARUS_MATMUL_B_VREG 1 // v1-v15 (15 rows max)
#define CARUS_MATMUL_R_VREG 16 // v16-v31 (16 rows max)

// Binary size
// -----------
#define CARUS_MATMUL_SIZE 84

// Binary files
// ------------
uint32_t carus_matmul[] = {
    0x1E802283,
    0x23034601,
    0x46811EC0,
    0x8062F2D7,
    0x1E502423,
    0x25034281,
    0x03131F00,
    0x25831100,
    0xA4DB1F40,
    0x02854202,
    0xE05BE681,
    0xA0199664,
    0xB664E05B,
    0x03130685,
    0xE4E31003,
    0x7313FEB6,
    0x03130FF3,
    0x06051013,
    0x6CE34681,
    0x8082FCA6,
    0x00000000
};

#endif // CARUS_MATMUL_H_
