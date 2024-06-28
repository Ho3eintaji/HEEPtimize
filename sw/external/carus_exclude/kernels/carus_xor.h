#ifndef CARUS_XOR_H_
#define CARUS_XOR_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_XOR_A_VREG 0 // v0
#define CARUS_XOR_B_VREG 10 // v10
#define CARUS_XOR_R_VREG 20 // v20

// Binary size
// -----------
#define CARUS_XOR_SIZE 60

// Binary files
// ------------
uint32_t carus_xor[] = {
    0x1E802503,
    0x2483842A,
    0x45A91EC0,
    0x03136305,
    0x63C1A143,
    0x10138393,
    0x72D74605,
    0x8D918094,
    0x40540433,
    0x2E60005B,
    0xC199931E,
    0xFE8047E3,
    0x24238D01,
    0x80821EA0,
    0x00000000
};

#endif // CARUS_XOR_H_
