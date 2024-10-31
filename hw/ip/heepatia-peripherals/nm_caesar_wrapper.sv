// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: nm_caesar_wrapper.sv
// Author: Michele Caon
// Date: 30/05/2023
// Description: OBI wrapper for nm-caesar top-level module

module nm_caesar_wrapper #(
  // Mask a command if received in a cycle where imc_i toggles. When this is
  // not asserted, the system must ensure that the imc_i signal is stable TWO
  // cycles before the first command is issued. Enabling this creates a
  // combinational path between the imc_i input signal and the bus_gnt_o
  // output signal.
  parameter int unsigned REQ_PROXY = 32'd0,  // defined as int for FuseSoC compatibility,
  parameter MEM_NUM_WORDS = 32'd4096,  // 32kB
  parameter MEM_DATA_WIDTH = 32'd32  // 32 bits
) (
  // Clock and reset
  input logic clk_i,
  input logic rst_ni,

  // Memory retentive mode
  input logic set_retentive_ni,

  // Operating mode
  input logic imc_i,

  // OBI bus interface
  input  obi_pkg::obi_req_t  bus_req_i,
  output obi_pkg::obi_resp_t bus_rsp_o
);
  // INTERNAL SIGNALS
  // ----------------
  // Bus to memory interface
  logic        mem_cs;
  logic        mem_ready;
  logic [12:0] mem_addr;
  logic        bus_gnt;
  logic        bus_rvalid;
  logic [31:0] bus_rdata;

  // --------------
  // INTERNAL LOGIC
  // --------------

  // OBI bus to memory bridge
  // ------------------------
  // Address translation
  assign mem_addr = bus_req_i.addr[14:2];  // ignore byte address

  // rvalid flip-flop
  // NOTE: the rvalid signal is asserted one cycle after a request
  // is accepted.
  always_ff @(posedge clk_i or negedge rst_ni) begin : rvalid_ff
    if (!rst_ni) bus_rvalid <= 1'b0;
    else bus_rvalid <= mem_cs & bus_gnt;
  end

  // Request mask logic
  // ------------------
  // NOTE: prevent a request from being accepted (i.e., granted) in the same
  // cycle in which the imc_i signal toggles.
  generate
    if (REQ_PROXY != 32'h0) begin : gen_req_mask
      logic imc_q;

      // imc_i flip-flop
      always_ff @(posedge clk_i or negedge rst_ni) begin : imc_ff
        if (!rst_ni) imc_q <= 1'b0;
        else imc_q <= imc_i;
      end

      // Memory request generation
      assign mem_cs  = ~(imc_q ^ imc_i) & bus_req_i.req;

      // Grant generation
      assign bus_gnt = ~(imc_q ^ imc_i) & mem_ready;
    end else begin : gen_req_wires
      assign mem_cs  = bus_req_i.req;
      assign bus_gnt = mem_ready;
    end
  endgenerate

  // ---------
  // NM-CAESAR
  // ---------
  caesar_top #(
    .MEM_NUM_WORDS (MEM_NUM_WORDS),
    .MEM_DATA_WIDTH(MEM_DATA_WIDTH)
  ) u_caesar_top (
    .clk_i           (clk_i),
    .rst_ni          (rst_ni),
    .set_retentive_ni(set_retentive_ni),
    .imc_i           (imc_i),
    .cs_i            (mem_cs),
    .we_i            (bus_req_i.we),
    .be_i            (bus_req_i.be),
    .addr_i          (mem_addr),
    .wdata_i         (bus_req_i.wdata),
    .ready_o         (mem_ready),
    .rdata_o         (bus_rdata)
  );

  // --------------
  // OUTPUT NETWORK
  // --------------
  assign bus_rsp_o.gnt    = bus_gnt;
  assign bus_rsp_o.rvalid = bus_rvalid;
  assign bus_rsp_o.rdata  = bus_rdata;
endmodule
