// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_clk_gate_wrapper.sv
// Author: Luigi Giuffrida
// Date: 06/05/2024
// Description: Clock gating cell to be used in 16nm implementations of carus

module cgra_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic test_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i,
      .clk_o
  );

endmodule
