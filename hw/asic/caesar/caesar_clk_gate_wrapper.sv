// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: caesar_clk_gate_wrapper.sv
// Author: Hossein Taji
// Date: 06/05/2024
// Description: Clock gating cell to be used in 65nm implementations of caesar

module caesar_clk_gate_wrapper (
    input  logic clk_i,
    input  logic en_i,
    input  logic scan_cg_en_i,
    output logic clk_o
);
  tsmc65_clk_gating tsmc65_clk_gating_i (
      .clk_i,
      .en_i,
      .test_en_i(scan_cg_en_i),
      .clk_o
  );

endmodule