// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus.h
// Author: Michele Caon
// Date: 19/06/2023
// Description: Header file for NM-Carus driver

#ifndef CARUS_H_
#define CARUS_H_

#include <stdint.h>

#include "carus_addr_map.h"

/*********************/
/* ---- DEFINES ---- */
/*********************/

// Vector register file
#define VLEN_MAX ${carus_vlen_max} // maximum vector length in bytes

// NM-Carus control register initializer (for carus_ctl_t structures)
#define CARUS_CTL_INIT {\\

    .start    = 0x0,\\

    .done     = 0x0,\\

    .err      = 0x0,\\

    .done_en  = 0x0,\\

    .fetch_en = 0x0,\\

    .boot_pc  = 0x0,\\

}

// NM-Carus configuration registers initializer (for carus_cfg_t structures)
#define CARUS_CFG_INIT {\\

    .koffs   = 0x0,\\
    
    .scratch = 0x0,\\
    
    .vl      = VLEN_MAX,\\
    
    .vtype   = VTYPE_VSEW_32,\\
    
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

// Vector register file addresses 
extern const uint8_t *vregs[];

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/

/**
 * @brief Get the specified NM-Carus instance operating mode.
 * @param inst NM-Carus instance number.
 * @param mode Pointer to the operating mode variable to be filled.
 * @return 0 if success, -1 otherwise.
 */
int carus_get_mode(const uint8_t inst, carus_mode_t *mode);

/**
 * @brief Set the specified NM-Carus instance operating mode.
 * @param inst NM-Carus instance number.
 * @param mode Operating mode (memory or configuration).
 * @return 0 if success, -1 otherwise.
 */
int carus_set_mode(const uint8_t inst, const carus_mode_t mode);

/**
 * @brief Get the status data of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param ctl Pointer to the control data structure to be filled.
 * @return 0 if success, -1 otherwise.
 */
int carus_get_ctl(const uint8_t inst, carus_ctl_t *ctl);

/**
 * @brief Write the control register of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param cfg Pointer to the configuration structure to be set.
 * @return 0 if success, -1 otherwise.
 */
int carus_set_ctl(const uint8_t inst, const carus_ctl_t *ctl);

/**
 * @brief Get the parameters of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param params Pointer to the parameters structure to be filled.
 * @return 0 if success, -1 otherwise.
 */
int carus_get_cfg(const uint8_t inst, carus_cfg_t *cfg);

/**
 * @brief Set the parameters of the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param params Pointer to the parameters structure to be set.
 * @return 0 if success, -1 otherwise.
 */
int carus_set_cfg(const uint8_t inst, const carus_cfg_t *cfg);

/**
 * @brief Initialize the specified NM-Carus instance
 * @details This function must be called by the user code before any other configuration function. It resets the specified NM-Carus instance to a known state and it loads the startup up code (kernel loader) into the eCPU memory.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int carus_init(const uint8_t inst);

/**
 * @brief Load a vector kernel into the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param kernel Pointer to the kernel to be loaded.
 * @param size Size of the kernel to be loaded.
 * @param offs Offset at which to load the kernel in bytes.
 * @return 0 if success, -1 otherwise.
 */
int carus_load_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size, const uint32_t offs);

/**
 * @brief Run the current vector kernel on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int carus_run_kernel(const uint8_t inst);

/**
 * @brief Wait for the current vector kernel to finish on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @return 0 if success, -1 otherwise.
 */
int carus_wait_done(const uint8_t inst);

/**
 * @brief Load and execute a vector kernel on the specified NM-Carus instance.
 * @param inst NM-Carus instance number.
 * @param kernel Pointer to the kernel to be executed.
 * @param size Size of the kernel to be executed.
 * @return 0 if success, -1 otherwise.
 */
int carus_exec_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size);

/**
 * @brief Move a vector from and to the system memory to and from NM-Carus instance vector register file.
 * @param inst NM-Carus instance number.
 * @param src Pointer to the vector to be read.
 * @param tgt Pointer to the vector to be written.
 * @param size Size of the vector to be loaded (bytes)
 */
int carus_copy_vector_to_vector(const uint8_t inst, const uint32_t *src, const uint32_t *tgt, const uint32_t size);

/**
 * @brief Load a scalar in the specified NM-Carus instance vector register file (replicated size times).
 * @param inst NM-Carus instance number.
 * @param src the scalar to be loaded.
 * @param tgt Pointer to the vector destination register.
 * @param size Size of the vector to be loaded (number of words, not bytes)
 */
int carus_copy_scalar_to_vector(const uint8_t inst, const uint32_t src, const uint32_t *tgt, const uint32_t size);


#endif // CARUS_H_
