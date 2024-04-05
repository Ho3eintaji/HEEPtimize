// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_components.hh
// Author: Michele Caon
// Date: 06/07/2023
// Description: Header file for testbench components

#ifndef TB_COMPONENTS_HH_
#define TB_COMPONENTS_HH_

#include <verilated.h>

#include "Vtb_system.h"

// Monitor
class Monitor {
private:
    Vtb_system *dut;
    bool carus_exec_time_en;
    vluint64_t carus_kernel_start;
    vluint64_t carus_kernel_end;
    bool caesar_exec_time_en;
    vluint64_t caesar_kernel_start;
    vluint64_t caesar_kernel_end;

public:
    Monitor(Vtb_system *dut);
    ~Monitor();

    // Monitor DUT signals
    void monitor();
    
    // NM-Carus execution time measurement
    void carusExecTimeEn(bool en);
    vluint64_t carusGetCycles();

    // NM-Caesar execution time measurement
    void caesarExecTimeEn(bool en);
    vluint64_t caesarGetCycles();
};

#endif // TB_COMPONENTS_HH_
