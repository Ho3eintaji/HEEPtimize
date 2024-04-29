// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_components.cpp
// Author: Michele Caon
// Date: 06/07/2023
// Description: Implementation of testbench components

#include <verilated.h>

#include "tb_components.hh"
#include "tb_macros.hh"

enum carus_cfg_field {
    CARUS_CFG_START,
    CARUS_CFG_DONE,
    CARUS_CFG_FETCH_EN,
    CARUS_CFG_BOOT_PC,
};

Monitor::Monitor(Vtb_system *dut) {
    this->dut = dut;
    this->carus_exec_time_en = false;
    this->carus_kernel_start = 0;
    this->carus_kernel_end = 0;
}

Monitor::~Monitor() {
}

void Monitor::monitor() {
    static vluint8_t carus_prev_start = 0;
    static vluint8_t carus_prev_done = 0;
    vluint8_t carus_start = this->dut->tb_get_carus_cfg(CARUS_CFG_START);
    vluint8_t carus_done = this->dut->tb_get_carus_cfg(CARUS_CFG_DONE);
    // NM-Carus execution timer
    if (this->carus_exec_time_en) {
        // Set start time
        if (carus_start && !carus_prev_start) {
            this->carus_kernel_start = this->dut->contextp()->time();
            TB_LOG(LOG_HIGH, "Carus kernel start: %lu", this->carus_kernel_start);
        }
        // Set end time
        else if (carus_done && !carus_prev_done) {
            this->carus_kernel_end = this->dut->contextp()->time();
            TB_LOG(LOG_HIGH, "Carus kernel end: %lu", this->carus_kernel_end);
        }
        
        carus_prev_start = carus_start;
        carus_prev_done = carus_done;
    }
}

void Monitor::carusExecTimeEn(bool en) {
    this->carus_exec_time_en = en;
}


vluint64_t Monitor::carusGetCycles() {
    // Divide by 2 to get the number of cycles
    return (this->carus_kernel_end - this->carus_kernel_start) >> 1;
}

