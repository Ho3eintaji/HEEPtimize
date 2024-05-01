// Copyright 2024 EPFL and Universidad Complutense de Madrid
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: sram_wrapper.sv
// Author: David Mallasén
// Date: 24/04/2024
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
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low)
          .A        (addr_i),           // Address (A[0] = LSB) 
          .D        (wdata_i),          // Data Input (D[0] = LSB)
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB) 
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB)
          .SI       (2'b00),            // Scan Input (SI[0] = LSB)
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: Do we have to tie this to something?
          .CTLSI    (),                 // Scan Input - scan chains  2 (active high)
          .CTLSO    (),                 // Scan Output - scan chains  2
          .WENSI    (),                 // Scan Input - scan chains 3 & 4
          .WENSO    ()                  // Scan Ouput - scan chains 3 & 4
        );
      end
      128: begin: gen_512B_rf
        rf128x32m2 mem_bank (
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low)
          .A        (addr_i),           // Address (A[0] = LSB) 
          .D        (wdata_i),          // Data Input (D[0] = LSB)
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB) 
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB)
          .SI       (2'b00),            // Scan Input (SI[0] = LSB)
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: Do we have to tie this to something?
          .CTLSI    (),                 // Scan Input - scan chains  2 (active high)
          .CTLSO    (),                 // Scan Output - scan chains  2
          .WENSI    (),                 // Scan Input - scan chains 3 & 4
          .WENSO    ()                  // Scan Ouput - scan chains 3 & 4
        );
      end
      256: begin: gen_1kB_rf
        rf256x32m2 mem_bank (
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low)
          .A        (addr_i),           // Address (A[0] = LSB) 
          .D        (wdata_i),          // Data Input (D[0] = LSB)
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB) 
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB)
          .SI       (2'b00),            // Scan Input (SI[0] = LSB)
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: Do we have to tie this to something?
          .CTLSI    (),                 // Scan Input - scan chains  2 (active high)
          .CTLSO    (),                 // Scan Output - scan chains  2
          .WENSI    (),                 // Scan Input - scan chains 3 & 4
          .WENSO    ()                  // Scan Ouput - scan chains 3 & 4
        );
      end
      4096: begin: gen_16kB_mem
        sram4096x32m8 mem_bank (
          .CENY     (),                 // Chip Enable Mux Output
          .GWENY    (),                 // Global Write Enable Mux Outputs
          .AY       (),                 // Address Mux Output (AY[0] = LSB)
          .WENY     (),                 // Write Enable Mux Output (WENY[0] = LSB)
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low) 
          .A        (addr_i),           // Address (A[0] = LSB)
          .D        (wdata_i),          // Data Input (D[0] = LSB
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB)
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB) 
          .SI       (2'b00),            // Scan Input (SI[0] = LSB) 
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: The column redundancy enable and address CRE1,CRE2,FCA1,FCA2 will replace defective columns in the memory.
          .CRE1     (),                 // Column Redundancy Enable FCA1 (active high)
          .CRE2     (),                 // Column Redundancy Enable FCA2 (active high)
          .FCA1     (),                 // Faulty Column Address
          .FCA2     ()                  // Faulty Column Address
        );
      end
      8192: begin: gen_32kB_mem
        sram8192x32m8 mem_bank (
          .CENY     (),                 // Chip Enable Mux Output
          .GWENY    (),                 // Global Write Enable Mux Outputs
          .AY       (),                 // Address Mux Output (AY[0] = LSB)
          .WENY     (),                 // Write Enable Mux Output (WENY[0] = LSB)
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low) 
          .A        (addr_i),           // Address (A[0] = LSB)
          .D        (wdata_i),          // Data Input (D[0] = LSB
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB)
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB) 
          .SI       (2'b00),            // Scan Input (SI[0] = LSB) 
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: The column redundancy enable and address CRE1,CRE2,FCA1,FCA2 will replace defective columns in the memory.
          .CRE1     (),                 // Column Redundancy Enable FCA1 (active high)
          .CRE2     (),                 // Column Redundancy Enable FCA2 (active high)
          .FCA1     (),                 // Faulty Column Address
          .FCA2     ()                  // Faulty Column Address
        );
      end
      default: begin: gen_8kB_mem // 2048
        sram2048x32m8 mem_bank (
          .CENY     (),                 // Chip Enable Mux Output
          .GWENY    (),                 // Global Write Enable Mux Outputs
          .AY       (),                 // Address Mux Output (AY[0] = LSB)
          .WENY     (),                 // Write Enable Mux Output (WENY[0] = LSB)
          .Q        (rdata_o),          // Data Output (Q[0] = LSB)
          .SO       (),                 // Scan Output (SO[0] = LSB)
          .CLK      (clk_i),            // Clock
          .CEN      (~req_i),           // Chip Enable (active low)
          .GWEN     (~we_i),            // Global Write Enable (active low) 
          .A        (addr_i),           // Address (A[0] = LSB)
          .D        (wdata_i),          // Data Input (D[0] = LSB
          .WEN      (~bit_we),          // Write Enable (active low, WEN[0] = LSB)
          // TODO: Check if this is the correct value for STOV. For now it is just disabled
          .STOV     (1'b0),             // Self time overrides (active high)
          // TODO: Extra Margin Adjustment pins provide the option of adding delays into internal timing pulses. There are 3 different EMA pins: EMA, EMAW, EMAS to control Read/Write internal timing pulses.
          // In 65nm the EMAS signal didn't exist, and the other had the same names and descriptions.
          .EMA      (ReadMargin),       // Extra Margin Adjustment (EMA[0] = LSB)
          .EMAW     (WriteMargin),      // Write Extra Margin Adjustment (EMAW[0] = LSB)
          .EMAS     (),                 // Read Extra Margin Adjustment
          .TEN      (1'b1),             // Test Mode Enable (active low)
          .TCEN     (1'b1),             // Chip Enable Test Input (active low)
          .TGWEN    (1'b1),             // Global Write Enable Test Input (active low)
          .TA       ('0),               // Address Test Input (TA[0] = LSB)
          .TD       ('0),               // Data Test Input (TD[0] = LSB) 
          .TWEN     ('0),               // Write Enable Test Input (active low, TWEN[0] = LSB) 
          .SI       (2'b00),            // Scan Input (SI[0] = LSB) 
          .SE       (1'b0),             // Scan Enable Input (active high)
          .DFTRAMBYP(1'b0),             // Test Control Input (active high)
          .RET1N    (set_retentive_ni), // Retention Input (active low)
          // TODO: The column redundancy enable and address CRE1,CRE2,FCA1,FCA2 will replace defective columns in the memory.
          .CRE1     (),                 // Column Redundancy Enable FCA1 (active high)
          .CRE2     (),                 // Column Redundancy Enable FCA2 (active high)
          .FCA1     (),                 // Faulty Column Address
          .FCA2     ()                  // Faulty Column Address
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
