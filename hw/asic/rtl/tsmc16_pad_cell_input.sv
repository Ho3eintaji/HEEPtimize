// Copyright 2024 EPFL and Universidad Complutense de Madrid
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Author: David Mallasén
// Date: 23/04/2024
// Description: TSMC16 input pad standard cell

// NOTE: Based on the same cell from HEEPocrates (https://eslgit.epfl.ch/heep/HEEPpocrates)

module tsmc16_pad_cell_input #(
  parameter int unsigned PADATTR = 16,
  parameter core_v_mini_mcu_pkg::pad_side_e SIDE = core_v_mini_mcu_pkg::TOP
) (
  input logic pad_in_i, // pad input value
  input logic pad_oe_i, // pad output enable
  output logic pad_out_o, // pad output value
  input logic pad_io, // pad value
  input logic [PADATTR-1:0] pad_attributes_i // pad attributes
);

  if (SIDE == core_v_mini_mcu_pkg::TOP || SIDE == core_v_mini_mcu_pkg::BOTTOM) begin
    // Top or bottom placement, use the horizontal version of the pad
    // TSMC16 pad standard cell
    PDDWUWSWCDGS_H u_pad_input_h (
      .ST  (pad_attributes_i[3]),
      .IE  (pad_attributes_i[2]),
      .PU  (pad_attributes_i[0]),
      .PD  (pad_attributes_i[1]),
      .DS0 (pad_attributes_i[4]),
      .DS1 (pad_attributes_i[5]),
      .DS2 (pad_attributes_i[6]),
      .DS3 (pad_attributes_i[7]),
      .I   (1'b0),
      .OEN (1'b1),
      .RTE (1'b0),
      .C   (pad_out_o),
      .PAD (pad_io)
    );
  end else if (SIDE == core_v_mini_mcu_pkg::LEFT || SIDE == core_v_mini_mcu_pkg::RIGHT) begin
    // Left or right placement, use the vertical version of the pad
    // TSMC16 pad standard cell
    PDDWUWSWCDGS_V u_pad_input_v (
      .ST  (pad_attributes_i[3]),
      .IE  (pad_attributes_i[2]),
      .PU  (pad_attributes_i[0]),
      .PD  (pad_attributes_i[1]),
      .DS0 (pad_attributes_i[4]),
      .DS1 (pad_attributes_i[5]),
      .DS2 (pad_attributes_i[6]),
      .DS3 (pad_attributes_i[7]),
      .I   (1'b0),
      .OEN (1'b1),
      .RTE (1'b0),
      .C   (pad_out_o),
      .PAD (pad_io)
    );
  end
endmodule
