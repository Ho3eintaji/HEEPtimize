// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus.c
// Author: Michele Caon, Luigi Giuffrida
// Date: 19/06/2024
// Description: Driver for NM-Carus near-memory computing peripheral.

#include <string.h>

#include "carus.h"
#include "carus_loader.h"
#include "heepatia_ctrl_reg.h"
#include "carus_addr_map.h"
#include "dma_sdk.h"
#include "ext_irq.h"
#include "hart.h"
#include "csr.h"
#include "dma.h"

/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// Carus interrupt flags
volatile int8_t carus_irq_flag = 0;
volatile uint32_t carus_irq_idx = 0;
volatile uint32_t carus_irq_error = 0;


const int32_t carus_vlen[CARUS_NUM] = {
%for inst in range(carus_num):
    (int32_t) CARUS${inst}_VLEN_MAX,
%endfor
};

int32_t carus[CARUS_NUM] = {
%for inst in range(carus_num):
    (int32_t) CARUS${inst}_START_ADDRESS,
%endfor
};

/*************************************/
/* ---- CONFIGURATION FUNCTIONS ---- */
/*************************************/
// Get NM-Carus configuration mode
int __attribute__((noinline)) carus_get_mode(const uint8_t inst, carus_mode_t *mode)
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
int __attribute__((noinline)) carus_set_mode(const uint8_t inst, const carus_mode_t mode)
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
int __attribute__((noinline)) carus_get_ctl(const uint8_t inst, carus_ctl_t *ctl)
{
    const uint32_t *op_ctl_ptr = (uint32_t *)(carus[inst] + CTL_REG_OP_CTL_REG_ADDR);
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
int __attribute__((noinline)) carus_set_ctl(const uint8_t inst, const carus_ctl_t *ctl)
{
    uint32_t *op_ctl = (uint32_t *)(carus[inst] + CTL_REG_OP_CTL_REG_ADDR);

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
int __attribute__((noinline)) carus_get_cfg(const uint8_t inst, carus_cfg_t *cfg)
{
    const uint32_t *koffs = (uint32_t *)(carus[inst] + CTL_REG_KERNEL_REG_ADDR);
    const uint32_t *scratch = (uint32_t *)(carus[inst] + CTL_REG_SCRATCH_REG_ADDR);
    const uint32_t *vl = (uint32_t *)(carus[inst] + CTL_REG_VL_REG_ADDR);
    const uint32_t *vtype = (uint32_t *)(carus[inst] + CTL_REG_VTYPE_REG_ADDR);
    const uint32_t *arg0_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG0_REG_ADDR);
    const uint32_t *arg1_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG1_REG_ADDR);
    const uint32_t *arg2_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG2_REG_ADDR);
    const uint32_t *arg3_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG3_REG_ADDR);

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
int __attribute__((noinline)) carus_set_cfg(const uint8_t inst, const carus_cfg_t *cfg)
{
    uint32_t *koffs = (uint32_t *)(carus[inst] + CTL_REG_KERNEL_REG_ADDR);
    uint32_t *scratch = (uint32_t *)(carus[inst] + CTL_REG_SCRATCH_REG_ADDR);
    uint32_t *vl = (uint32_t *)(carus[inst] + CTL_REG_VL_REG_ADDR);
    uint32_t *vtype = (uint32_t *)(carus[inst] + CTL_REG_VTYPE_REG_ADDR);
    uint32_t *arg0_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG0_REG_ADDR);
    uint32_t *arg1_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG1_REG_ADDR);
    uint32_t *arg2_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG2_REG_ADDR);
    uint32_t *arg3_ptr = (uint32_t *)(carus[inst] + CTL_REG_ARG3_REG_ADDR);

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
int __attribute__((noinline)) carus_init(const uint8_t inst)
{
    carus_ctl_t ctl = CARUS_CTL_INIT;
    carus_cfg_t cfg = CARUS_CFG_INIT(inst);
    uint32_t *emem_ptr = (uint32_t *)(carus[inst] + CARUS_EMEM_BASE_ADDR);
    uint32_t emem_image[CARUS_EMEM_SIZE >> 2] = {0};

    for (int i = 0; i < 32; ++i)
    {
        int32_t *v = (uint32_t *)carus_vrf(inst, i);
        v = (int32_t *)(carus[inst] + i * carus_vlen[inst]);
    }

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Zero-pad the startup code to obtain the initial eMEM image
    // NOTE: initializing the eMEM to zero prevents the eCPU from fetching
    // invalid data (e.g., after a jump located at the very end of a kernel).
    memcpy(emem_image, carus_loader, CARUS_LOADER_SIZE);

    // Initialize the DMA
    dma_sdk_init();

    // Reset control register
    if (carus_set_ctl(inst, &ctl) != 0)
        return -1;

    // Load the initial eMEM image with the startup code
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;
    dma_copy(emem_ptr, emem_image, CARUS_EMEM_SIZE >> 2, inst % DMA_CH_NUM, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);

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
int __attribute__((noinline)) carus_load_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size, const uint32_t offs)
{
    uint32_t *emem_ptr = (uint32_t *)(carus[inst] + CARUS_EMEM_KERNEL_BASE_ADDR + offs);

    // Check instance number
    if (inst > (CARUS_NUM - 1))
        return -1;

    // Check kernel size
    if (size > (CARUS_EMEM_SIZE - CARUS_PARAMS_SIZE - 1))
        return -1;

    // Initialize the DMA
    dma_sdk_init();

    // Set the target instance in configuration mode
    if (carus_set_mode(inst, CARUS_MODE_CFG) != 0)
        return -1;

    // Load the kernel
    dma_copy(emem_ptr, kernel, size >> 2, inst % DMA_CH_NUM, DMA_DATA_TYPE_WORD, DMA_DATA_TYPE_WORD, 0);

    // Set the target instance in memory mode
    if (carus_set_mode(inst, CARUS_MODE_MEM) != 0)
        return -1;

    return 0;
}

