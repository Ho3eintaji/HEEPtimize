// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_clk_gate_wrapper.sv
// Author: Michele Caon
// Date: 15/05/2023
// Description: Simulation model for clock gating cells

// Based on cv32e40x_sim_clock_gate from OpenHW Group.

// FOR SIMULATION ONLY. DO NOT USE FOR IMPLEMENTATION

module fll_clk_gate_wrapper (
  input  logic clk_i,
  input  logic en_i,
  input  logic scan_cg_en_i,
  output logic clk_o
);
  // INTERNAL SIGNALS
  logic clk_en;

  always_latch begin : clk_gate
    if (clk_i == 1'b0) clk_en = en_i | scan_cg_en_i;
  end

  assign clk_o = clk_i & clk_en;

endmodule
