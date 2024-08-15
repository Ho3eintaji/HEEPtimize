// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: cgra_sram_wrapper.sv
// Author: Luigi Giuffrida
// Date: 04/05/2024
// Description: SRAM wrapper for generated memory banks to be used in ASIC implementations

// NOTE: based on the same module from HEEPocrates

module cgra_sram_wrapper #(
    parameter int unsigned NumWords = 32'd1024,  // Number of Words in data array
    parameter int unsigned DataWidth = 32'd32,  // Data signal width
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
    input  logic                 set_retentive_i,
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
          .NumWords (NumWords),  // Number of Words in data array
          .DataWidth(DataWidth)  // Data signal width
      ) cgra_mem_i (
        .clk_i          (clk_i),
        .rst_ni         (rst_ni),
        // input ports
        .req_i          (req_i),
        .we_i           (we_i),
        .addr_i         (addr_i),
        .wdata_i        (wdata_i),
        .be_i           (be_i),
        .set_retentive_ni(~set_retentive_i),
        // output ports
        .rdata_o        (rdata_o)
        
      );

endmodule