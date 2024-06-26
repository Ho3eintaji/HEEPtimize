// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia_peripherals.sv
// Author: Michele Caon, Luigi Giuffrida, Hossein Taji
// Date: 29/04/2024
// Description: heepatia peripheral subsystem

module heepatia_peripherals #(
  // Dependent parameters: do not override!
  localparam int unsigned CarusNumRnd  = (heepatia_pkg::CarusNum > 32'd1) ? heepatia_pkg::CarusNum : 32'd1,
  localparam int unsigned ExtXbarNmasterRnd = (heepatia_pkg::ExtXbarNMaster > 0) ? heepatia_pkg::ExtXbarNMaster : 32'd1
) (
  input logic ref_clk_i,
  input logic rst_ni,

  // System clock
  output logic system_clk_o,
  input  logic bypass_fll_i,

  // NM-Carus
  input  logic                                 carus_rst_ni,
  input  logic                                 carus_set_retentive_ni,
  input  obi_pkg::obi_req_t  [CarusNumRnd-1:0] carus_req_i,
  output obi_pkg::obi_resp_t [CarusNumRnd-1:0] carus_rsp_o,

  // OECGRA
  input logic oecgra_rst_ni,
  input logic oecgra_enable_i,

  output obi_pkg::obi_req_t  [ExtXbarNmasterRnd-1:0] oecgra_master_req_o,
  input  obi_pkg::obi_resp_t [ExtXbarNmasterRnd-1:0] oecgra_master_resp_i,

  input  obi_pkg::obi_req_t  oecgra_context_mem_slave_req_i,
  output obi_pkg::obi_resp_t oecgra_context_mem_slave_rsp_o,

  input  reg_pkg::reg_req_t oecgra_config_regs_slave_req_i,
  output reg_pkg::reg_rsp_t oecgra_config_regs_slave_rsp_o,

  input logic oecgra_context_mem_set_retentive_i,


  // FLL Subsystem
  input  reg_pkg::reg_req_t fll_req_i,
  output reg_pkg::reg_rsp_t fll_rsp_o,

  // Control and status registers
  input  reg_pkg::reg_req_t heepatia_ctrl_req_i,
  output reg_pkg::reg_rsp_t heepatia_ctrl_rsp_o,

  // Interrupts
  output [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector_o
);
  import heepatia_pkg::*;

  // INTERNAL SIGNALS
  // ----------------
  // System clock
  logic                   system_clk;

  // Near-memory computing devices
  logic [CarusNumRnd-1:0] carus_imc;  // computing mode trigger for NM-Carus
  logic [   CarusNum-1:0] carus_intr;  // interrupts from NM-Carus

  // OECGRA interrupt
  logic                   oecgra_int;

  // --------------
  // OUTPUT CONTROL
  // --------------
  assign system_clk_o                                        = system_clk;
  assign ext_int_vector_o[core_v_mini_mcu_pkg::NEXT_INT-1:2] = '0;
  // NOTE: all Carus interrupts are aggregated into a single external interrupt
  // line. The associated interrupt handling routine must determine which
  // instance triggered the interrupt by reading the corresponding status
  // registers.
  assign ext_int_vector_o[0]                                 = |carus_intr;
  assign ext_int_vector_o[1]                                 = oecgra_int;

  // ----------
  // COMPONENTS
  // ----------

  // NM-Carus
  // --------
  generate
    for (genvar i = 0; unsigned'(i) < CarusNum; i++) begin : gen_carus
      nm_carus_wrapper #(
        .NUM_BANKS      (heepatia_pkg::InstancesNumBanks[i]),
        .BANK_ADDR_WIDTH(heepatia_pkg::InstancesBankAddrWidth[i])
      ) u_nm_carus_wrapper (
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

  // `ifndef REMOVE_OECGRA

  // OECGRA //todo: double check the top wrapper!
  // -----
  cgra_top_wrapper oecgra_i (
    .clk_i               (system_clk),
    .rst_ni              (oecgra_rst_ni),
    .rst_logic_ni        (oecgra_rst_ni),
    .cgra_enable_i       (oecgra_enable_i),
    .masters_req_o       (oecgra_master_req_o),
    .masters_resp_i      (oecgra_master_resp_i),
    .reg_req_i           (oecgra_config_regs_slave_req_i),
    .reg_rsp_o           (oecgra_config_regs_slave_rsp_o),
    .slave_req_i         (oecgra_context_mem_slave_req_i),
    .slave_resp_o        (oecgra_context_mem_slave_rsp_o),
    .cmem_set_retentive_i(oecgra_context_mem_set_retentive_i),
    .cgra_int_o          (oecgra_int)
  );

  // `endif

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
  heepatia_ctrl_reg u_heepatia_ctrl_reg (
    .clk_i      (system_clk),
    .rst_ni     (rst_ni),
    .req_i      (heepatia_ctrl_req_i),
    .rsp_o      (heepatia_ctrl_rsp_o),
    .carus_imc_o(carus_imc)
  );

endmodule
