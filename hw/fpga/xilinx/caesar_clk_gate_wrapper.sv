// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: caesar_clk_gate_wrapper.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 15/05/2023
// Description: Clock gating cell to be used in XILINX FPGAs implementations

module caesar_clk_gate_wrapper (
    input logic clk_i,
    input logic en_i,
    input logic scan_cg_en_i,
    output logic clk_o
);
    // INTERNAL SIGNALS
    logic clk_en;

    always_latch begin : clk_gate
        if (clk_i == 1'b0) clk_en <= en_i | scan_cg_en_i;
    end

    assign clk_o = clk_i & clk_en;

endmodule
