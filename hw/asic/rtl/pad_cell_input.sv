// Copyright 2024 EPFL.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: pad_cell_input.sv
// Author: Clement CHONE
// Date: 10/05/2024
// Description: Inpud pad standard cell

module pad_cell_input #(
  parameter int unsigned PADATTR = 16,
  parameter core_v_mini_mcu_pkg::pad_side_e SIDE = core_v_mini_mcu_pkg::TOP
) (
  input logic pad_in_i, // pad input value
  input logic pad_oe_i, // pad output enable
  output logic pad_out_o, // pad output value
  input logic pad_io, // pad value
  input logic [PADATTR-1:0] pad_attributes_i // pad attributes
);
// TODO: add padding later
  /*
  // gf22 pad standard cell
  gf22_pad_cell_input #(
    .PADATTR (PADATTR )
  ) u_pad_cell_input (
  	.pad_in_i         (pad_in_i         ),
    .pad_oe_i         (pad_oe_i         ),
    .pad_out_o        (pad_out_o        ),
    .pad_io           (pad_io           ),
    .pad_attributes_i (pad_attributes_i )
  );
  */

  // Directly assign the pad_io to pad_out_o
  assign pad_out_o = pad_io;

  
endmodule
