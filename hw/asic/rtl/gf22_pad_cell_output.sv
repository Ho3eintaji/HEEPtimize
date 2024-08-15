// Copyright 2024 EPFL.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: gf22_pad_cell_output.sv
// Author: Clement CHONE
// Date: 10/05/2024
// Description: gf22 output pad standard cell

// NOTE: Based on the same cell from HEEPocrates (https://eslgit.epfl.ch/heep/HEEPpocrates)

module gf22_pad_cell_output #(
  parameter int unsigned PADATTR = 16,
  parameter core_v_mini_mcu_pkg::pad_side_e SIDE = core_v_mini_mcu_pkg::TOP
) (
  input logic pad_in_i, // pad input value
  input logic pad_oe_i, // pad output enable
  output logic pad_out_o, // pad output value
  inout logic pad_io, // pad value
  inout logic pwrok, // Core power detection
  inout logic iopwrok, // IO power detection
  inout logic bias_ls, // BIAS for level shifter
  input logic [PADATTR-1:0] pad_attributes_i // pad attributes
);
  // gf22 pad standard cell
  if (SIDE == core_v_mini_mcu_pkg::TOP || SIDE == core_v_mini_mcu_pkg::BOTTOM) begin
    // Top or bottom placement, use the horizontal version of the pad
    // GF22 pad standard cell
    IN22FDX_GPIO18_10M3S40PI_IO_H u_pad_output_h (        
      .TRIEN    (1'b0), 
      .NDIN     (1'b0), 
      .DRV      (2'b00),
      .PAD      (pad_io),
      .PWROK    (pad_attributes_i[0]), 
      .Y        (pad_out_o), 
      .RXEN     (~pad_oe_i),  
      .PDEN     (1'b0),  
      .DATA     (pad_in_i),
      .IOPWROK  (pad_attributes_i[1]), 
      .NDOUT    (), 
      .PUEN     (1'b0), 
      .RETC     (1'b1), 
      .BIAS     (pad_attributes_i[2]), 
      .SMT      (~pad_oe_i), 
      .SLW      (1'b0) 
    );
  end else if (SIDE == core_v_mini_mcu_pkg::LEFT || SIDE == core_v_mini_mcu_pkg::RIGHT) begin
    // Left or right placement, use the vertical version of the pad
    // GF22 pad standard cell
    IN22FDX_GPIO18_10M3S40PI_IO_V u_pad_output_v (      
      .TRIEN    (~pad_oe_i),  // 1'b0
      .NDIN     (1'b0), 
      .DRV      (2'b00),
      .PAD      (pad_io),
      .PWROK    (pad_attributes_i[0]), 
      .Y        (pad_out_o), 
      .RXEN     (~pad_oe_i),  // 1'b0
      .PDEN     (1'b0),  
      .DATA     (pad_in_i),
      .IOPWROK  (pad_attributes_i[1]), 
      .NDOUT    (), 
      .PUEN     (1'b0), 
      .RETC     (1'b1), 
      .BIAS     (pad_attributes_i[2]), 
      .SMT      (~pad_oe_i), 
      .SLW      (1'b0) 
    );
  end

endmodule
