// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia-top.sv
// Author: Michele Caon, Luigi Giuffrida, Hossein Taji
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
  localparam int unsigned ExtDomainsRnd = core_v_mini_mcu_pkg::EXTERNAL_DOMAINS == 0 ? 32'd1 
                                            : core_v_mini_mcu_pkg::EXTERNAL_DOMAINS;
  localparam int unsigned CarusNumRnd = (heepatia_pkg::CarusNum > 32'd1) ? heepatia_pkg::CarusNum : 32'd1;

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
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_read_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_read_rsp;
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_write_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_write_rsp;
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_addr_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_addr_rsp;

  // X-HEEP slave ports
  obi_req_t  [ExtXbarNmasterRnd-1:0] heep_slave_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] heep_slave_rsp;

  // External master ports
  obi_req_t  [ExtXbarNmasterRnd-1:0] heepatia_master_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] heepatia_master_resp;

  // X-HEEP external peripheral master ports
  reg_req_t heep_peripheral_req;
  reg_rsp_t heep_peripheral_rsp;

  // Interrupt vector
  logic [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector;

  // OBI external slaves
  obi_req_t  [CarusNumRnd - 1:0]carus_req; // request to NM-Carus
  obi_resp_t [CarusNumRnd - 1:0]carus_rsp; // response from NM-Carus
  obi_req_t  caesar_req; // request to NM-Caesar
  obi_resp_t caesar_rsp; // response from NM-Caesar
  obi_req_t  oecgra_context_mem_slave_req;
  obi_resp_t oecgra_context_mem_slave_rsp;

  // // Power Manager signals
  // logic cpu_subsystem_powergate_switch_n;
  // logic cpu_subsystem_powergate_switch_ack_n;
  // logic peripheral_subsystem_powergate_switch_n;
  // logic peripheral_subsystem_powergate_switch_ack_n;

  // External peripherals
  reg_req_t fll_req; // request to FLL subsystem
  reg_rsp_t fll_rsp; // response from FLL subsystem
  reg_req_t heepatia_ctrl_req; // request to heepatia controller
  reg_rsp_t heepatia_ctrl_rsp; // response from heepatia controller

  reg_req_t oecgra_config_regs_slave_req;
  reg_rsp_t oecgra_config_regs_slave_rsp;

  /* verilator lint_off UNUSED */
  reg_req_t heepatia_im2col_req; // request to heepatia coprosit controller
  reg_rsp_t heepatia_im2col_rsp; // response from heepatia coprosit controller

  // External SPC interface signals
  reg_req_t [AoSPCNum-1:0] ext_ao_peripheral_req;
  reg_rsp_t  [AoSPCNum-1:0] ext_ao_peripheral_resp;
  logic [DMACHNum-1:0] dma_busy;
  logic im2col_spc_done_int;
  /* verilator lint_on UNUSED */

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

  // External domains clock-gating
  logic [ExtDomainsRnd-1:0] external_subsystem_clkgate_en_n; //todo: one removed because it seems it is not using for

  // NM-Carus signals
  logic carus_rst_n;
  logic carus_set_retentive_n;
  logic carus_clkgate_n;
  // NM-Caesar signals
  logic caesar_rst_n;
  logic caesar_set_retentive_n;
  logic caesar_clkgate_n;
  // OECGRA signals
  logic oecgra_rst_n;
  logic oecgra_set_retentive_n;
  logic oecgra_clkgate_n;

  logic ext_debug_req;
  logic ext_debug_reset_n;

  // Tie the CV-X-IF coprocessor signals to a default value that will
  // receive petitions but reject all offloaded instructions
  // CV-X-IF is unused in core-v-mini-mcu as it has the cv32e40p CPU
  if_xif #() ext_if ();

  assign ext_if.compressed_ready = 1'b1;
  assign ext_if.compressed_resp  = '0;

  assign ext_if.issue_ready      = 1'b1;
  assign ext_if.issue_resp       = '0;

  assign ext_if.mem_valid        = 1'b0;
  assign ext_if.mem_req          = '0;

  assign ext_if.result_valid     = 1'b0;
  assign ext_if.result           = '0;

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
    .X_EXT           (CpuCorevXif),
    .AO_SPC_NUM      (AoSPCNum),
    .EXT_HARTS       (1)
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
    .xif_compressed_if (ext_if.cpu_compressed),
    .xif_issue_if      (ext_if.cpu_issue),
    .xif_commit_if     (ext_if.cpu_commit),
    .xif_mem_if        (ext_if.cpu_mem),
    .xif_mem_result_if (ext_if.cpu_mem_result),
    .xif_result_if     (ext_if.cpu_result),

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
    .ext_dma_read_req_o (heep_dma_read_req),
    .ext_dma_read_resp_i (heep_dma_read_rsp),
    .ext_dma_write_req_o (heep_dma_write_req),
    .ext_dma_write_resp_i (heep_dma_write_rsp),
    .ext_dma_addr_req_o (heep_dma_addr_req),
    .ext_dma_addr_resp_i (heep_dma_addr_rsp),

    // External peripherals slave ports
    .ext_peripheral_slave_req_o  (heep_peripheral_req),
    .ext_peripheral_slave_resp_i (heep_peripheral_rsp),

    // SPC signals
    .ext_ao_peripheral_slave_req_i(ext_ao_peripheral_req),
    .ext_ao_peripheral_slave_resp_o(ext_ao_peripheral_resp),

    // Power switches connected by the backend
    .cpu_subsystem_powergate_switch_no            (), // unused
    .cpu_subsystem_powergate_switch_ack_ni        (1'b1),
    .peripheral_subsystem_powergate_switch_no     (), // unused
    .peripheral_subsystem_powergate_switch_ack_ni (1'b1),

    // // Other power switches controlled here
    // .memory_subsystem_banks_powergate_switch_no(),
    // .memory_subsystem_banks_powergate_switch_ack_ni('1),

    .external_subsystem_powergate_switch_no(external_subsystem_powergate_switch_n),
    .external_subsystem_powergate_switch_ack_ni(external_subsystem_powergate_switch_ack_n),
    .external_subsystem_powergate_iso_no(external_subsystem_powergate_iso_n),

    // Control signals for external peripherals
    .external_subsystem_rst_no (external_subsystem_rst_n),
    .external_ram_banks_set_retentive_no (external_ram_banks_set_retentive_n),
    .external_subsystem_clkgate_en_no (external_subsystem_clkgate_en_n),

    // External interrupts
    .intr_vector_ext_i (ext_int_vector),

    .ext_dma_slot_tx_i('0),
    .ext_dma_slot_rx_i('0),

    .ext_debug_req_o(ext_debug_req),
    .ext_debug_reset_no(ext_debug_reset_n),
    .ext_cpu_subsystem_rst_no(),

    .ext_dma_stop_i('0),
    .dma_done_o(dma_busy),
    
    .exit_value_o (exit_value)
  );

  // assign cpu_subsystem_powergate_switch_ack_n = cpu_subsystem_powergate_switch_n;
  // assign peripheral_subsystem_powergate_switch_ack_n = peripheral_subsystem_powergate_switch_n;

  //todo: double check
  assign oecgra_rst_n            = external_subsystem_rst_n[0];
  assign oecgra_set_retentive_n  = external_ram_banks_set_retentive_n[0];
  assign oecgra_clkgate_n        = external_subsystem_clkgate_en_n[0];

  assign carus_rst_n            = external_subsystem_rst_n[1];
  assign carus_set_retentive_n  = external_ram_banks_set_retentive_n[1];
  assign carus_clkgate_n        = external_subsystem_clkgate_en_n[1];

  assign caesar_rst_n = external_subsystem_rst_n[2];
  assign caesar_set_retentive_n = external_ram_banks_set_retentive_n[2];
  assign caesar_clkgate_n        = external_subsystem_clkgate_en_n[2];




  // External peripherals
  // --------------------
  heepatia_peripherals u_heepatia_peripherals(
    .ref_clk_i                        (ref_clk_in_x),
    .rst_ni                           (rst_nin_sync),
    .system_clk_o                     (system_clk),
    .bypass_fll_i                     (bypass_fll_in_x),
    .carus_rst_ni                     (carus_rst_n),
    .carus_set_retentive_ni           (carus_set_retentive_n),
    .carus_req_i                      (carus_req),
    .carus_rsp_o                      (carus_rsp),
    .caesar_rst_ni          (caesar_rst_n),
    .caesar_set_retentive_ni(caesar_set_retentive_n),
    .caesar_req_i           (caesar_req),
    .caesar_rsp_o           (caesar_rsp),

    .oecgra_rst_ni                     (oecgra_rst_n),
    .oecgra_enable_i                   (oecgra_clkgate_n),
    .oecgra_master_req_o               (heepatia_master_req), //todo: double check
    .oecgra_master_resp_i              (heepatia_master_resp),
    .oecgra_config_regs_slave_req_i    (oecgra_config_regs_slave_req),
    .oecgra_config_regs_slave_rsp_o    (oecgra_config_regs_slave_rsp),
    .oecgra_context_mem_slave_req_i    (oecgra_context_mem_slave_req),
    .oecgra_context_mem_slave_rsp_o    (oecgra_context_mem_slave_rsp),
    .oecgra_context_mem_set_retentive_i(~oecgra_set_retentive_n),
    .fll_req_i                        (fll_req),
    .fll_rsp_o                        (fll_rsp),
    .heepatia_ctrl_req_i              (heepatia_ctrl_req),
    .heepatia_ctrl_rsp_o              (heepatia_ctrl_rsp),
    .im2col_spc_done_int_i            (im2col_spc_done_int),
    .ext_int_vector_o                 (ext_int_vector)
  );

  // // Co-prosit subsystem
  // // --------------------
  // cv32e40px_coprosit_ss #(
  //   .X_EXT     (CpuCorevXif),
  //   .COREV_PULP(CpuCorevPulp),
  //   .FPU       (CpuFpu),
  //   .ZFINX     (CpuRiscvZfinx)
  // ) u_cv32e40px_coprosit_ss (
  //   .clk_i            (system_clk),
  //   .rst_ni           (coprosit_reset_n && ext_debug_reset_n),
  //   .coprosit_clken_ni(coprosit_clken_n),
  //   .core_instr_req_o (heepatia_master_req[CoprositInstrIdx]),
  //   .core_instr_resp_i(heepatia_master_resp[CoprositInstrIdx]),
  //   .core_data_req_o  (heepatia_master_req[CoprositDataIdx]),
  //   .core_data_resp_i (heepatia_master_resp[CoprositDataIdx]),
  //   .irq_i            (coprosit_interrupts),
  //   .irq_ack_o        (),
  //   .irq_id_o         (),
  //   .debug_req_i      (ext_debug_req),
  //   .boot_addr_i      (coprosit_boot_addr),
  //   .dm_halt_addr_i   (coprosit_dm_halt_addr),
  //   .mtvec_addr_i     (coprosit_mtvec_addr),
  //   .fetch_enable_i   (coprosit_fetch_enable),
  //   .core_sleep_o     ()
  // );

  if (heepatia_pkg::Im2ColEnable == 1) begin : gen_im2col

    im2col_spc im2col_spc_i (
      .clk_i (system_clk),
      .rst_ni(rst_nin_sync),

      .aopb2im2col_resp_i(ext_ao_peripheral_resp[0]),
      .im2col2aopb_req_o (ext_ao_peripheral_req[0]),

      .reg_req_i(heepatia_im2col_req),
      .reg_rsp_o(heepatia_im2col_rsp),

      .dma_done_i(dma_busy),
      .im2col_spc_done_int_o(im2col_spc_done_int)
    );
  
  end else begin : gen_no_im2col
    
    assign heepatia_im2col_rsp = '0;
    assign ext_ao_peripheral_req[0] = '0;
    assign im2col_spc_done_int = '0;

  end

  // External peripherals bus
  // ------------------------
  heepatia_bus u_heepatia_bus (
    .clk_i                        (system_clk),
    .rst_ni                       (rst_nin_sync),
    .heep_core_instr_req_i        (heep_core_instr_req),
    .heep_core_instr_resp_o       (heep_core_instr_rsp),
    .heep_core_data_req_i         (heep_core_data_req),
    .heep_core_data_resp_o        (heep_core_data_rsp),
    .heep_debug_master_req_i      (heep_debug_master_req),
    .heep_debug_master_resp_o     (heep_debug_master_rsp),
    .heep_dma_read_req_i          (heep_dma_read_req),
    .heep_dma_read_resp_o         (heep_dma_read_rsp),
    .heep_dma_write_req_i         (heep_dma_write_req),
    .heep_dma_write_resp_o        (heep_dma_write_rsp),
    .heep_dma_addr_req_i          (heep_dma_addr_req),
    .heep_dma_addr_resp_o         (heep_dma_addr_rsp),
    .heepatia_master_req_i        (heepatia_master_req),
    .heepatia_master_resp_o       (heepatia_master_resp),
    .heep_slave_req_o             (heep_slave_req),
    .heep_slave_resp_i            (heep_slave_rsp),
    .carus_req_o                  (carus_req),
    .carus_resp_i                 (carus_rsp),
    .caesar_req_o           (caesar_req),
    .caesar_resp_i           (caesar_rsp),
    .oecgra_context_mem_slave_req_o(oecgra_context_mem_slave_req),
    .oecgra_context_mem_slave_rsp_i(oecgra_context_mem_slave_rsp),
    .oecgra_config_regs_slave_req_o(oecgra_config_regs_slave_req),
    .oecgra_config_regs_slave_rsp_i(oecgra_config_regs_slave_rsp),
    .heep_periph_req_i            (heep_peripheral_req),
    .heep_periph_resp_o           (heep_peripheral_rsp),
    .fll_req_o                    (fll_req),
    .fll_resp_i                   (fll_rsp),
    .heepatia_ctrl_req_o          (heepatia_ctrl_req),
    .heepatia_ctrl_resp_i         (heepatia_ctrl_rsp),
    .heepatia_im2col_req_o        (heepatia_im2col_req),
    .heepatia_im2col_resp_i       (heepatia_im2col_rsp)
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

  // CLOCK GATING PART WAS REMOVED FOR THESE MEMORY MODULES

  logic caesar_sw0_ctrl;
  logic caesar_sw0_ack, caesar_sw1_ack, caesar_sw2_ack, caesar_sw3_ack;

  logic carus_sw0_ctrl;
  logic carus_sw0_ack, carus_sw1_ack, carus_sw2_ack, carus_sw3_ack;

// `ifndef FPGA
//     assign caesar_sw0_ctrl = ~external_subsystem_powergate_switch_n[2];
//     assign external_subsystem_powergate_switch_ack_n[2] = ~caesar_sw3_ack;
// // Power switch and synchronizer
//     switch_cell_mem mem_caesar_sw0_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (caesar_sw0_ctrl),  // Switch Signal Input
//       .VCTRLFBn (),               // Negated Schmitt Trigger Output
//       .VCTRLFB  (),    // Schmitt Trigger Output
//       .VCTRL_BUF(caesar_sw0_ack)    //ACK signal Output
//     );

//     switch_cell_mem mem_caesar_sw1_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (caesar_sw0_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(caesar_sw1_ack)   //ACK signal Output
//     );

//     switch_cell_mem mem_caesar_sw2_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (caesar_sw1_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(caesar_sw2_ack)   //ACK signal Output
//     );

//     switch_cell_mem mem_caesar_sw3_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (caesar_sw2_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(caesar_sw3_ack)   //ACK signal Output
//     );

// `else

    assign caesar_sw0_ctrl = '0;
    assign external_subsystem_powergate_switch_ack_n[2] = '0;

// `endif

// `ifndef FPGA

//     assign carus_sw0_ctrl = ~external_subsystem_powergate_switch_n[1];
//     assign external_subsystem_powergate_switch_ack_n[1] = ~carus_sw3_ack;

//     // Power switch and synchronizer
//     switch_cell_mem mem_carus_sw0_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (carus_sw0_ctrl),  // Switch Signal Input
//       .VCTRLFBn (),               // Negated Schmitt Trigger Output
//       .VCTRLFB  (),    // Schmitt Trigger Output
//       .VCTRL_BUF(carus_sw0_ack)    //ACK signal Output
//     );

//     switch_cell_mem mem_carus_sw1_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (carus_sw0_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(carus_sw1_ack)   //ACK signal Output
//     );

//     switch_cell_mem mem_carus_sw2_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (carus_sw1_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(carus_sw2_ack)   //ACK signal Output
//     );

//     switch_cell_mem mem_carus_sw3_i (
//   `ifdef USE_PG_PIN
//       .VIN,
//       .VOUT,
//       .VSS,
//   `endif
//       .VCTRL    (carus_sw2_ack),  // Switch Signal Input
//       .VCTRLFBn (),              // Negated Schmitt Trigger Output
//       .VCTRLFB  (),   // Schmitt Trigger Output
//       .VCTRL_BUF(carus_sw3_ack)   //ACK signal Output
//     );

// `else

  assign carus_sw0_ctrl = '0;
  assign external_subsystem_powergate_switch_ack_n[1] = '0;

// `endif



endmodule // heepatia_top
