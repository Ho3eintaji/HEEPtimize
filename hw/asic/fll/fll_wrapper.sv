// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: fll_wrapper.sv
// Author: Michele Caon
// Date: 14/06/2023
// Description: Simulation wrapper for the FLL

module fll_wrapper (
  // Clock signals
  output logic clk_o,      // generated clock
  input  logic oe_i,       // output enable
  input  logic ref_clk_i,  // reference clock
  output logic lock_o,     // FLL locked

  // Configuration
  input  logic        req_i,        // FLL request
  output logic        ack_o,        // FLL acknowledge
  input  logic [ 1:0] addr_i,       // address
  input  logic [31:0] wdata_i,      // write data
  output logic [31:0] rdata_o,      // read data
  input  logic        wr_ni,        // write enable
  input  logic        rst_ni,       // reset
  input  logic        pwd_i,
  input  logic        test_mode_i,
  input  logic        shift_en_i,
  input  logic        td_i,
  output logic        tq_o,
  input  logic        jtd_i,
  output logic        jtq_o
);
  // TSMC 65nm FLL
  // gf22_FLL fll_i (
  tsmc65_FLL fll_i (
    .FLLCLK(clk_o),
    .FLLOE (oe_i),
    .REFCLK(ref_clk_i),
    .LOCK  (lock_o),
    .CFGREQ(req_i),
    .CFGACK(ack_o),
    .CFGAD (addr_i),
    .CFGD  (wdata_i),
    .CFGQ  (rdata_o),
    .CFGWEB(wr_ni),
    .RSTB  (rst_ni),
    .PWD   (pwd_i),
    .TM    (test_mode_i),
    .TE    (shift_en_i),
    .TD    (td_i),
    .TQ    (tq_o),
    .JTD   (jtd_i),
    .JTQ   (jtq_o)
  );
endmodule