// Run the current vector kernel
int __attribute__((noinline)) carus_run_kernel(const uint8_t inst)
{
    volatile uint32_t *op_ctl_ptr = (uint32_t *)(carus[inst] + CTL_REG_OP_CTL_REG_ADDR);

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
int __attribute__((noinline)) carus_wait_done(const uint8_t inst)
{
    int ret_val; // return value

    // Wait for the kernel to complete
    while (carus_irq_flag == 0)
    {
        wait_for_interrupt();
    }
    ret_val = carus_irq_error;
    carus_irq_flag--; // reset Carus interrupt flag

    if (ret_val != 0)
    {
        carus_irq_error = 0;
        return -1;
    }
    return 0;
}

// Load and execute a vector kernel into the specified NM-Carus instance
int __attribute__((noinline)) carus_exec_kernel(const uint8_t inst, const uint32_t *kernel, const uint32_t size)
{
    // Load the kernel
    if (carus_load_kernel(inst, kernel, size, 0) != 0)
        return -1;

    // Start the kernel and wait completion
    if (carus_run_kernel(inst) != 0)
        return -1;
    if (carus_wait_done(inst) != 0)
        return -1;

    return 0;
}

void __attribute__((noinline)) carus_irq(uint32_t id) {
    carus_ctl_t ctl;

    // Stop VCD dump (NM-Carus power simulation)
    vcd_disable();

    // Check interrupt type
    if (id != EXT_INTR_0) {
        return;
    }

    // Check which NM-Carus instance triggered the interrupt
    for (unsigned int i = 0; i < CARUS_NUM; i++) {
        if (carus_get_ctl(i, &ctl) != 0) {
            carus_irq_error = 1;
            return;
        }
        if (ctl.done == 0) continue;
        
        // Register IRQ
        carus_irq_idx = i;
        carus_irq_flag++;
        if (ctl.err != 0) carus_irq_error = 1;
        
        // Clear done bit
        ctl.done = 0;
        if (carus_set_ctl(i, &ctl) != 0) {
            carus_irq_error = 1;
            return;
        }
    }

    return;
}

