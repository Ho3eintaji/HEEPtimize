// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: fll_wrapper.sv
// Author: Michele Caon
// Date: 14/06/2023
// Description: Simulation wrapper for the FLL

// NOTE: this is a dummy implementation for simulation purposes only.
// DO NOT USE FOR REAL IMPLEMENTATION.

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
  // INTERNAL SIGNALS
  // ----------------
  logic       gen_clk;  // generated clock
  logic       div_clk;  // divided clock
  logic [1:0] clk_cnt;  // clock counter
  logic       clk_cnt_tc;  // clock counter terminal count
  logic [1:0] clk_div_q;  // clock divider
  logic       clk_sel;  // clock selector

  // --------------
  // INTERNAL LOGIC
  // --------------

  // Clock divider register
  // ----------------------
  always_ff @(posedge ref_clk_i or negedge rst_ni) begin : clk_div_reg
    if (!rst_ni) begin
      clk_div_q <= 2'b0;
    end else if (req_i && !wr_ni && addr_i == 2'b00) begin
      clk_div_q <= wdata_i[1:0];
    end
  end

  // Clock generator
  // ---------------
  // Reference clock counter
  always_ff @(posedge ref_clk_i or negedge rst_ni) begin : clk_div_cnt
    if (!rst_ni) begin
      clk_cnt <= 2'b00;
    end else if (clk_cnt_tc) begin
      clk_cnt <= 2'b00;
    end else begin
      clk_cnt <= clk_cnt + 2'b01;
    end
  end
  assign clk_cnt_tc = clk_cnt == clk_div_q;

  // Generated clock flip-flop
  always_ff @(posedge ref_clk_i or negedge rst_ni) begin : gen_clk_ff
    if (!rst_ni) div_clk <= 1'b0;
    else if (clk_cnt_tc) div_clk <= ~div_clk;
  end

  // Generated clock mux
  assign clk_sel = clk_div_q != 2'b00;
  tc_clk_mux2 u_clk_mux (
      .clk0_i   (ref_clk_i),
      .clk1_i   (div_clk),
      .clk_sel_i(clk_sel),
      .clk_o    (gen_clk)
  );

  // --------------
  // OUTPUT NETWORK
  // --------------
  // Output clock gate
  fll_clk_gate_wrapper u_clk_gate (
      .clk_i       (gen_clk),
      .en_i        (oe_i),
      .scan_cg_en_i(1'b0),
      .clk_o       (clk_o)
  );
  assign lock_o  = 1'b1;

  // Other outputs
  assign ack_o   = 1'b1;
  assign rdata_o = {30'h0, clk_div_q};
  assign tq_o    = 1'b0;
  assign jtq_o   = 1'b0;
endmodule
