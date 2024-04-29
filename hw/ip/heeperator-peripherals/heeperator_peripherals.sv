// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heeperator_peripherals.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 29/04/2024
// Description: HEEPerator peripheral subsystem

module heeperator_peripherals #(
  // Dependent parameters: do not override!
  localparam int unsigned CarusNumRnd  = (heeperator_pkg::CarusNum > 32'd1) ? heeperator_pkg::CarusNum : 32'd1
) (
  input logic ref_clk_i,
  input logic rst_ni,

  // System clock
  output logic system_clk_o,
  input  logic bypass_fll_i,

  // Slaves
  input  logic                                 carus_rst_ni,
  input  logic                                 carus_set_retentive_ni,
  input  obi_pkg::obi_req_t  [CarusNumRnd-1:0] carus_req_i,
  output obi_pkg::obi_resp_t [CarusNumRnd-1:0] carus_rsp_o,

  input  reg_pkg::reg_req_t fll_req_i,
  output reg_pkg::reg_rsp_t fll_rsp_o,

  input  reg_pkg::reg_req_t heeperator_ctrl_req_i,
  output reg_pkg::reg_rsp_t heeperator_ctrl_rsp_o,

  // Interrupts
  output [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector_o
);
  import heeperator_pkg::*;

  // INTERNAL SIGNALS
  // ----------------
  // System clock
  logic                   system_clk;

  // Near-memory computing devices
  logic [CarusNumRnd-1:0] carus_imc;  // computing mode trigger for NM-Carus
  logic [   CarusNum-1:0] carus_intr;  // interrupts from NM-Carus

  // --------------
  // OUTPUT CONTROL
  // --------------
  assign system_clk_o                                        = system_clk;
  assign ext_int_vector_o[core_v_mini_mcu_pkg::NEXT_INT-1:1] = '0;
  // NOTE: all Carus interrupts are aggregated into a single external interrupt
  // line. The associated interrupt handling routine must determine which
  // instance triggered the interrupt by reading the corresponding status
  // registers.
  assign ext_int_vector_o[0]                                 = |carus_intr;

  // ----------
  // COMPONENTS
  // ----------

  // NM-Carus
  // --------
  generate
    for (genvar i = 0; unsigned'(i) < CarusNum; i++) begin : gen_carus
      nm_carus_wrapper u_nm_carus_wrapper (
        .clk_i           (system_clk),
        .rst_ni          (carus_rst_ni),
        .set_retentive_ni(carus_set_retentive_ni),
        .imc_i           (carus_imc[i]),
        .done_o          (carus_intr[i]),
        .bus_req_i       (carus_req_i[i]),
        .bus_rsp_o       (carus_rsp_o[i])
      );
    end
  endgenerate

`ifndef FPGA

  // FLL Subsystem
  // -------------
  fll_subsystem u_fll_subsystem (
    .ref_clk_i       (ref_clk_i),
    .rst_ni          (rst_ni),
    .system_clk_o    (system_clk),
    .bypass_fll_i    (bypass_fll_i),
    .fll_slave_req_i (fll_req_i),
    .fll_slave_resp_o(fll_rsp_o)
  );

`else

  assign system_clk = ref_clk_i;

`endif

  // Control and status registers
  // ----------------------------
  heeperator_ctrl_reg u_heeperator_ctrl_reg (
    .clk_i      (system_clk),
    .rst_ni     (rst_ni),
    .req_i      (heeperator_ctrl_req_i),
    .rsp_o      (heeperator_ctrl_rsp_o),
    .carus_imc_o(carus_imc)
  );
endmodule
