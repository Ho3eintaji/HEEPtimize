#ifndef CARUS_GEMM_H_
#define CARUS_GEMM_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_GEMM_A_VREG 0 // v0 (flattened)
#define CARUS_GEMM_B_VREG 2 // v2-v11 (10 rows max)
#define CARUS_GEMM_C_VREG 12 // v12-v21 (10 rows max)
#define CARUS_GEMM_R_VREG 22 // v22-v31 (10 rows max)

// Binary size
// -----------
#define CARUS_GEMM_SIZE 116

// Binary files
// ------------
uint32_t carus_gemm[] = {
    0x1F002503,
    0x1F402583,
    0x1F802603,
    0x1FC02683,
    0x1E402283,
    0x23034401,
    0x44811EC0,
    0x8062F2D7,
    0x960660D7,
    0x1E802283,
    0x8062F2D7,
    0x1E502423,
    0x63054281,
    0xC1630313,
    0x9666E05B,
    0x10130313,
    0x6BE30405,
    0x4401FEA4,
    0x21600313,
    0x4212A25B,
    0x605B0285,
    0x0485B662,
    0x10030313,
    0xFEB4E8E3,
    0x0FF37313,
    0x20130313,
    0x44810405,
    0xFEA460E3,
    0x00008082
};

#endif // CARUS_GEMM_H_
