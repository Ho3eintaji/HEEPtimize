// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus.h
// Author: Michele Caon, Luig Giuffrida
// Date: 20/06/2024
// Description: Header file for NM-Carus driver

#ifndef CARUS_H_
#define CARUS_H_

#include <stdint.h>

#include "heepatia.h"
#include "carus_addr_map.h"
#include "dma.h"

/*********************/
/* ---- DEFINES ---- */
/*********************/

// Vector register file length in bytes
%for inst in range(carus_num):
#define CARUS${inst}_VLEN_MAX ${carus_vlen_max[inst]}
%endfor

#define carus_vrf(inst, v) (int8_t*)(carus[inst] + (v) * carus_vlen[inst])

// NM-Carus control register initializer (for carus_ctl_t structures)
#define CARUS_CTL_INIT {\\

    .start    = 0x0,\\

    .done     = 0x0,\\

    .err      = 0x0,\\

    .done_en  = 0x0,\\

    .fetch_en = 0x0,\\

    .boot_pc  = 0x0,\\

}

// NM-Carus configuration register initializer (for carus_cfg_t structures)
#define CARUS_CFG_INIT(inst) {\\

    .koffs   = 0x0,\\

    .scratch = 0x0,\\

    .vl      = carus_vlen[inst],\\

    .vtype   = 0x0,\\

    .arg0    = 0x0,\\

    .arg1    = 0x0,\\

    .arg2    = 0x0,\\

    .arg3    = 0x0,\\

}

/************************/
/* ---- DATA TYPES ---- */
/************************/

// NM-Carus operating modes
typedef enum {
    CARUS_MODE_MEM, // Memory mode
    CARUS_MODE_CFG, // Configuration mode
} carus_mode_t;

// NM-Carus OP_CTL configuration register
typedef struct {
    uint8_t  start;    // start bit
    uint8_t  done;     // done bit
    uint8_t  err;      // error bit
    uint8_t  done_en;  // done interrupt enable bit
    uint8_t  fetch_en; // eCPU fetch enable bit
    uint16_t boot_pc;  // eCPU boot program counter
} carus_ctl_t;

// NM-Carus parameters
typedef struct {
    uint32_t koffs;    // kernel address offset
    uint32_t scratch;  // scratchpad data
    uint32_t vl;       // requested vector length
    uint32_t vtype;    // requested vector type
    uint32_t arg0;     // first kernel argument
    uint32_t arg1;     // second kernel argument
    uint32_t arg2;     // third kernel argument
    uint32_t arg3;     // fourth kernel argument
} carus_cfg_t;

// NM-Carus vtype flags
enum vtype_flags_e {
    VTYPE_VSEW_8  = 0x00, // 8-bit elements
    VTYPE_VSEW_16 = 0x08, // 16-bit elements
    VTYPE_VSEW_32 = 0x10, // 32-bit elements
};

/***************************************/
/* ---- EXPORTED GLOBAL VARIABLES ---- */
/***************************************/

// Pointers to carus instances

extern int32_t carus[CARUS_NUM];

extern const int32_t carus_vlen[CARUS_NUM];

// NM-Carus configuration registers initializers (for carus_cfg_t structures)
extern const carus_cfg_t carus_cfg_init[CARUS_NUM];

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/

/**
 * @brief Get the specified NM-Carus instance operating mode.
 * @param inst NM-Carus instance number.
 * @param mode Pointer to the operating mode variable to be filled.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_get_mode(const uint8_t inst, carus_mode_t *mode);

/**
 * @brief Set the specified NM-Carus instance operating mode.
 * @param inst NM-Carus instance number.
 * @param mode Operating mode (memory or configuration).
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_set_mode(const uint8_t inst, const carus_mode_t mode);

/**
 * @brief Get the status data of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param ctl Pointer to the control data structure to be filled.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_get_ctl(const uint8_t inst, carus_ctl_t *ctl);

/**
 * @brief Write the control register of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param cfg Pointer to the configuration structure to be set.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_set_ctl(const uint8_t inst, const carus_ctl_t *ctl);

/**
 * @brief Get the parameters of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param params Pointer to the parameters structure to be filled.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_get_cfg(const uint8_t inst, carus_cfg_t *cfg);

/**
 * @brief Set the parameters of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param params Pointer to the parameters structure to be set.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_set_cfg(const uint8_t inst, const carus_cfg_t *cfg);

/**
 * @brief Initialize the specified NM-Carus instance
 * @details This function must be called by the user code before any other configuration function. It resets the specified NM-Carus instance to a known state and it loads the startup up code (kernel loader) into the eCPU memory.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_init(const uint8_t inst);

/**
 * @brief Load a vector kernel into the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param kernel Pointer to the kernel to be loaded.
 * @param size Size of the kernel to be loaded.
 * @param offs Offset at which to load the kernel in bytes.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_load_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size, const uint32_t offs);

/**
 * @brief Run the current vector kernel on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_run_kernel(const uint8_t inst);

/**
 * @brief Wait for the current vector kernel to finish on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_wait_done(const uint8_t inst);

/**
 * @brief Load and execute a vector kernel on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param kernel Pointer to the kernel to be executed.
 * @param size Size of the kernel to be executed.
 * @return 0 if success, -1 otherwise.
 */
int __attribute__ ((noinline)) carus_exec_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size);

void __attribute__((noinline)) carus_irq(uint32_t id);

#endif // CARUS_H_
