// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus.c
// Author: Michele Caon
// Date: 19/06/2023
// Description: Driver for NM-Carus near-memory computing peripheral.

#include "carus.h"
#include "carus_loader.h"
#include "heepatia.h"
#include "heepatia_ctrl_reg.h"
#include "carus_addr_map.h"
#include "dma_util.h"
#include "ext_irq.h"
#include "hart.h"

/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// Vector register file addresses
const uint8_t *vregs[] = {
    (uint8_t *)(0 * VLEN_MAX),  // v0
    (uint8_t *)(1 * VLEN_MAX),  // v1
    (uint8_t *)(2 * VLEN_MAX),  // v2
    (uint8_t *)(3 * VLEN_MAX),  // v3
    (uint8_t *)(4 * VLEN_MAX),  // v4
    (uint8_t *)(5 * VLEN_MAX),  // v5
    (uint8_t *)(6 * VLEN_MAX),  // v6
    (uint8_t *)(7 * VLEN_MAX),  // v7
    (uint8_t *)(8 * VLEN_MAX),  // v8
    (uint8_t *)(9 * VLEN_MAX),  // v9
    (uint8_t *)(10 * VLEN_MAX), // v10
    (uint8_t *)(11 * VLEN_MAX), // v11
    (uint8_t *)(12 * VLEN_MAX), // v12
    (uint8_t *)(13 * VLEN_MAX), // v13
    (uint8_t *)(14 * VLEN_MAX), // v14
    (uint8_t *)(15 * VLEN_MAX), // v15
    (uint8_t *)(16 * VLEN_MAX), // v16
    (uint8_t *)(17 * VLEN_MAX), // v17
    (uint8_t *)(18 * VLEN_MAX), // v18
    (uint8_t *)(19 * VLEN_MAX), // v19
    (uint8_t *)(20 * VLEN_MAX), // v20
    (uint8_t *)(21 * VLEN_MAX), // v21
    (uint8_t *)(22 * VLEN_MAX), // v22
    (uint8_t *)(23 * VLEN_MAX), // v23
    (uint8_t *)(24 * VLEN_MAX), // v24
    (uint8_t *)(25 * VLEN_MAX), // v25
    (uint8_t *)(26 * VLEN_MAX), // v26
    (uint8_t *)(27 * VLEN_MAX), // v27
    (uint8_t *)(28 * VLEN_MAX), // v28
    (uint8_t *)(29 * VLEN_MAX), // v29
    (uint8_t *)(30 * VLEN_MAX), // v30
    (uint8_t *)(31 * VLEN_MAX)  // v31
};

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/
// Get NM-Carus configuration mode
int carus_get_mode(const uint8_t inst, carus_mode_t *mode)
{
    const uint32_t *op_mode = (uint32_t *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_OP_MODE_REG_OFFSET);

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Get mode
    if (*op_mode & (1 << (inst + HEEPATIA_CTRL_OP_MODE_CARUS_IMC_0_BIT)))
    {
        *mode = CARUS_MODE_CFG;
    }
    else
    {
        *mode = CARUS_MODE_MEM;
    }

    return 0;
}

// Set NM-Carus operating mode
int carus_set_mode(const uint8_t inst, const carus_mode_t mode)
{
    uint32_t *op_mode = (uint32_t *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_OP_MODE_REG_OFFSET);

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Set mode
    if (mode == CARUS_MODE_CFG)
    {
        *op_mode |= (1 << (inst + HEEPATIA_CTRL_OP_MODE_CARUS_IMC_0_BIT));
        asm volatile("nop"); // Wait device configuration to be completed
    }
    else
    {
        *op_mode &= ~(1 << (inst + HEEPATIA_CTRL_OP_MODE_CARUS_IMC_0_BIT));
        asm volatile("nop"); // Wait device configuration to be completed
    }

    return 0;
}

