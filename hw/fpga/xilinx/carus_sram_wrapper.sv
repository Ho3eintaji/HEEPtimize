// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: carus_sram_wrapper.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 01/02/2024
// Description: SRAM wrapper for generated memory banks to be used in XILINX FPGAs implementations

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

 generate
  if (NUM_WORDS == 128) begin
    xilinx_emem_gen_carus xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 512) begin
    xilinx_mem_gen_carus_2k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 1024) begin
    xilinx_mem_gen_carus_4k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 2048) begin
    xilinx_mem_gen_carus_8k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 4096) begin
    xilinx_mem_gen_carus_16k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 8192) begin
    xilinx_mem_gen_carus_32k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end else if (NUM_WORDS == 16384) begin
    xilinx_mem_gen_carus_64k xilinx_sram_i (
      .clka (clk_i),
      .ena  (req_i),
      .wea  ({4{req_i & we_i}} & be_i),
      .addra(addr_i),
      .dina (wdata_i),
      // output ports
      .douta(rdata_o)
    );
  end
 endgenerate
  

endmodule
