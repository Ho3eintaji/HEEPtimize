// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia_peripherals.sv
// Author: Michele Caon, Luigi Giuffrida
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

  // Masters (only used for cgra)
  output obi_req_t  [ExtXbarNmasterRnd-1:0] heep_slave_req_o,
  input  obi_resp_t [ExtXbarNmasterRnd-1:0] heep_slave_resp_i,

  // Slaves
  input  logic                                 carus_rst_ni,
  input  logic                                 carus_set_retentive_ni,
  input  obi_pkg::obi_req_t  [CarusNumRnd-1:0] carus_req_i,
  output obi_pkg::obi_resp_t [CarusNumRnd-1:0] carus_rsp_o,

  input  reg_pkg::reg_req_t fll_req_i,
  output reg_pkg::reg_rsp_t fll_rsp_o,

  input  reg_pkg::reg_req_t heepatia_ctrl_req_i,
  output reg_pkg::reg_rsp_t heepatia_ctrl_rsp_o,

  // Slaves (cgra)
  input  obi_req_t  cgra_req_i,
  output obi_resp_t cgra_resp_o,

  input  reg_req_t cgra_periph_slave_req_i,
  output reg_rsp_t cgra_periph_slave_resp_o,

  input logic cgra_ram_banks_set_retentive_i,

  // input  reg_req_t fll_slave_req_i,
  // output reg_rsp_t fll_slave_resp_o,

  // input  reg_req_t       heepocrates_ctrl_slave_req_i,
  // output reg_rsp_t       heepocrates_ctrl_slave_resp_o,

  // we don't have memory switches in the current implementation
  // input  logic     [3:0] heepocrates_ctrl_mem_sw_fb_i      [core_v_mini_mcu_pkg::NUM_BANKS],
  input  logic     [3:0] heepatia_ctrl_cgra_mem_sw_fb_i,    //TODO: is it fine? I also add to heepatia_ctrl_reg.sv?

  // CGRA logic reset
  input logic cgra_logic_rst_n,

  // Interrupts
  output [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector_o
);
  import heepatia_pkg::*;

  // INTERNAL SIGNALS
  // ----------------
  // System clock
  logic system_clk;

  // Near-memory computing devices
  logic [CarusNumRnd-1:0] carus_imc;  // computing mode trigger for NM-Carus
  logic [CarusNum-1:0] carus_intr;  // interrupts from NM-Carus

  // CGRA master ports
  obi_req_t [ExtXbarNmasterRnd-1:0] cgra_masters_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] cgra_masters_resp;

  // CGRA enable and interrupt
  logic cgra_int;
  logic cgra_enable;

  // --------------
  // OUTPUT CONTROL
  // --------------
  assign system_clk_o = system_clk;
  assign ext_int_vector_o[core_v_mini_mcu_pkg::NEXT_INT-1:1] = '0;
  // NOTE: all Carus interrupts are aggregated into a single external interrupt
  // line. The associated interrupt handling routine must determine which
  // instance triggered the interrupt by reading the corresponding status
  // registers.
  assign ext_int_vector_o[0] = |carus_intr;
  // NOTE: the CGRA interrupt is directly connected to the external interrupt
  assign ext_int_vector_o[1]    = cgra_int; //TODO: is it correct? (in HEEPocrates it is in another format)

  // CGRA master ports
  assign heep_slave_req_o = cgra_masters_req;
  assign cgra_masters_resp = heep_slave_resp_i;


  // ----------
  // COMPONENTS
  // ----------

  // NM-Carus
  // --------
  generate
    for (genvar i = 0; unsigned'(i) < CarusNum; i++) begin : gen_carus
      nm_carus_wrapper #(
        .NUM_BANKS      (heepatia_pkg::CarusNumBanks),
        .BANK_ADDR_WIDTH(heepatia_pkg::CarusBankAddrWidth)
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
    .clk_i           (system_clk),
    .rst_ni          (rst_ni),
    .req_i           (heepatia_ctrl_req_i),
    .rsp_o           (heepatia_ctrl_rsp_o),
    .carus_imc_o     (carus_imc),
    .cgra_enable_o   (cgra_enable),
    .cgra_mem_sw_fb_i(heepatia_ctrl_cgra_mem_sw_fb_i)
  );

  cgra_top_wrapper cgra_top_wrapper_i (
    .clk_i               (system_clk),
    .rst_ni,
    .cgra_enable_i       (cgra_enable),
    .rst_logic_ni        (cgra_logic_rst_n),
    .masters_req_o       (cgra_masters_req),
    .masters_resp_i      (cgra_masters_resp),
    .reg_req_i           (cgra_periph_slave_req_i),
    .reg_rsp_o           (cgra_periph_slave_resp_o),
    .slave_req_i         (cgra_req_i),
    .slave_resp_o        (cgra_resp_o),
    .cmem_set_retentive_i(cgra_ram_banks_set_retentive_i),  //TODO: should be hardweird
    .cgra_int_o          (cgra_int)
  );

endmodule
