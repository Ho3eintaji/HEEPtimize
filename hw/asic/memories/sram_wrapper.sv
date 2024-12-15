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
    // power manager signals that goes to the ASIC macros
    input logic pwrgate_ni,
    output logic pwrgate_ack_no,
    input logic set_retentive_ni,
    // output ports
    output logic [         31:0] rdata_o
);

// TODO: now i consider memories as always active, later fix them when calling this wrapper to pass values

  assign pwrgate_ack_no = pwrgate_ni; // Direct acknowledgment

  // INTERNAL SIGNALS
  // ----------------
  logic [DataWidth-1:0] bit_we;

  // -------------
  // SRAM INSTANCE
  // -------------
  // Byte enable to bit enable conversion
  generate
    for (genvar i = 0; unsigned'(i) < DataWidth / 8; i++) begin : gen_bit_we
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

      128: begin : gen_sram_128_32_512b

        wire [1-1:0] asI;
        wire [4-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[4-1:2], asI, awI[1:0], acI} = addr_i;

        sram128x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      256: begin : gen_sram_256_32_1kb

        wire [1-1:0] asI;
        wire [5-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[5-1:2], asI, awI[1:0], acI} = addr_i;

        sram256x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      512: begin : gen_sram_512_32_2kb
        // check the generated memory for the size of asI, awI, and acI, and how addr_i is assigned to them
        wire [1-1:0] asI;
        wire [6-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[6-1:2], asI, awI[1:0], acI} = addr_i;

        sram512x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      1024: begin : gen_sram_1024_32_4kb
        wire [1-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      2048: begin : gen_sram_2048_32_8kb
        wire [1-1:0] asI;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        sram2048x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      4096: begin : gen_sram_4096_32_16kb
        wire [2-1:0] asI;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        sram4096x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      8192: begin : gen_sram_8192_32_32kb

        wire [3-1:0] asI;
        wire [8-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        sram8192x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      16384: begin : gen_sram_16384_32_64kb

        wire [3-1:0] asI;
        wire [8-1:0] awI;
        wire [3-1:0] acI;
        assign {awI[8-1:2], asI, awI[1:0], acI} = addr_i;

        sram8192x32m8 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

      default:
      begin : gen_sram_1024_32_4kb_2
        wire [1-1:0] asI;
        wire [7-1:0] awI;
        wire [2-1:0] acI;
        assign {awI[7-1:2], asI, awI[1:0], acI} = addr_i;

        sram1024x32m4 mem_bank (
            .CLK        (clk_i),
            .CEN        (~req_i),
            .RDWEN      (~we_i),
            .DEEPSLEEP  (1'b0),   // Always active
            .POWERGATE  (1'b0),   // Always powered
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

  // `elsif TSMC65
  //     generate
  //     case (NumWords)
  //       64: begin: gen_256B_rf
  //         rf64x32m2 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //       128: begin: gen_512B_rf
  //         rf128x32m2 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //       256: begin: gen_1kB_rf
  //         rf256x32m2 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //       2048: begin: gen_4kB_mem 
  //         sram2048x32m8 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //       8192: begin: gen_32kB_mem
  //         sram8192x32m8 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //       default: begin: gen_16kB_mem
  //         sram4096x32m8 mem_bank (
  //           .CENY     (),
  //           .WENY     (),
  //           .AY       (),
  //           .GWENY    (),
  //           .Q        (rdata_o),
  //           .SO       (),
  //           .CLK      (clk_i),
  //           .CEN      (~req_i),
  //           .WEN      (~bit_we),
  //           .A        (addr_i),
  //           .D        (wdata_i),
  //           .EMA      (ReadMargin),
  //           .EMAW     (WriteMargin),
  //           .TEN      (1'b1),             // Test Mode Enable (active low)
  //           .TCEN     (1'b1),
  //           .TWEN     ('0),
  //           .TA       ('0),
  //           .TD       ('0),
  //           .GWEN     (~we_i),
  //           .TGWEN    (1'b1),
  //           .RET1N    (set_retentive_ni),
  //           .SI       (2'b00),
  //           .SE       (1'b0),             // Scan Enable Input (active high)
  //           .DFTRAMBYP(1'b0)
  //         );
  //       end
  //     endcase
  //   endgenerate

  //   // ----------
  //   // ASSERTIONS
  //   // ----------
  //   `ifndef SYNTHESIS
  //   `include "assertions/sram_wrapper_sva.svh"
  //   `endif // SYNTHESIS
  // `else
  //   // Default case if neither GF22 nor TSMC65 is defined
  //   initial begin
  //     $error("Neither GF22 nor TSMC65 is defined! Please define one of them.");
  //   end

  // `endif

endmodule
