// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heeperator_ctrl_reg.sv
// Author: Michele Caon
// Date: 15/05/2023
// Description: HEEPerator control registers

module heeperator_ctrl_reg #(
  // Dependent parameters: do not override!
  localparam int unsigned CaesarNumRnd = (heeperator_pkg::CaesarNum > 32'd1) ? heeperator_pkg::CaesarNum : 32'd1,
  localparam int unsigned CarusNumRnd = (heeperator_pkg::CarusNum > 32'd1) ? heeperator_pkg::CarusNum : 32'd1
) (
  input logic clk_i,
  input logic rst_ni,

  // Bus interface
  input reg_pkg::reg_req_t req_i,
  output reg_pkg::reg_rsp_t rsp_o,

  // Hardware interface
  output logic [CaesarNumRnd-1:0] caesar_imc_o,
  output logic [CarusNumRnd-1:0] carus_imc_o
);
  import reg_pkg::*;

  // INTERNAL SIGNALS
  // ----------------
  // Registers <--> hardware
  heeperator_ctrl_reg_pkg::heeperator_ctrl_reg2hw_t reg2hw;

  // --------------
  // OUTPUT CONTROL
  // --------------

  // To near-memory computing IPs
  assign caesar_imc_o = {
% for inst in reversed(range(1, caesar_num)):
    reg2hw.op_mode.caesar_imc_${inst}.q,
% endfor
    reg2hw.op_mode.caesar_imc_0.q
  };
  assign carus_imc_o = {
% for inst in reversed(range(1, carus_num)):
    reg2hw.op_mode.carus_imc_${inst}.q,
% endfor
    reg2hw.op_mode.carus_imc_0.q
  };

  // ----------
  // COMPONENTS
  // ----------
  // Control and status registers
  heeperator_ctrl_reg_top #(
    .reg_req_t (reg_req_t),
    .reg_rsp_t (reg_rsp_t)
  ) u_heeperator_ctrl_reg_top (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .reg_req_i (req_i),
    .reg_rsp_o (rsp_o),
    .reg2hw    (reg2hw),
    .devmode_i (1'b0)
  );
endmodule
