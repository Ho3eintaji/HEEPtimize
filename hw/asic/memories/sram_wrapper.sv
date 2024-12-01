// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: sram_wrapper.sv
// Author: Hossein Taji
// Date: 14/08/2024
// Description: SRAM wrapper for generated memory banks

// NOTE: based on the same module from HEEPocrates

// `define GF22
// or
// `define TSMC65

module sram_wrapper #(
  parameter int unsigned NumWords = 32'd8192,  // Number of Words in data array
  parameter int unsigned DataWidth = 32'd32,  // Data signal width
  // DEPENDENT PARAMETERS, DO NOT OVERWRITE!
  parameter int unsigned AddrWidth = (NumWords > 32'd1) ? unsigned'($clog2(NumWords)) : 32'd1,
  // Compiler will be defined as follows : 0 (default) : S1DU, 1 : R1PU, 2 : R1PL, 3 : S1DU for sizes >= 2048 and R1PU else, 4 : S1DU and R1PL.
  parameter int unsigned Compiler = 32'd0
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

// `ifdef GF22
    generate
    case (NumWords)

      128: begin
        
        // S1DU
        if(Compiler == 0) begin : gen_S1DU_sram_128_32_512b
        wire [1-1:0] asI;
        wire [4-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[4-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram128x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU
        if((Compiler == 1) || (Compiler == 3)) begin : gen_R1PU_sram_128_32_512b
        wire [5-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[5-1:0], acI} = addr_i;

        R1PU_sram128x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PL
        if((Compiler == 2) || (Compiler == 4)) begin : gen_R1PL_sram_128_32_512b
        wire [5-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[5-1:0], acI} = addr_i;

        R1PL_sram128x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end
      
      end

      256: begin
        
        // S1DU
        if(Compiler == 0) begin : gen_S1DU_sram_256_32_1kb
        wire [1-1:0] asI;
        wire [5-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[5-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram256x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU
        if((Compiler == 1) || (Compiler == 3)) begin : gen_R1PU_sram_256_32_1kb
        wire [6-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[6-1:0], acI} = addr_i;

        R1PU_sram256x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PL
        if((Compiler == 2) || (Compiler == 4)) begin : gen_R1PL_sram_256_32_1kb
        wire [6-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[6-1:0], acI} = addr_i;

        R1PL_sram256x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end
      
      end

      512: begin
        
        // S1DU
        if(Compiler == 0) begin : gen_S1DU_sram_512_32_2kb
        wire [1-1:0] asI;
        wire [6-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[6-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram512x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU
        if((Compiler == 1) || (Compiler == 3)) begin : gen_R1PU_sram_512_32_2kb
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:0], acI} = addr_i;

        R1PU_sram512x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PL
        if((Compiler == 2) || (Compiler == 4)) begin : gen_R1PL_sram_512_32_2kb
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:0], acI} = addr_i;

        R1PL_sram512x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end
      
      end

      1024: begin
        
        // S1DU
        if(Compiler == 0) begin : gen_S1DU_sram_1024_32_4kb
        wire [1-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram1024x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU
        if((Compiler == 1) || (Compiler == 3)) begin : gen_R1PU_sram_1024_32_4kb
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:0], acI} = addr_i;

        R1PU_sram1024x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PL
        if((Compiler == 2) || (Compiler == 4)) begin : gen_R1PL_sram_1024_32_4kb
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:0], acI} = addr_i;

        R1PL_sram1024x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end
      
      end

      2048: begin
        
        // S1DU
        if((Compiler == 0) || (Compiler == 3) || (Compiler == 4)) begin : gen_S1DU_sram_2048_32_8kb
        wire [2-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram2048x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU : Tiling MSB will decide which bank is used 
        // Depending on MSB CEN is enabled or not
        if((Compiler == 1)) begin : gen_R1PU_sram_2048_32_8kb
        wire bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        R1PU_sram1024x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~(req_i & bank_chosen)),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PU_sram1024x32m4 mem_bank_R1PU_1 (
          .CLK        (clk_i),
          .CEN        (~(req_i & ~bank_chosen)),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PL : same tiling as R1PU
        if((Compiler == 2)) begin : gen_R1PL_sram_2048_32_8kb
        wire bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        R1PL_sram1024x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~(req_i & bank_chosen)),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PL_sram1024x32m4 mem_bank_R1PL_1 (
          .CLK        (clk_i),
          .CEN        (~(req_i & ~bank_chosen)),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end
      
      end

      4096: begin
        
        // S1DU
        if((Compiler == 0) || (Compiler == 3) || (Compiler == 4)) begin : gen_S1DU_sram_4096_32_16kb
        wire [3-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram4096x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU : Tiling 2 MSB will decide which bank is used 
        // Depending on MSB CEN is enabled or not
        if((Compiler == 1)) begin : gen_R1PU_sram_4096_32_16kb
        wire [1:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        R1PU_sram1024x32m4 mem_bank_R1PU_0 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b00))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PU_sram1024x32m4 mem_bank_R1PU_1 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b01))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PU_sram1024x32m4 mem_bank_R1PU_2 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b10))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PU_sram1024x32m4 mem_bank_R1PU_3 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b11))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        ); 
        end

        // R1PL : same tiling as R1PU
        if((Compiler == 2)) begin : gen_R1PL_sram_4096_32_16kb
        wire [1:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        R1PL_sram1024x32m4 mem_bank_R1PL_0 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b00))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PL_sram1024x32m4 mem_bank_R1PL_1 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b01))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PL_sram1024x32m4 mem_bank_R1PL_2 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b10))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );

        R1PL_sram1024x32m4 mem_bank_R1PL_3 (
          .CLK        (clk_i),
          .CEN        (~(req_i & (bank_chosen == 2'b11))),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_LOGIC    ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        ); 
        end
      
      end

      8192: begin
        
        // S1DU
        if((Compiler == 0) || (Compiler == 3) || (Compiler == 4)) begin : gen_S1DU_sram_8192_32_32kb
        wire [3-1:0] asI;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram8192x32m4 mem_bank_S1DU_0 (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU : Tiling 3 MSB will decide which bank is used 
        // Depending on MSB CEN is enabled or not
        if((Compiler == 1)) begin : gen_R1PU_sram_8192_32_32kb
        wire [2:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        genvar i;
          for(i=0; i < (1 << 3); i++) begin : sub_mem_bank
            R1PU_sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~(req_i & (bank_chosen == i))),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),
            .POWERGATE  (~set_retentive_ni),
            .AW         (awI),
            .AC         (acI),
            .D          (wdata_i),
            .BW         (bit_we),
            .T_LOGIC    ('0),
            .MA_SAWL    ('0),
            .MA_WL      ('0),
            .MA_WRAS    ('0),
            .MA_WRASD   ('0),
            .Q          (rdata_o),
            .OBSV_CTL   ()
          );
        end
      
        end

        // R1PL : Tiling 3 MSB will decide which bank is used 
        // Depending on MSB CEN is enabled or not
        if((Compiler == 2)) begin : gen_R1PL_sram_8192_32_32kb
        wire [2:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        genvar i;
          for(i=0; i < (1 << 3); i++) begin : sub_mem_bank
            R1PL_sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~(req_i & (bank_chosen == i))),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),
            .POWERGATE  (~set_retentive_ni),
            .AW         (awI),
            .AC         (acI),
            .D          (wdata_i),
            .BW         (bit_we),
            .T_LOGIC    ('0),
            .MA_SAWL    ('0),
            .MA_WL      ('0),
            .MA_WRAS    ('0),
            .MA_WRASD   ('0),
            .Q          (rdata_o),
            .OBSV_CTL   ()
          );
        end
      
        end

      end

      16384: begin
        
        // S1DU
        if((Compiler == 0) || (Compiler == 3) || (Compiler == 4)) begin : gen_S1DU_sram_16384_32_64kb
        wire [3-1:0] asI;
        wire [8-1:0] awI;
        wire [3-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram16384x32m4 mem_bank (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
        end

        // R1PU : Tiling 4 MSB will decide which bank is used 
        // Depending on MSB CEN is enabled or not
        if((Compiler == 1)) begin : gen_R1PU_sram_16384_32_64kb
        wire [3:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        genvar i;
          for(i=0; i < (1 << 4); i++) begin : sub_mem_bank
            R1PU_sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~(req_i & (bank_chosen == i))),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),
            .POWERGATE  (~set_retentive_ni),
            .AW         (awI),
            .AC         (acI),
            .D          (wdata_i),
            .BW         (bit_we),
            .T_LOGIC    ('0),
            .MA_SAWL    ('0),
            .MA_WL      ('0),
            .MA_WRAS    ('0),
            .MA_WRASD   ('0),
            .Q          (rdata_o),
            .OBSV_CTL   ()
          );
        end
      
        end

        if((Compiler == 2)) begin : gen_R1PL_sram_16384_32_64kb
        wire [3:0] bank_chosen;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {bank_chosen,awI[8-1:0], acI} = addr_i;

        genvar i;
          for(i=0; i < (1 << 4); i++) begin : sub_mem_bank
            R1PL_sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~(req_i & (bank_chosen == i))),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),
            .POWERGATE  (~set_retentive_ni),
            .AW         (awI),
            .AC         (acI),
            .D          (wdata_i),
            .BW         (bit_we),
            .T_LOGIC    ('0),
            .MA_SAWL    ('0),
            .MA_WL      ('0),
            .MA_WRAS    ('0),
            .MA_WRASD   ('0),
            .Q          (rdata_o),
            .OBSV_CTL   ()
          );
        end
      
        end

      end      

      default: begin : gen_S1DU_sram_1024_32_4kb_2
        wire [1-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        S1DU_sram1024x32m4 mem_bank (
          .CLK        (clk_i),
          .CEN        (~req_i),
          .RDWEN      (~we_i),
          .DEEPSLEEP  (1'b0),
          .POWERGATE  (~set_retentive_ni),
          .AS         (asI),
          .AW         (awI),
          .AC         (acI),
          .D          (wdata_i),
          .BW         (bit_we),
          .T_BIST     ('0),
          .T_LOGIC    ('0),
          .T_CEN      ('0),
          .T_RDWEN    ('0),
          .T_DEEPSLEEP('0),
          .T_POWERGATE('0),
          .T_AS       ('0),
          .T_AW       ('0),
          .T_AC       ('0),
          .T_D        ('0),
          .T_BW       ('0),
          .T_WBT      ('0),
          .T_STAB     ('0),
          .MA_SAWL    ('0),
          .MA_WL      ('0),
          .MA_WRAS    ('0),
          .MA_WRASD   ('0),
          .Q          (rdata_o),
          .OBSV_CTL   ()
        );
      end      
        
    endcase
  endgenerate

endmodule