// Get NM-Carus configuration
int carus_get_ctl(const uint8_t inst, carus_ctl_t *ctl)
{
    const uint32_t *op_ctl_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_OP_CTL_REG_ADDR);
    uint32_t op_ctl;

    // Set instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Get configuration
    op_ctl = *op_ctl_ptr;
    ctl->start = (op_ctl >> CARUS_CTL_OP_CTL_START_BIT) & 0x1;
    ctl->done = (op_ctl >> CARUS_CTL_OP_CTL_DONE_BIT) & 0x1;
    ctl->err = (op_ctl >> CARUS_CTL_OP_CTL_ERR_BIT) & 0x1;
    ctl->done_en = (op_ctl >> CARUS_CTL_OP_CTL_DONE_EN_BIT) & 0x1;
    ctl->fetch_en = (op_ctl >> CARUS_CTL_OP_CTL_CPU_FETCH_EN_BIT) & 0x1;
    ctl->boot_pc = (op_ctl >> CARUS_CTL_OP_CTL_BOOT_PC_OFFSET) & 0xff;

    // Set instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Set NM-Carus configuration
int carus_set_ctl(const uint8_t inst, const carus_ctl_t *ctl)
{
    uint32_t *op_ctl = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_OP_CTL_REG_ADDR);

    // Set instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Write configuration
    *op_ctl = ((ctl->start & 0x1) << CARUS_CTL_OP_CTL_START_BIT) |
              ((ctl->done & 0x1) << CARUS_CTL_OP_CTL_DONE_BIT) |
              ((ctl->err & 0x1) << CARUS_CTL_OP_CTL_ERR_BIT) |
              ((ctl->done_en & 0x1) << CARUS_CTL_OP_CTL_DONE_EN_BIT) |
              ((ctl->fetch_en & 0x1) << CARUS_CTL_OP_CTL_CPU_FETCH_EN_BIT) |
              ((ctl->boot_pc & 0x3ff) << CARUS_CTL_OP_CTL_BOOT_PC_OFFSET);

    // Set instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Get current parameters
int carus_get_cfg(const uint8_t inst, carus_cfg_t *cfg)
{
    const uint32_t *koffs = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_KERNEL_REG_ADDR);
    const uint32_t *scratch = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_SCRATCH_REG_ADDR);
    const uint32_t *vl = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_VL_REG_ADDR);
    const uint32_t *vtype = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_VTYPE_REG_ADDR);
    const uint32_t *arg0_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG0_REG_ADDR);
    const uint32_t *arg1_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG1_REG_ADDR);
    const uint32_t *arg2_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG2_REG_ADDR);
    const uint32_t *arg3_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG3_REG_ADDR);

    // Set instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Read parameters
    cfg->koffs = *koffs - CARUS_EMEM_KERNEL_BASE_ADDR;
    cfg->scratch = *scratch;
    cfg->vl = *vl;
    cfg->vtype = *vtype;
    cfg->arg0 = *arg0_ptr;
    cfg->arg1 = *arg1_ptr;
    cfg->arg2 = *arg2_ptr;
    cfg->arg3 = *arg3_ptr;

    // Set instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Set parameters
int carus_set_cfg(const uint8_t inst, const carus_cfg_t *cfg)
{
    uint32_t *koffs = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_KERNEL_REG_ADDR);
    uint32_t *scratch = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_SCRATCH_REG_ADDR);
    uint32_t *vl = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_VL_REG_ADDR);
    uint32_t *vtype = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_VTYPE_REG_ADDR);
    uint32_t *arg0_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG0_REG_ADDR);
    uint32_t *arg1_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG1_REG_ADDR);
    uint32_t *arg2_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG2_REG_ADDR);
    uint32_t *arg3_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_ARG3_REG_ADDR);

    // Set instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Write parameters
    *koffs = cfg->koffs + CARUS_EMEM_KERNEL_BASE_ADDR;
    *scratch = cfg->scratch;
    *vl = cfg->vl;
    *vtype = cfg->vtype;
    *arg0_ptr = cfg->arg0;
    *arg1_ptr = cfg->arg1;
    *arg2_ptr = cfg->arg2;
    *arg3_ptr = cfg->arg3;

    // Set instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Initialize NM-Carus instance
