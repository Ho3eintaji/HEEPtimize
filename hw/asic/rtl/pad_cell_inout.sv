// Copyright 2024 EPFL.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: pad_cell_inout.sv
// Author: Clement CHONE
// Date: 10/05/2024
// Description: Input/output pad standard cell

module pad_cell_inout #(
  parameter int unsigned PADATTR = 16,
  parameter core_v_mini_mcu_pkg::pad_side_e SIDE = core_v_mini_mcu_pkg::TOP
) (
  input logic pad_in_i, // pad input value
  input logic pad_oe_i, // pad output enable
  output logic pad_out_o, // pad output value
  inout logic pad_io, // pad value
  input logic [PADATTR-1:0] pad_attributes_i // pad attributes
);
  // TODO: add padding later
  /*
  gf22_pad_cell_inout #(
    .PADATTR (PADATTR )
  ) u_pad_cell_inout (
  	.pad_in_i         (pad_in_i         ),
    .pad_oe_i         (pad_oe_i         ),
    .pad_out_o        (pad_out_o        ),
    .pad_io           (pad_io           ),
    .pad_attributes_i (pad_attributes_i )
  );
  */
  
  // Directly assign the pad_io to pad_out_o and pad_in_i
  logic pad;
  assign pad_out_o = pad_io;
  assign pad_io = pad;
  assign pad = (pad_oe_i) ? pad_in_i : 1'bz;

endmodule
