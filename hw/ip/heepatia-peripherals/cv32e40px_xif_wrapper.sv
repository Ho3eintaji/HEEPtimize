// Copyright 2024 EPFL and Universidad Complutense de Madrid
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: cv32e40px_xif_wrapper.sv
// Author: David Mallasén
// Date: 17/09/2024
// Description: Wrapper for the cv32e40px core to route the XIF signals

module cv32e40px_xif_wrapper
  import cv32e40px_pkg::*;
#(
    parameter int unsigned X_EXT = 1,
    parameter int unsigned COREV_PULP = 1,
    parameter int unsigned FPU = 1,
    parameter int unsigned ZFINX = 0
) (
    input logic clk_i,
    input logic rst_ni,

    input logic pulp_clock_en_i,
    input logic scan_cg_en_i,

    input logic [31:0] boot_addr_i,
    input logic [31:0] mtvec_addr_i,
    input logic [31:0] dm_halt_addr_i,
    input logic [31:0] hart_id_i,
    input logic [31:0] dm_exception_addr_i,

    output logic        instr_req_o,
    input  logic        instr_gnt_i,
    input  logic        instr_rvalid_i,
    output logic [31:0] instr_addr_o,
    input  logic [31:0] instr_rdata_i,

    output logic        data_req_o,
    input  logic        data_gnt_i,
    input  logic        data_rvalid_i,
    output logic        data_we_o,
    output logic [ 3:0] data_be_o,
    output logic [31:0] data_addr_o,
    output logic [31:0] data_wdata_o,
    input  logic [31:0] data_rdata_i,

    if_xif.cpu_compressed xif_compressed_if,
    if_xif.cpu_issue      xif_issue_if,
    if_xif.cpu_commit     xif_commit_if,
    if_xif.cpu_mem        xif_mem_if,
    if_xif.cpu_mem_result xif_mem_result_if,
    if_xif.cpu_result     xif_result_if,

    input  logic [31:0] irq_i,
    output logic        irq_ack_o,
    output logic [ 4:0] irq_id_o,

    input  logic debug_req_i,
    output logic debug_havereset_o,
    output logic debug_running_o,
    output logic debug_halted_o,

    input  logic fetch_enable_i,
    output logic core_sleep_o
);

  cv32e40px_top #(
      .COREV_X_IF      (X_EXT),
      .COREV_PULP      (COREV_PULP),
      .COREV_CLUSTER   (0),
      .FPU             (FPU),
      .ZFINX           (ZFINX),
      .NUM_MHPMCOUNTERS(1)
  ) cv32e40px_top_i (
      .clk_i,
      .rst_ni,

      .pulp_clock_en_i,
      .scan_cg_en_i,

      .boot_addr_i,
      .mtvec_addr_i,
      .dm_halt_addr_i,
      .hart_id_i,
      .dm_exception_addr_i,

      .instr_req_o,
      .instr_gnt_i,
      .instr_rvalid_i,
      .instr_addr_o,
      .instr_rdata_i,

      .data_req_o,
      .data_gnt_i,
      .data_rvalid_i,
      .data_we_o,
      .data_be_o,
      .data_addr_o,
      .data_wdata_o,
      .data_rdata_i,

      // CORE-V-XIF
      // Compressed interface
      .x_compressed_valid_o(xif_compressed_if.compressed_valid),
      .x_compressed_ready_i(xif_compressed_if.compressed_ready),
      .x_compressed_req_o  (xif_compressed_if.compressed_req),
      .x_compressed_resp_i (xif_compressed_if.compressed_resp),

      // Issue Interface
      .x_issue_valid_o(xif_issue_if.issue_valid),
      .x_issue_ready_i(xif_issue_if.issue_ready),
      .x_issue_req_o  (xif_issue_if.issue_req),
      .x_issue_resp_i (xif_issue_if.issue_resp),

      // Commit Interface
      .x_commit_valid_o(xif_commit_if.commit_valid),
      .x_commit_o      (xif_commit_if.commit),

      // Memory Request/Response Interface
      .x_mem_valid_i(xif_mem_if.mem_valid),
      .x_mem_ready_o(xif_mem_if.mem_ready),
      .x_mem_req_i  (xif_mem_if.mem_req),
      .x_mem_resp_o (xif_mem_if.mem_resp),

      // Memory Result Interface
      .x_mem_result_valid_o(xif_mem_result_if.mem_result_valid),
      .x_mem_result_o      (xif_mem_result_if.mem_result),

      // Result Interface
      .x_result_valid_i(xif_result_if.result_valid),
      .x_result_ready_o(xif_result_if.result_ready),
      .x_result_i      (xif_result_if.result),

      .irq_i,
      .irq_ack_o,
      .irq_id_o,

      .debug_req_i,
      .debug_havereset_o,
      .debug_running_o,
      .debug_halted_o,

      .fetch_enable_i,
      .core_sleep_o
  );

endmodule