int carus_init(const uint8_t inst)
{
    carus_ctl_t ctl = CARUS_CTL_INIT;
    carus_cfg_t cfg = CARUS_CFG_INIT;
    uint32_t *emem_ptr = (uint32_t *)(CARUS0_START_ADDRESS + inst * CARUS0_SIZE + CARUS_EMEM_BASE_ADDR);
    uint32_t emem_image[CARUS_EMEM_SIZE >> 2] = {0};

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Zero-pad the startup code to obtain the initial eMEM image
    // NOTE: initializing the eMEM to zero prevents the eCPU from fetching
    // invalid data (e.g., after a jump located at the very end of a kernel).
    memcpy(emem_image, carus_loader, CARUS_LOADER_SIZE);

    // Initialize the DMA
    dma_init(NULL);

    // Reset control register
    if (carus_set_ctl(inst, &ctl) != 0)
        return -1;

    // Load the initial eMEM image with the startup code
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;
    dma_copy_32b(emem_ptr, emem_image, CARUS_EMEM_SIZE >> 2);
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    // Reset configuration registers
    if (carus_set_cfg(inst, &cfg) != 0)
        return -1;

    // Enable eCPU instruction fetch
    ctl.fetch_en = 1;
    if (carus_set_ctl(inst, &ctl) != 0)
        return -1;

    return 0;
}

// Load a vector kernel into the specified NM-Carus instance
int carus_load_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size, const uint32_t offs)
{
    uint32_t *emem_ptr = (uint32_t *)(CARUS0_START_ADDRESS + inst * CARUS0_SIZE + CARUS_EMEM_KERNEL_BASE_ADDR + offs);

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Check kernel size
    if (size > (CARUS_EMEM_SIZE - CARUS_PARAMS_SIZE - 1))
        return -1;

    // Initialize the DMA
    dma_init(NULL);

    // Set the target instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Load the kernel, @DAVIDE: supposed size was byte size
    dma_copy_32b(emem_ptr, kernel, size >> 2);

    // Set the target instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Run the current vector kernel
int carus_run_kernel(const uint8_t inst)
{
    volatile uint32_t *op_ctl_ptr = (uint32_t *)(inst * CARUS0_SIZE + CARUS0_START_ADDRESS + CTL_REG_OP_CTL_REG_ADDR);

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Enter configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Set the start bit
    *op_ctl_ptr |= (1 << CARUS_CTL_OP_CTL_START_BIT) |
                   (1 << CARUS_CTL_OP_CTL_CPU_FETCH_EN_BIT);

    // Exit configuration mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Wait for the current kernel to complete
int carus_wait_done(const uint8_t inst)
{
    int ret_val; // return value

    // Wait for the kernel to complete
    while (carus_irq_flag == 0 || carus_irq_idx != inst)
    {
        wait_for_interrupt();
    }
    ret_val = carus_irq_flag;
    carus_irq_flag = 0; // reset Carus interrupt flag

    if (ret_val < 0)
        return -1;
    return 0;
}

// Load and execute a vector kernel into the specified NM-Carus instance
int carus_exec_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size)
{
    // Load the kernel
    if (carus_load_kernel(inst, kernel, size, NULL) != 0)
        return -1;

    // Start the kernel and wait completion
    if (carus_run_kernel(inst) != 0)
        return -1;
    if (carus_wait_done(inst) != 0)
        return -1;

    return 0;
}

// Systemlevel vload.vv
int carus_copy_vector_to_vector(const uint8_t inst, const uint32_t *src, const uint32_t *tgt, const uint32_t size)
{
    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Check vector size
    if (size > VLEN_MAX)
        return -1;

    // Copy the vector
    dma_copy_16_32(tgt, src, size);

    return 0;
}

// Systemlevel vload.vx
int carus_copy_scalar_to_vector(const uint8_t inst, const uint32_t src, const uint32_t *tgt, const uint32_t size)
{
    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Check vector size
    if (size > VLEN_MAX)
        return -1;

    // Copy the scalar
    dma_fill(tgt, src, size);

    return 0;
}
