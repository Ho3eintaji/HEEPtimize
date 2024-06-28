#ifndef CARUS_RELU_H_
#define CARUS_RELU_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_RELU_A_REG 0 // # v0-v15
#define CARUS_RELU_R_REG 16 // # v16-v31

// Binary size
// -----------
#define CARUS_RELU_SIZE 76

// Binary files
// ------------
uint32_t carus_relu[] = {
    0x1E802283,
    0x1EC02303,
    0x8062F2D7,
    0x1E502423,
    0x1F002503,
    0x25834281,
    0x03371F40,
    0x03410010,
    0x405BE989,
    0x02851E60,
    0x10130313,
    0xFEA2CBE3,
    0x63C18082,
    0x10138393,
    0xA665C05B,
    0x1E60005B,
    0x931E0285,
    0xFEA2CAE3,
    0x00008082
};

#endif // CARUS_RELU_H_
