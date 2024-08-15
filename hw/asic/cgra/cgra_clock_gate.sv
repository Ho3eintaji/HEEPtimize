// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_clk_gate_wrapper.sv
// Author: Hossein Taji
// Date: 14/08/2024
// Description: Clock gating cell to be used in TSMC65 or GF22 implementations of carus

// Define the technology flag
`define GF22
// or
// `define TSMC65

module cgra_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic test_en_i,
    output logic clk_o
);

`ifdef GF22
  // GF22 clock gating standard cell
  gf22_clk_gating clk_gate_i (
      .clk_i(clk_i),
      .en_i(en_i),
      .test_en_i(scan_cg_en_i),
      .clk_o(clk_o)
  );
`elsif TSMC65
  // TSMC65 clock gating standard cell
  tsmc65_clk_gating clk_gate_i (
      .clk_i(clk_i),
      .en_i(en_i),
      .test_en_i(scan_cg_en_i),
      .clk_o(clk_o)
  );
`else
  // Default case if neither GF22 nor TSMC65 is defined
  initial begin
    $error("Neither GF22 nor TSMC65 is defined! Please define one of them.");
  end

`endif

endmodule
