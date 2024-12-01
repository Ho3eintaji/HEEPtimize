// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_sram_wrapper.sv
// Author: Hossein Taji
// Date: 26/06/2024
// Description: SRAM wrapper for generated memory banks to be used in ASIC implementations

// NOTE: based on the same module from HEEPocrates

module carus_sram_wrapper #(
    parameter int unsigned NUM_WORDS = 32'd1024,  // Number of Words in data array
    parameter int unsigned DATA_WIDTH = 32'd32,  // Data signal width
    // DEPENDENT PARAMETERS, DO NOT OVERWRITE!
    localparam int unsigned AddrWidth = (NUM_WORDS > 32'd1) ? unsigned'($clog2(NUM_WORDS)) : 32'd1
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
        .DataWidth(DATA_WIDTH),  // Data signal width
        .Compiler (2) // Compiler used (0 default) S1DU, (1) R1PU, (2) R1PL, (3) S1DU and R1PU, (4) S1DU and R1PL
    ) carus_mem_i (
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