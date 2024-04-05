// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_maxpool.S
// Author: Michele Caon
// Date: 28/10/2023
// Description: NM-Carus max pooling kernel

// This kernel computes the max pooling of a 2D matrix, with arbitrary pooling
// size and stride:
//       R = maxpool2d(A, pool_size, stride)
// where:
// - A is an [MxN] input matrix
// - R is an [MxN] output matrix
// - size: height and width of the pooling window
// - stride: pooling stride
// The calling software shall prove the following arguments:
// - ARG0: rows(A)
// - VL: cols(A), equal to the vector length
// - ARG1: pooling size (>= 2)
// - ARG2: pooling stride (>= 1)
// - ARG3: rows(R) = (rows(A) - size) / stride + 1 (must be integer)
// - SCRATCH: cols(R) = (cols(A) - size) / stride + 1 (must be integer)
// Registers v0-v15 are used to store the input matrix, while registers v16-v31
// contain the output after pooling. The dimension of the output matrix depends
// on the pooling size and stride.

// TODO: currently, there are no NM-Carus vector-scalar move instructions
// that allow for indirect vector register indexing (like there are for
// arithmetics, e.g., xvmacc.vv). Therefore, this implementation uses a
// single vector register v31 to buffer the pooled elements, and then
// moves the resulting vector into the final position using a xvmv.v.v
// custom instruction with indirect indexing. This is very inefficient: we
// should consider adding the custom move instructions to NM-Carus,
// although they would require support for three source scalar operands:
// 1. GPR containing the element index
// 2. GPR containing the vector register index
// 3. GPR containing the scalar data to store

#include <stdint.h>

// NM-Carus address map
#include "carus_regs.h"

// Register map
#define CARUS_MAXPOOL_A_VREG 0 // v0-v15
#define CARUS_MAXPOOL_R_VREG 16 // v16-v31
#define TMP_VREG0 30 // v30
#define TMP_VREG1 31 // v31

void _start(void) {
    // Configure vector type and length, and return HW-validated vector length
    uint32_t vl = *CARUS_VL_REG;
    const uint32_t vtype = *CARUS_VTYPE_REG;
    asm volatile ("vsetvl %0, %0, %1" : "+r"(vl) : "r"(vtype));
    *CARUS_VL_REG = vl;

    // Load kernel parameters
    const uint32_t pool_size = *CARUS_ARG1_REG;
    const uint32_t pool_stride = *CARUS_ARG2_REG;
    const uint32_t row_r = *CARUS_ARG3_REG;
    const uint32_t col_r = *CARUS_SCRATCH_REG;

    // Apply pooling
    // -------------
    unsigned int vregs; // custom instructions source and destination register indexes
    unsigned int out_vregs = (TMP_VREG1 << 16) | CARUS_MAXPOOL_R_VREG;
    unsigned int out_reg_idx = CARUS_MAXPOOL_R_VREG; // first register index for the output matrix
    unsigned int first_reg_idx = CARUS_MAXPOOL_A_VREG; // first register index for the current row widnow
    unsigned int elem_idx = 0; // current element index
    int32_t elem; // current element value

    // Iterate over the output rows
    for (unsigned int i = 0; i < row_r; i++) {
        // Set VL to the number of input columns
        asm volatile ("vsetvl zero, %0, %1" : : "r"(vl), "r"(vtype));

        // Max by columns
        // --------------
        // v[out_reg_idx] = max(v[first_reg_idx], v[first_reg_idx+1])
        vregs = (first_reg_idx << 16) | ((first_reg_idx + 1) << 8) | out_reg_idx;
        asm volatile ("xvmax.vv %0" : : "r"(vregs));

        // Iterate over the window input rows
        vregs &= 0x00ffff; // reset vs1 index
        vregs |= (out_reg_idx << 16); // use v[out_reg_idx] as vs1
        for (unsigned int j = 2; j < pool_size; j++) {
            vregs += 0x000100; // increment vs2 index
            asm volatile ("xvmax.vv %0" : : "r"(vregs));
        }
        first_reg_idx += pool_stride;

        // Max by row
        // ----------
        // v31 = max(v[out_reg_idx], (v[out_reg_idx] << 1))
        vregs = (TMP_VREG1 << 16) | (out_reg_idx << 8) | TMP_VREG1; // store shifted output row in v31
        asm volatile ("xvslidedown.vi %0, 1" : : "r"(vregs)); // v31 = v[out_reg_idx] << 1
        asm volatile ("xvmax.vv %0" : : "r"(vregs)); // v31 = max(v31, v[out_reg_idx])
        
        // Slide and max for the remaining pooling window columns
        vregs = (out_reg_idx << 8) | TMP_VREG0; // store shifted output row in v30
        for (unsigned int j = 2; j < pool_size; j++) {
            asm volatile ("xvslidedown.vx %0, %1" : : "r"(vregs), "r"(j)); // v30 = v[out_reg_idx] << j
            asm volatile ("vmax.vv v31, v31, v30"); // v31 = max(v31, v30)
        }

        // Gather meaningful output elements
        elem_idx = pool_stride;
        for (unsigned int k = 1; k < col_r; k++) {
            // v31[k*stride] = v31[k]
            asm volatile ("xvmv.x.s %0, v31, %1" : "=r"(elem) : "r"(elem_idx)); // elem = v31[k*stride]
            asm volatile ("xvmv.s.x v31, %0, %1" : : "r"(elem), "r"(k)); // v31[k] = elem
            elem_idx += pool_stride;
        }

        // Move the output row to the final position
        asm volatile ("vsetvl zero, %0, %1" : : "r"(col_r), "r"(vtype));
        asm volatile ("xvmv.v.v %0" : : "r"(out_vregs)); // v[out_reg_idx] = v31
        out_reg_idx++;
        out_vregs += 1; // increment output register index
    }

    // Return to loader
    return;
}
