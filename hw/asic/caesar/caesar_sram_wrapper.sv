// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: caesar_sram_wrapper.sv
// Author: Hossein Taji
// Date: 14/08/2024
// Description: SRAM wrapper for generated memory banks to be used in ASIC implementations

// NOTE: Inside the SRAM wrapper, it is handled from which technology the SRAM is generated

module caesar_sram_wrapper #(
    parameter int unsigned NUM_WORDS = 32'd1024,  // Number of Words in data array
    parameter int unsigned DATA_WIDTH = 32'd32,  // Data signal width
    // DEPENDENT PARAMETERS, DO NOT OVERWRITE!
    // localparam int unsigned AddrWidth = (NUM_WORDS > 32'd1) ? unsigned'($clog2(NUM_WORDS)) : 32'd1
    // hardcoding the address width for 16KB SRAM which is 12 bits (problem is in caesar it is hardcoded!!)
    localparam int unsigned AddrWidth = 12
) (
    input  logic                 clk_i,
    input  logic                 rst_ni,
    // input ports
    input  logic                 req_i,
    input  logic                 we_i,
    input  logic [AddrWidth-1:0] addr_i,
    input  logic [         31:0] wdata_i,
    input  logic [          3:0] be_i,
    input  logic                 set_retentive_ni,
    // input  logic                  pwrgate_ni,
    // output ports
    output logic [         31:0] rdata_o
);
  // INTERNAL SIGNALS
  logic write_en;
  logic read_en;

  // Read/Write enable generation
  assign write_en = req_i & we_i;
  assign read_en  = req_i & (~we_i);

    sram_wrapper #(
        .NumWords (NUM_WORDS),  // Number of Words in data array
        .DataWidth(DATA_WIDTH)  // Data signal width
    ) caesar_mem_i (
        .clk_i,
        .rst_ni,
        // input ports
        .req_i,
        .we_i,
        .addr_i,
        .wdata_i,
        .be_i,
        .set_retentive_ni,
        // output ports
        .rdata_o
    );

endmodule