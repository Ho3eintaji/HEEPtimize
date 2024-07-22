// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: sram_wrapper.sv
// Author: Michele Caon
// Date: 21/02/2023
// Description: SRAM wrapper for generated memory banks

// NOTE: based on the same module from HEEPocrates

module sram_wrapper #(
  parameter int unsigned NumWords = 32'd2048,  // Number of Words in data array
  parameter int unsigned DataWidth = 32'd32,  // Data signal width
  // DEPENDENT PARAMETERS, DO NOT OVERWRITE!
  parameter int unsigned AddrWidth = (NumWords > 32'd1) ? unsigned'($clog2(NumWords)) : 32'd1
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
  // ----------------
  logic [DataWidth-1:0] bit_we;

  // -------------
  // SRAM INSTANCE
  // -------------
  // Byte enable to bit enable conversion
  generate
    for (genvar i = 0; unsigned'(i) < DataWidth / 8; i++) begin: gen_bit_we
      assign bit_we[i*8+7:i*8] = {8{be_i[i]}};
    end
  endgenerate

  // SRAM instantiation
  // NOTE: timing are charaterized by setting EMA[2:0] and EMAW[1:0]. Default
  // characterization settings:
  // - EMA[2:0]=3'b010
  // - EMAW[1:0]=2'b00
  localparam ReadMargin = 3'b010;
  localparam WriteMargin = 2'b00;

  generate
    case (NumWords)
      64: begin: gen_256B_rf
        rf64x32m2 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
      128: begin: gen_512B_rf
        rf128x32m2 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
      256: begin: gen_1kB_rf
        rf256x32m2 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
      4096: begin: gen_16kB_mem
        sram4096x32m8 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
      8192: begin: gen_32kB_mem
        sram8192x32m8 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
      default: begin: gen_8kB_mem // 2048
        sram2048x32m8 mem_bank (
          .CENY     (),
          .WENY     (),
          .AY       (),
          .GWENY    (),
          .Q        (rdata_o),
          .SO       (),
          .CLK      (clk_i),
          .CEN      (~req_i),
          .WEN      (~bit_we),
          .A        (addr_i),
          .D        (wdata_i),
          .EMA      (ReadMargin),
          .EMAW     (WriteMargin),
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),
          .TWEN     ('0),
          .TA       ('0),
          .TD       ('0),
          .GWEN     (~we_i),
          .TGWEN    (1'b1),
          .RET1N    (set_retentive_ni),
          .SI       (2'b00),
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0)
        );
      end
    endcase
  endgenerate

  // ----------
  // ASSERTIONS
  // ----------
  `ifndef SYNTHESIS
  `include "assertions/sram_wrapper_sva.svh"
  `endif // SYNTHESIS
endmodule
