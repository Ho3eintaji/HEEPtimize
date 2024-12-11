// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: nmc_tb_util.svh
// Author: Michele Caon
// Date: 06/07/2023
// Description: Testbench utility functions for NM-Carus

`ifndef NMC_TB_UTIL_SVH_
`define NMC_TB_UTIL_SVH_

`ifdef VERILATOR
`define TOP u_heepatia_top
`else
`define TOP u_tb_system.u_heepatia_top
`endif

// ------------------
// EXPORTED FUNCTIONS
// ------------------
// Get NM-Carus configuration registers value
export "DPI-C" function tb_get_carus_cfg;

// Get NM-Caesar execution timer GPIO trigger
export "DPI-C" function tb_get_caesar_timer_trigger;

// NM-Carus monitor functions
// --------------------------
typedef struct packed {
  logic start;
  logic done;
  logic fetch_en;
  logic [31:0] boot_pc;
} carus_cfg_t;

function int tb_get_carus_cfg(int unsigned field);
  carus_cfg_t cfg;
  cfg.start = `TOP.u_heepatia_peripherals.gen_carus[0].u_nm_carus_wrapper.u_carus_top.u_carus_ctl.u_carus_ctl_reg.start_o;
  cfg.done = `TOP.u_heepatia_peripherals.gen_carus[0].u_nm_carus_wrapper.u_carus_top.u_carus_ctl.u_carus_ctl_reg.done_o;
  cfg.fetch_en = `TOP.u_heepatia_peripherals.gen_carus[0].u_nm_carus_wrapper.u_carus_top.u_carus_ctl.u_carus_ctl_reg.fetch_en_o;
  cfg.boot_pc = `TOP.u_heepatia_peripherals.gen_carus[0].u_nm_carus_wrapper.u_carus_top.u_carus_ctl.u_carus_ctl_reg.boot_pc_o;

  case (field)
    0: return {31'h0, cfg.start};
    1: return {31'h0, cfg.done};
    2: return {31'h0, cfg.fetch_en};
    3: return cfg.boot_pc;
    default: return -1;
  endcase
endfunction : tb_get_carus_cfg

// NM-Caesar execution timer trigger
// ----------------------------------
function logic tb_get_caesar_timer_trigger();
  bit gpio_trigger;
  bit caesar_imc;
  bit caesar_cs;
  gpio_trigger = `TOP.gpio_1_io; // check GPIO pin number in "timer_util.h"
  caesar_imc   = `TOP.u_heepatia_peripherals.gen_caesar[0].u_nm_caesar_wrapper.u_caesar_top.imc_i;
  caesar_cs    = `TOP.u_heepatia_peripherals.gen_caesar[0].u_nm_caesar_wrapper.u_caesar_top.cs_i;
  return (caesar_imc & caesar_cs) | gpio_trigger;  // use GPIO trigger if toggled by software
endfunction : tb_get_caesar_timer_trigger


`endif  // NMC_TB_UTIL_SVH_
