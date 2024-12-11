// Copyright 2022 EPFL.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: oecgra_bus.sv
// Author: Simone Machetti, Hossein Taji
// Date: 11/06/2024
// Description: OE-CGRA bus for HEEPatia

module oecgra_bus
  import obi_pkg::*;
  import reg_pkg::*;
  import addr_map_rule_pkg::*;
  import core_v_mini_mcu_pkg::*;
  import heepatia_pkg::*;
#(
) (
    // Clock and reset
    input logic clk_i,
    input logic rst_ni,

    // OECGRA OBI slave input
    input  obi_pkg::obi_req_t  oecgra_req_i,
    output obi_pkg::obi_resp_t oecgra_rsp_o,

    // OECGRA OBI master output
    output obi_pkg::obi_req_t  oecgra_context_mem_req_o,
    input  obi_pkg::obi_resp_t oecgra_context_mem_rsp_i,

    // OECGRA REG master output
    output reg_pkg::reg_req_t oecgra_config_regs_req_o,
    input  reg_pkg::reg_rsp_t oecgra_config_regs_rsp_i
);
  obi_pkg::obi_req_t  [1:0] oecgra_req;
  obi_pkg::obi_resp_t [1:0] oecgra_rsp;

  xbar_varlat_one_to_n #(
      .XBAR_NSLAVE(32'd2),
      .NUM_RULES  (32'd2)
  ) oecgra_varlat_i (
      .clk_i        (clk_i),
      .rst_ni       (rst_ni),
      .addr_map_i   (OecgraSlaveAddrRules),
      .default_idx_i('0),
      .master_req_i (oecgra_req_i),
      .master_resp_o(oecgra_rsp_o),
      .slave_req_o  (oecgra_req),
      .slave_resp_i (oecgra_rsp)
  );

  assign oecgra_context_mem_req_o = oecgra_req[0];
  assign oecgra_rsp[0]            = oecgra_context_mem_rsp_i;

  periph_to_reg #(
      .req_t(reg_pkg::reg_req_t),
      .rsp_t(reg_pkg::reg_rsp_t),
      .IW   (1)
  ) oecgra_periph_to_reg_i (
      .clk_i,
      .rst_ni,
      .req_i    (oecgra_req[1].req),
      .add_i    (oecgra_req[1].addr),
      .wen_i    (~oecgra_req[1].we),
      .wdata_i  (oecgra_req[1].wdata),
      .be_i     (oecgra_req[1].be),
      .id_i     ('0),
      .gnt_o    (oecgra_rsp[1].gnt),
      .r_rdata_o(oecgra_rsp[1].rdata),
      .r_opc_o  (),
      .r_id_o   (),
      .r_valid_o(oecgra_rsp[1].rvalid),
      .reg_req_o(oecgra_config_regs_req_o),
      .reg_rsp_i(oecgra_config_regs_rsp_i)
  );

endmodule : oecgra_bus
