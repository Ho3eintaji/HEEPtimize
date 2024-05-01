// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia_bus.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 29/04/2024
// Description: External bus for heepatia

module heepatia_bus #(
  // Dependent parameters: do not override!
  localparam int unsigned ExtXbarNmasterRnd = (heepatia_pkg::ExtXbarNMaster > 0) ?
    heepatia_pkg::ExtXbarNMaster : 32'd1,
  localparam int unsigned CarusNumRnd = (heepatia_pkg::CarusNum > 0) ? heepatia_pkg::CarusNum : 32'd1
) (
  input logic clk_i,
  input logic rst_ni,

  // X-HEEP master ports
  input  obi_pkg::obi_req_t  heep_core_instr_req_i,
  output obi_pkg::obi_resp_t heep_core_instr_resp_o,

  input  obi_pkg::obi_req_t  heep_core_data_req_i,
  output obi_pkg::obi_resp_t heep_core_data_resp_o,

  input  obi_pkg::obi_req_t  heep_debug_master_req_i,
  output obi_pkg::obi_resp_t heep_debug_master_resp_o,

  input  obi_pkg::obi_req_t  heep_dma_read_ch0_req_i,
  output obi_pkg::obi_resp_t heep_dma_read_ch0_resp_o,

  input  obi_pkg::obi_req_t  heep_dma_write_ch0_req_i,
  output obi_pkg::obi_resp_t heep_dma_write_ch0_resp_o,

  // X-HEEP slave ports (one per external master)
  output obi_pkg::obi_req_t  [ExtXbarNmasterRnd-1:0] heep_slave_req_o,
  input  obi_pkg::obi_resp_t [ExtXbarNmasterRnd-1:0] heep_slave_resp_i,

  // NM-Carus slave ports
  output obi_pkg::obi_req_t  [CarusNumRnd-1:0] carus_req_o,
  input  obi_pkg::obi_resp_t [CarusNumRnd-1:0] carus_resp_i,

  // X-HEEP peripheral master port
  input  reg_pkg::reg_req_t heep_periph_req_i,
  output reg_pkg::reg_rsp_t heep_periph_resp_o,

  // External peripherals slave ports
  output reg_pkg::reg_req_t fll_req_o,
  input  reg_pkg::reg_rsp_t fll_resp_i,

  output reg_pkg::reg_req_t heepatia_ctrl_req_o,
  input  reg_pkg::reg_rsp_t heepatia_ctrl_resp_i
);
  import heepatia_pkg::*;
  import obi_pkg::*;
  import reg_pkg::*;

  // PARAMETERS
  localparam int unsigned IdxWidth = cf_math_pkg::idx_width(ExtXbarNSlave);

  // INTERNAL SIGNALS
  // ----------------
  // External slaves request
  obi_req_t  [  ExtXbarNSlave-1:0] ext_slave_req;
  obi_resp_t [  ExtXbarNSlave-1:0] ext_slave_rsp;

  // External peripherals request
  reg_req_t  [ExtPeriphNSlave-1:0] ext_periph_req;
  reg_rsp_t  [ExtPeriphNSlave-1:0] ext_periph_rsp;

  // ----------
  // COMPONENTS
  // ----------
  generate
    for (genvar i = 0; i < CarusNumRnd; i++) begin : gen_carus_req
      assign carus_req_o[i]   = ext_slave_req[i];
      assign ext_slave_rsp[i] = carus_resp_i[i];
    end
  endgenerate
  // External slave bus
  // ------------------
  // External slave mapping

  // External bus
  ext_bus #(
    .EXT_XBAR_NMASTER(ExtXbarNMaster),
    .EXT_XBAR_NSLAVE (ExtXbarNSlave)
  ) u_ext_bus (
    .clk_i                    (clk_i),
    .rst_ni                   (rst_ni),
    .addr_map_i               (ExtSlaveAddrRules),
    .default_idx_i            (ExtSlaveDefaultIdx[IdxWidth-1:0]),
    .heep_core_instr_req_i    (heep_core_instr_req_i),
    .heep_core_instr_resp_o   (heep_core_instr_resp_o),
    .heep_core_data_req_i     (heep_core_data_req_i),
    .heep_core_data_resp_o    (heep_core_data_resp_o),
    .heep_debug_master_req_i  (heep_debug_master_req_i),
    .heep_debug_master_resp_o (heep_debug_master_resp_o),
    .heep_dma_read_ch0_req_i  (heep_dma_read_ch0_req_i),
    .heep_dma_read_ch0_resp_o (heep_dma_read_ch0_resp_o),
    .heep_dma_write_ch0_req_i (heep_dma_write_ch0_req_i),
    .heep_dma_write_ch0_resp_o(heep_dma_write_ch0_resp_o),
    .ext_master_req_i         ('0),                                // no external masters
    .ext_master_resp_o        (),                                  // no external masters
    .heep_slave_req_o         (heep_slave_req_o),
    .heep_slave_resp_i        (heep_slave_resp_i),
    .ext_slave_req_o          (ext_slave_req),
    .ext_slave_resp_i         (ext_slave_rsp)
  );

  // External peripherals bus
  // ------------------------
  // External peripherals mapping
  assign fll_req_o                         = ext_periph_req[FLLIdx];
  assign ext_periph_rsp[FLLIdx]            = fll_resp_i;
  assign heepatia_ctrl_req_o               = ext_periph_req[HeeperatorCtrlIdx];
  assign ext_periph_rsp[HeeperatorCtrlIdx] = heepatia_ctrl_resp_i;

  // External peripherals bus
  periph_bus #(
    .NSLAVE(ExtPeriphNSlave)
  ) u_periph_bus (
    .clk_i       (clk_i),
    .rst_ni      (rst_ni),
    .addr_map_i  (ExtPeriphAddrRules),
    .master_req_i(heep_periph_req_i),
    .master_rsp_o(heep_periph_resp_o),
    .slave_req_o (ext_periph_req),
    .slave_rsp_i (ext_periph_rsp)
  );
endmodule
