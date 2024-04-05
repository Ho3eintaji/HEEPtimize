// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tsmc65_pad_cell_inout.sv
// Author: Michele Caon
// Date: 14/05/2023
// Description: TSMC65 input/output pad standard cell

// NOTE: Based on the same cell from HEEPocrates (https://eslgit.epfl.ch/heep/HEEPpocrates)

module tsmc65_pad_cell_inout #(
  parameter int unsigned PADATTR = 16
) (
  input logic pad_in_i, // pad input value
  input logic pad_oe_i, // pad output enable
  output logic pad_out_o, // pad output value
  inout logic pad_io, // pad value
  input logic [PADATTR-1:0] pad_attributes_i // pad attributes
);
  // TSMC65 pad standard cell
  PDUW0204CDG u_pad_inout (
    .I (pad_in_i),
    .OEN (~pad_oe_i),
    .PE (pad_attributes_i[0]),
    .PAD (pad_io),
    .IE (1'b1),
    .DS(1'b1),
    .C (pad_out_o)
  );
endmodule
