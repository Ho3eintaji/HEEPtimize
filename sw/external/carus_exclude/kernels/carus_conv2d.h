#ifndef CARUS_CONV2D_H_
#define CARUS_CONV2D_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_CONV2D_A_VREG 0 // # v0-v9
#define CARUS_CONV2D_R_VREG 20 // # v20-v27
#define CARUS_CONV2D_F_VREG 31 // # v31 (flattened and transposed)

// Binary size
// -----------
#define CARUS_CONV2D_SIZE 120

// Binary files
// ------------
uint32_t carus_conv2d[] = {
    0x1E802283,
    0x23034401,
    0x44811EC0,
    0x8062F2D7,
    0x1E502423,
    0x25034601,
    0x42811F00,
    0x1F402583,
    0x06B34351,
    0x068540B5,
    0x43F2A25B,
    0xB662605B,
    0x031392AE,
    0x06051003,
    0xFEB648E3,
    0x93934601,
    0x03330085,
    0x03134073,
    0x04851013,
    0xCDE382A2,
    0x0405FCD4,
    0x428143A9,
    0x3E74405B,
    0x10138393,
    0xCBE30285,
    0x82A2FEA2,
    0x03136305,
    0x4481A143,
    0xFAB44CE3,
    0x00008082
};

#endif // CARUS_CONV2D_H_
