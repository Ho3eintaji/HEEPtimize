#ifndef CARUS_MAXPOOL_H_
#define CARUS_MAXPOOL_H_

#include <stdint.h>

// Macros
// ------
#define CARUS_MAXPOOL_A_VREG 0 // v0-v15
#define CARUS_MAXPOOL_R_VREG 16 // v16-v31
#define TMP_VREG0 30 // v30
#define TMP_VREG1 31 // v31

// Binary size
// -----------
#define CARUS_MAXPOOL_SIZE 224

// Binary files
// ------------
uint32_t carus_maxpool[] = {
    0xC8221131,
    0x1E802783,
    0x1EC02403,
    0xF7D7C626,
    0x873E8087,
    0x2783C23E,
    0x24231FC0,
    0x26831EE0,
    0x87131F40,
    0x25030107,
    0x25831F80,
    0xC03A1E40,
    0x67C1CFD9,
    0x428117FD,
    0xC43E4341,
    0x001F03B7,
    0x04B34792,
    0xF0570073,
    0x87138087,
    0x97930012,
    0x07220102,
    0x67338F5D,
    0x005B0067,
    0x47A21EE0,
    0x00831613,
    0x8F7D92AA,
    0x01031793,
    0x87138FD9,
    0x63B301F3,
    0x470900E6,
    0x04D77F63,
    0x10078793,
    0x1EF0005B,
    0x9BE30705,
    0xB05BFEE6,
    0x005B3E70,
    0x47891E70,
    0x01E66613,
    0x3EC7C05B,
    0x1FFF0FD7,
    0x9BE30785,
    0x4785FEF6,
    0x00B7FB63,
    0x265B872A,
    0x6FDB43F7,
    0x078542F6,
    0x9AE3972A,
    0xF057FEF5,
    0x005B8085,
    0x47825E90,
    0x99E30305,
    0x4442F667,
    0x015144B2,
    0xB05B8082,
    0x005B3E70,
    0xB7E11E70
};

#endif // CARUS_MAXPOOL_H_
