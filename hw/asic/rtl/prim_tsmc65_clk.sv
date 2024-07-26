// Copyright 2022 OpenHW Group
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

module tsmc65_clk_gating (
    input  logic clk_i,
    input  logic en_i,
    input  logic test_en_i,
    output logic clk_o
);


  /*
    add here your standard cell
  */
  CKLNQD16LVT clk_gating_i (
      .TE(test_en_i),
      .CP(clk_i),
      .E (en_i),
      .Q (clk_o)
  );

endmodule

module tsmc65_clk_inverter (
    input  logic clk_i,
    output logic clk_o
);

  /*
    add here your standard cell
  */
  CKND16LVT clk_inv_i (
      .I (clk_i),
      .ZN(clk_o)
  );

endmodule


module tsmc65_clk_mux2 (
    input  logic clk0_i,
    input  logic clk1_i,
    input  logic clk_sel_i,
    output logic clk_o
);

  /*
    add here your standard cell
  */
  CKMUX2D4LVT clk_mux2_i (
      .I0(clk0_i),
      .I1(clk1_i),
      .S (clk_sel_i),
      .Z (clk_o)
  );

endmodule

module tsmc65_clk_xor2 (
    input  logic clk0_i,
    input  logic clk1_i,
    output logic clk_o
);

  /*
    add here your standard cell
  */
  CKXOR2D4LVT clk_xor2_i (
      .A1(clk0_i),
      .A2(clk1_i),
      .Z (clk_o)
  );

endmodule

module cluster_clock_inverter (
    input  logic clk_i,
    output logic clk_o
);

  tsmc65_clk_inverter clk_inv_i (.*);

endmodule

module pulp_clock_mux2 (
    input  logic clk0_i,
    input  logic clk1_i,
    input  logic clk_sel_i,
    output logic clk_o
);

  tsmc65_clk_mux2 clk_mux2_i (.*);

endmodule

module cv32e40p_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic scan_cg_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i(scan_cg_en_i),
      .clk_o
  );

endmodule

module cve2_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic scan_cg_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i(scan_cg_en_i),
      .clk_o
  );

endmodule

module cv32e40x_clock_gate #(
    parameter LIB = 0
) (
    input  logic clk_i,
    input  logic en_i,
    input  logic scan_cg_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i(scan_cg_en_i),
      .clk_o
  );

endmodule

module cv32e40px_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic scan_cg_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i(scan_cg_en_i),
      .clk_o
  );

endmodule

module cgra_clock_gate (
    input  logic clk_i,
    input  logic en_i,
    input  logic test_en_i,
    output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i,
      .clk_o
  );

endmodule

module tc_clk_gating #(
  parameter bit IS_FUNCTIONAL = 1'b1
)(
   input  logic clk_i,
   input  logic en_i,
   input  logic test_en_i,
   output logic clk_o
);

  tsmc65_clk_gating clk_gate_i (
      .clk_i,
      .en_i,
      .test_en_i,
      .clk_o
  );

endmodule

module tc_clk_mux2 (
  input  logic clk0_i,
  input  logic clk1_i,
  input  logic clk_sel_i,
  output logic clk_o
);

  tsmc65_clk_mux2 tsmc65_clk_mux2_i (
    .clk0_i,
    .clk1_i,
    .clk_sel_i,
    .clk_o
  );

endmodule

module tc_clk_xor2 (
  input  logic clk0_i,
  input  logic clk1_i,
  output logic clk_o
);

  tsmc65_clk_xor2 tsmc65_clk_xor2_i (
    .clk0_i,
    .clk1_i,
    .clk_o
  );

endmodule

module tc_clk_inverter (
    input  logic clk_i,
    output logic clk_o
);

  tsmc65_clk_inverter clk_inv_i (.*);

endmodule
