// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia-top.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 29/04/2024
// Description: heepatia top-level module

module heepatia_top (
    // X-HEEP interface
% for pad in total_pad_list:
${pad.x_heep_system_interface}
% endfor
);
  import obi_pkg::*;
  import reg_pkg::*;
  import heepatia_pkg::*;
  import core_v_mini_mcu_pkg::*;

  // PARAMETERS
  localparam int unsigned ExtXbarNmasterRnd = (heepatia_pkg::ExtXbarNMaster > 0) ? heepatia_pkg::ExtXbarNMaster : 32'd1;
  localparam int unsigned ExtDomainsRnd = core_v_mini_mcu_pkg::EXTERNAL_DOMAINS == 0 ?
    32'd1 : core_v_mini_mcu_pkg::EXTERNAL_DOMAINS;

  // INTERNAL SIGNALS
  // ----------------
  // Synchronized reset
  logic rst_nin_sync;

  // System clock
  logic system_clk;

  // Exit value
  logic [31:0] exit_value;

  // X-HEEP external master ports
  obi_req_t  heep_core_instr_req;
  obi_resp_t heep_core_instr_rsp;
  obi_req_t  heep_core_data_req;
  obi_resp_t heep_core_data_rsp;
  obi_req_t  heep_debug_master_req;
  obi_resp_t heep_debug_master_rsp;
  obi_req_t  heep_dma_read_ch0_req;
  obi_resp_t heep_dma_read_ch0_rsp;
  obi_req_t  heep_dma_write_ch0_req;
  obi_resp_t heep_dma_write_ch0_rsp;

  // X-HEEP slave ports
  obi_req_t  [ExtXbarNmasterRnd-1:0] heep_slave_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] heep_slave_rsp;

  // X-HEEP external peripheral master ports
  reg_req_t heep_peripheral_req;
  reg_rsp_t heep_peripheral_rsp;

  // Interrupt vector
  logic [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector;

  // OBI external slaves
  obi_req_t  carus_req; // request to NM-Carus
  obi_resp_t carus_rsp; // response from NM-Carus

  // External peripherals
  reg_req_t fll_req; // request to FLL subsystem
  reg_rsp_t fll_rsp; // response from FLL subsystem
  reg_req_t heepatia_ctrl_req; // request to heepatia controller
  reg_rsp_t heepatia_ctrl_rsp; // response from heepatia controller

  // Pad controller
  reg_req_t pad_req;
  reg_rsp_t pad_rsp;
% if pads_attributes != None:
  logic [core_v_mini_mcu_pkg::NUM_PAD-1:0][${pads_attributes['bits']}] pad_attributes;
% endif
% if total_pad_muxed > 0:
  logic [core_v_mini_mcu_pkg::NUM_PAD-1:0][${max_total_pad_mux_bitlengh-1}:0] pad_muxes;
% endif

  // External power domains
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_switch_n;
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_switch_ack_n;
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_iso_n;

  // External RAM banks retentive mode control
  logic [ExtDomainsRnd-1:0] external_ram_banks_set_retentive_n;

  // External domains reset
  logic [ExtDomainsRnd-1:0] external_subsystem_rst_n;
  logic carus_rst_n;
  logic carus_set_retentive_n;

  // eXtension Interface
  if_xif #() ext_xif ();

`ifndef VERILATOR
  coprosit #(
    .XLEN(coprosit_pkg::XLEN),
    .INPUT_BUFFER_DEPTH(1),
    .FORWARDING(1)
  ) coprosit_i (
    // Clock and Reset
    .clk_i(system_clk),
    .rst_ni(rst_nin_sync),

    // CORE-V eXtension Interface
    .xif_compressed_if (ext_xif.coproc_compressed),
    .xif_issue_if      (ext_xif.coproc_issue),
    .xif_commit_if     (ext_xif.coproc_commit),
    .xif_mem_if        (ext_xif.coproc_mem),
    .xif_mem_result_if (ext_xif.coproc_mem_result),
    .xif_result_if     (ext_xif.coproc_result)
  );
`else
  // Tie the CV-X-IF coprocessor signals to a default value that will
  // receive petitions but reject all offloaded instructions
  initial begin
    ext_xif.compressed_ready   = 1'b1;
    ext_xif.compressed_resp    = '0;

    ext_xif.issue_ready        = 1'b1;
    ext_xif.issue_resp         = '0;

    ext_xif.mem_valid          = 1'b0;
    ext_xif.mem_req            = '0;

    ext_xif.result_valid       = 1'b0;
    ext_xif.result             = '0;
  end
`endif

  // CORE-V-MINI-MCU input/output pins
% for pad in total_pad_list:
${pad.internal_signals}
% endfor

  // Drive to zero bypassed pins
% for pad in total_pad_list:
% if pad.pad_type == 'bypass_inout' or pad.pad_type == 'bypass_input':
% for i in range(len(pad.pad_type_drive)):
% if pad.driven_manually[i] == False:
  assign ${pad.in_internal_signals[i]} = 1'b0;
% endif
% endfor
% endif
% endfor

  // --------------
  // SYSTEM MODULES
  // --------------

  // Reset generator
  // ---------------
  rstgen u_rstgen (
    .clk_i      (ref_clk_in_x),
    .rst_ni     (rst_nin_x),
    .test_mode_i(1'b0 ), // not implemented
    .rst_no     (rst_nin_sync),
    .init_no    () // unused
  );

  // CORE-V-MINI-MCU (microcontroller)
  // ---------------------------------
  core_v_mini_mcu #(
    .COREV_PULP      (CpuCorevPulp),
    .FPU             (CpuFpu),
    .ZFINX           (CpuRiscvZfinx),
    .EXT_XBAR_NMASTER(ExtXbarNMaster),
    .X_EXT           (CpuCorevXif)
  ) u_core_v_mini_mcu (
    .rst_ni (rst_nin_sync),
    .clk_i  (system_clk),

    // MCU pads
% for pad in pad_list:
${pad.core_v_mini_mcu_bonding}
% endfor

`ifdef FPGA
    .spi_flash_cs_1_o (),
    .spi_flash_cs_1_i ('0),
    .spi_flash_cs_1_oe_o(),
`endif

    // CORE-V eXtension Interface
    .xif_compressed_if (ext_xif.cpu_compressed),
    .xif_issue_if      (ext_xif.cpu_issue),
    .xif_commit_if     (ext_xif.cpu_commit),
    .xif_mem_if        (ext_xif.cpu_mem),
    .xif_mem_result_if (ext_xif.cpu_mem_result),
    .xif_result_if     (ext_xif.cpu_result),

    // Pad controller interface
    .pad_req_o  (pad_req),
    .pad_resp_i (pad_rsp),

    // External slave ports
    .ext_xbar_master_req_i (heep_slave_req),
    .ext_xbar_master_resp_o (heep_slave_rsp),

    // External master ports
    .ext_core_instr_req_o (heep_core_instr_req),
    .ext_core_instr_resp_i (heep_core_instr_rsp),
    .ext_core_data_req_o (heep_core_data_req),
    .ext_core_data_resp_i (heep_core_data_rsp),
    .ext_debug_master_req_o (heep_debug_master_req),
    .ext_debug_master_resp_i (heep_debug_master_rsp),
    .ext_dma_read_ch0_req_o (heep_dma_read_ch0_req),
    .ext_dma_read_ch0_resp_i (heep_dma_read_ch0_rsp),
    .ext_dma_write_ch0_req_o (heep_dma_write_ch0_req),
    .ext_dma_write_ch0_resp_i (heep_dma_write_ch0_rsp),
    .ext_dma_addr_ch0_req_o (),
    .ext_dma_addr_ch0_resp_i ('0),

    // External peripherals slave ports
    .ext_peripheral_slave_req_o  (heep_peripheral_req),
    .ext_peripheral_slave_resp_i (heep_peripheral_rsp),

    // Power switches connected by the backend
    .cpu_subsystem_powergate_switch_no            (), // unused
    .cpu_subsystem_powergate_switch_ack_ni        (1'b1),
    .peripheral_subsystem_powergate_switch_no     (), // unused
    .peripheral_subsystem_powergate_switch_ack_ni (1'b1),

    // Other power switches controlled here
    .memory_subsystem_banks_powergate_switch_no(),
    .memory_subsystem_banks_powergate_switch_ack_ni('1),

    .external_subsystem_powergate_switch_no(external_subsystem_powergate_switch_n),
    .external_subsystem_powergate_switch_ack_ni(external_subsystem_powergate_switch_ack_n),
    .external_subsystem_powergate_iso_no(external_subsystem_powergate_iso_n),

    // Control signals for external peripherals
    .external_subsystem_rst_no (external_subsystem_rst_n),
    .external_ram_banks_set_retentive_no (external_ram_banks_set_retentive_n),
    .external_subsystem_clkgate_en_no (), // TODO: add clock gating for external subsystems

    // External interrupts
    .intr_vector_ext_i (ext_int_vector),

    .ext_dma_slot_tx_i('0),
    .ext_dma_slot_rx_i('0),

    .exit_value_o (exit_value)
  );

  // External peripherals
  // --------------------
  assign carus_rst_n  = external_subsystem_rst_n[0];
  assign carus_set_retentive_n  = external_ram_banks_set_retentive_n[0];
  heepatia_peripherals #(
    localparam int unsigned ExtXbarNmasterRnd = (heepatia_pkg::ExtXbarNMaster > 0) ? heepatia_pkg::ExtXbarNMaster : 32'd1 //TODO: is syntax correct?
    ) u_heepatia_peripherals(
    .ref_clk_i             (ref_clk_in_x),
    .rst_ni                (rst_nin_sync),
    .system_clk_o          (system_clk),
    .bypass_fll_i          (bypass_fll_in_x),
    .carus_rst_ni          (carus_rst_n),
    .carus_set_retentive_ni(carus_set_retentive_n),
    .carus_req_i           (carus_req),
    .carus_rsp_o           (carus_rsp),
    .fll_req_i             (fll_req),
    .fll_rsp_o             (fll_rsp),
    .heepatia_ctrl_req_i (heepatia_ctrl_req),
    .heepatia_ctrl_rsp_o (heepatia_ctrl_rsp),
    .ext_int_vector_o      (ext_int_vector),

    // CGRA part
    .cgra_req_i(cgra_req),
    .cgra_resp_o(cgra_resp),
    .cgra_periph_slave_req_i(cgra_periph_slave_req),
    .cgra_periph_slave_resp_o(cgra_periph_slave_resp),
    .cgra_ram_banks_set_retentive_i(external_ram_banks_set_retentive[1]),
    .cgra_logic_rst_n(cgra_logic_rst_n)
    .heepatia_ctrl_cgra_mem_sw_fb_i(cgra_mem_sw_fb_sync),
    .heep_slave_req_o(heep_ext_master_req),     // TODO: these two im not sure
    heep_slave_resp_i(heep_ext_master_resp)    // TODO: these two im not sure
  );

  // External peripherals bus
  // ------------------------
  // External subsystem bus
  heepatia_bus u_heepatia_bus (
    .clk_i                    (system_clk),
    .rst_ni                   (rst_nin_sync),
    .heep_core_instr_req_i    (heep_core_instr_req),
    .heep_core_instr_resp_o   (heep_core_instr_rsp),
    .heep_core_data_req_i     (heep_core_data_req),
    .heep_core_data_resp_o    (heep_core_data_rsp),
    .heep_debug_master_req_i  (heep_debug_master_req),
    .heep_debug_master_resp_o (heep_debug_master_rsp),
    .heep_dma_read_ch0_req_i  (heep_dma_read_ch0_req),
    .heep_dma_read_ch0_resp_o (heep_dma_read_ch0_rsp),
    .heep_dma_write_ch0_req_i (heep_dma_write_ch0_req),
    .heep_dma_write_ch0_resp_o(heep_dma_write_ch0_rsp),
    .heep_slave_req_o         (heep_slave_req),
    .heep_slave_resp_i        (heep_slave_rsp),
    .carus_req_o              (carus_req),
    .carus_resp_i             (carus_rsp),
    .heep_periph_req_i        (heep_peripheral_req),
    .heep_periph_resp_o       (heep_peripheral_rsp),
    .fll_req_o                (fll_req),
    .fll_resp_i               (fll_rsp),
    .heepatia_ctrl_req_o    (heepatia_ctrl_req),
    .heepatia_ctrl_resp_i   (heepatia_ctrl_rsp),

    // .ext_xbar_slave_req_i(heep_ext_slave_req),
    // .ext_xbar_slave_resp_o(heep_ext_slave_resp),
    .cgra_req_o(cgra_req),
    .cgra_resp_i(cgra_resp),
    // .ext_peripheral_slave_req_i(heep_ext_periph_slave_req),
    // .ext_peripheral_slave_resp_o(heep_ext_periph_slave_resp),
    .cgra_periph_slave_req_o(cgra_periph_slave_req),
    .cgra_periph_slave_resp_i(cgra_periph_slave_resp),
    // .heepocrates_ctrl_slave_req_o(heepocrates_ctrl_slave_req),
    // .heepocrates_ctrl_slave_resp_i(heepocrates_ctrl_slave_resp)
    // .fll_slave_req_o(fll_slave_req),
    // .fll_slave_resp_i(fll_slave_resp)
  );

  // Pad ring
  // --------
  assign exit_value_out_x = exit_value[0];
  pad_ring u_pad_ring (
% for pad in total_pad_list:
${pad.pad_ring_bonding_bonding}
% endfor

    // Pad attributes
% if pads_attributes != None:
    .pad_attributes_i(pad_attributes)
% else:
    .pad_attributes_i('0)
% endif
  );

  // Constant pad signals
${pad_constant_driver_assign}

  // Shared pads multiplexing
${pad_mux_process}

  // Pad control
  // -----------
  pad_control #(
    .reg_req_t (reg_req_t),
    .reg_rsp_t (reg_rsp_t),
    .NUM_PAD   (NUM_PAD)
  ) u_pad_control (
    .clk_i            (system_clk),
    .rst_ni           (rst_nin_sync),
    .reg_req_i        (pad_req),
    .reg_rsp_o        (pad_rsp)
% if total_pad_muxed > 0 or pads_attributes != None:
      ,
% endif
% if pads_attributes != None:
      .pad_attributes_o(pad_attributes)
% if total_pad_muxed > 0:
      ,
% endif
% endif
% if total_pad_muxed > 0:
      .pad_muxes_o(pad_muxes)
% endif
  );

  // FLL clock divided output through a pad for debugging
  clk_int_div #(
      .DIV_VALUE_WIDTH  ($clog2(1023 + 1)),
      .DEFAULT_DIV_VALUE(1023)
  ) i_clk_int_div (
      .clk_i(system_clk),
      .rst_ni(rst_nin_sync),
      .test_mode_en_i(1'b0),
      .en_i(1'b1),
      .div_i('1),  // Ignored, used default value
      .div_valid_i(1'b0),
      .div_ready_o(),
      .clk_o(fll_clk_div_out_x),
      .cycl_count_o()
  );

  // CARUS
  // -----------
  // TODO: add clock gating cell
  // Connect to CORE-V-MINI-MCU power manager

  logic carus_sw0_ctrl;
  logic carus_sw0_ack, carus_sw1_ack, carus_sw2_ack, carus_sw3_ack;

`ifndef FPGA

    assign carus_sw0_ctrl = ~external_subsystem_powergate_switch_n[0];
    assign external_subsystem_powergate_switch_ack_n[0] = ~carus_sw3_ack;

    // Power switch and synchronizer
    switch_cell_mem mem_carus_sw0_i (
  `ifdef USE_PG_PIN
      .VIN,
      .VOUT,
      .VSS,
  `endif
      .VCTRL    (carus_sw0_ctrl),  // Switch Signal Input
      .VCTRLFBn (),               // Negated Schmitt Trigger Output
      .VCTRLFB  (),    // Schmitt Trigger Output
      .VCTRL_BUF(carus_sw0_ack)    //ACK signal Output
    );

    switch_cell_mem mem_carus_sw1_i (
  `ifdef USE_PG_PIN
      .VIN,
      .VOUT,
      .VSS,
  `endif
      .VCTRL    (carus_sw0_ack),  // Switch Signal Input
      .VCTRLFBn (),              // Negated Schmitt Trigger Output
      .VCTRLFB  (),   // Schmitt Trigger Output
      .VCTRL_BUF(carus_sw1_ack)   //ACK signal Output
    );

    switch_cell_mem mem_carus_sw2_i (
  `ifdef USE_PG_PIN
      .VIN,
      .VOUT,
      .VSS,
  `endif
      .VCTRL    (carus_sw1_ack),  // Switch Signal Input
      .VCTRLFBn (),              // Negated Schmitt Trigger Output
      .VCTRLFB  (),   // Schmitt Trigger Output
      .VCTRL_BUF(carus_sw2_ack)   //ACK signal Output
    );

    switch_cell_mem mem_carus_sw3_i (
  `ifdef USE_PG_PIN
      .VIN,
      .VOUT,
      .VSS,
  `endif
      .VCTRL    (carus_sw2_ack),  // Switch Signal Input
      .VCTRLFBn (),              // Negated Schmitt Trigger Output
      .VCTRLFB  (),   // Schmitt Trigger Output
      .VCTRL_BUF(carus_sw3_ack)   //ACK signal Output
    );

`else

  assign carus_sw0_ctrl = '0;
  assign external_subsystem_powergate_switch_ack_n[1] = '0;

`endif

//   //CGRA SRAM SWITCH CELLs

//   logic cgra_mem_sw0_ctrl;

// % for cgra_switch in range(4):
//   logic cgra_mem_sw${cgra_switch}_fb;
//   logic cgra_mem_sw${cgra_switch}_fb_sync;
//   logic cgra_mem_sw${cgra_switch}_ack;
// % endfor

//   assign cgra_mem_sw_fb_sync = {cgra_mem_sw3_fb_sync, cgra_mem_sw2_fb_sync, cgra_mem_sw1_fb_sync, cgra_mem_sw0_fb_sync};

//   // connection with core-v-mini-mcu's power manager
//   assign cgra_mem_sw0_ctrl                = ~cgra_memory_powergate_switch;
//   assign cgra_memory_powergate_switch_ack = ~cgra_mem_sw3_ack;


// % for cgra_switch in range(4):
//   switch_cell_mem cgra_mem_sw${cgra_switch}_i (
// `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
// `endif
// % if cgra_switch == 0:
//     .VCTRL(cgra_mem_sw${cgra_switch}_ctrl), // Switch Signal Input
// %else:
//     .VCTRL(cgra_mem_sw${cgra_switch-1}_ack), // Switch Signal Input
// %endif
//     .VCTRLFBn(), // Negated Schmitt Trigger Output
//     .VCTRLFB(cgra_mem_sw${cgra_switch}_fb), // Schmitt Trigger Output
//     .VCTRL_BUF(cgra_mem_sw${cgra_switch}_ack) //ACK signal Output
//   );

//   sync #(
//     .ResetValue(1'b1)
//   ) sync_cgra_mem_sw${cgra_switch}_fb_i (
//       .clk_i(system_clk),
//       .rst_ni(rst_nin_sync),
//       .serial_i(cgra_mem_sw${cgra_switch}_fb),
//       .serial_o(cgra_mem_sw${cgra_switch}_fb_sync)
//   );

// % endfor

endmodule // heepatia_top
