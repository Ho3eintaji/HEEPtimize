// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia_pkg.sv
// Author: Michele Caon, Luigi Giuffrida, Hossein Taji
// Date: 29/04/2024
// Description: Package containing memory map and other definitions.

package heepatia_pkg;
  import addr_map_rule_pkg::*;
  import core_v_mini_mcu_pkg::*;

  // ---------------
  // CORE-V-MINI-MCU
  // ---------------

  // CPU
  localparam int unsigned CpuCorevPulp = 32'd${cpu_corev_pulp};
  localparam int unsigned CpuCorevXif = 32'd${cpu_corev_xif};
  localparam int unsigned CpuFpu = 32'd${cpu_fpu};
  localparam int unsigned CpuRiscvZfinx = 32'd${cpu_riscv_zfinx};

  // ----------------
  // EXTERNAL OBI BUS
  // ----------------

  // Number of masters and slaves
  localparam int unsigned ExtXbarNMaster = 32'd${xbar_nmasters};
  localparam int unsigned ExtXbarNSlave = 32'd${xbar_nslaves};
  localparam int unsigned LogExtXbarNMaster = ExtXbarNMaster > 32'd1 ? $clog2(ExtXbarNMaster) : 32'd1;
  localparam int unsigned LogExtXbarNSlave = ExtXbarNSlave > 32'd1 ? $clog2(ExtXbarNSlave) : 32'd1;

  localparam int unsigned OecgraMasterIdx   = 32'd2;

  // Memory map
  // ----------

  // OECGRA
  localparam int unsigned OecgraIdx = 32'd0;
  localparam logic [31:0] OecgraStartAddr = EXT_SLAVE_START_ADDRESS + 32'h${oecgra_start_address};
  localparam logic [31:0] OecgraEndAddr = OecgraStartAddr + 32'h${oecgra_size};


  // NM-Carus
  localparam int unsigned CarusNum = 32'd${carus_num};
  localparam int unsigned LogCarusNum = CarusNum > 32'd1 ? $clog2(CarusNum) : 32'd1;
% for inst in range(carus_num):
  localparam int unsigned Carus${inst}NumBanks = 32'd${carus_num_banks[inst]};
  localparam int unsigned Carus${inst}BankAddrWidth = 32'd${carus_bank_addr_width[inst]};
  localparam int unsigned Carus${inst}Idx = 32'd${inst + 1};
% if inst == 0:
  localparam logic [31:0] Carus${inst}StartAddr = EXT_SLAVE_START_ADDRESS + 32'h${carus_start_address[inst]};
% else:
  localparam logic [31:0] Carus${inst}StartAddr = Carus${inst - 1}EndAddr;
% endif
  localparam logic [31:0] Carus${inst}EndAddr = Carus${inst}StartAddr + 32'h${carus_size[inst]};
% endfor

  localparam int unsigned InstancesNumBanks[CarusNum] = '{${', '.join([str(carus_num_banks[inst]) for inst in range(carus_num)])}};
  localparam int unsigned InstancesBankAddrWidth[CarusNum] = '{${', '.join([str(carus_bank_addr_width[inst]) for inst in range(carus_num)])}};

  // External slaves address map
  localparam addr_map_rule_t [ExtXbarNSlave-1:0] ExtSlaveAddrRules = '{
    '{idx: OecgraIdx, start_addr: OecgraStartAddr, end_addr: OecgraEndAddr},
  % for inst in range(carus_num-1, 0, -1):
    '{idx: Carus${inst}Idx, start_addr: Carus${inst}StartAddr, end_addr: Carus${inst}EndAddr},
  % endfor
    '{idx: Carus${0}Idx, start_addr: Carus${0}StartAddr, end_addr: Carus${0}EndAddr}
  };
  localparam int unsigned ExtSlaveDefaultIdx = 32'd0;

  // ------------------
  // EXTERNAL OECGRA BUS
  // ------------------

  // OECGRA context memory
  localparam int unsigned OecgraContextMemIdx = 32'd0;
  localparam logic [31:0] OecgraContextMemStartAddr = EXT_SLAVE_START_ADDRESS + 32'h${oecgra_context_mem_start_address};
  localparam logic [31:0] OecgraContextMemEndAddr = OecgraContextMemStartAddr + 32'h${oecgra_context_mem_size};

  // OECGRA configuration registers
  localparam int unsigned OecgraConfigRegsIdx = 32'd1;
  localparam logic [31:0] OecgraConfigRegsStartAddr = EXT_SLAVE_START_ADDRESS + 32'h${oecgra_config_regs_start_address};
  localparam logic [31:0] OecgraConfigRegsEndAddr = OecgraConfigRegsStartAddr + 32'h${oecgra_config_regs_size};

  // OECGRA slaves address map
  localparam addr_map_rule_t [1:0] OecgraSlaveAddrRules = '{
    '{idx: OecgraContextMemIdx, start_addr: OecgraContextMemStartAddr, end_addr: OecgraContextMemEndAddr},
    '{idx: OecgraConfigRegsIdx, start_addr: OecgraConfigRegsStartAddr, end_addr: OecgraConfigRegsEndAddr}
  };

  
  // --------------------
  // EXTERNAL PERIPHERALS
  // --------------------

  // Number of external peripherals
  localparam int unsigned ExtPeriphNSlave = 32'd${periph_nslaves};
  localparam int unsigned LogExtPeriphNSlave = ExtPeriphNSlave > 32'd1 ? $clog2(ExtPeriphNSlave) : 32'd1;

  // Memory map
  // ----------
  // FLL
  localparam int unsigned FLLIdx = 32'd0;
  localparam logic [31:0] FLLStartAddr = EXT_PERIPHERAL_START_ADDRESS + 32'h${fll_start_address};
  localparam logic [31:0] FLLEndAddr = FLLStartAddr + 32'h${fll_size};

 // heepatia external subsystem control
  localparam int unsigned HeepatiaCtrlIdx = 32'd1;
  localparam logic [31:0] HeepatiaCtrlStartAddr = EXT_PERIPHERAL_START_ADDRESS + 32'h${heepatia_ctrl_start_address};
  localparam logic [31:0] HeepatiaCtrlEndAddr = HeepatiaCtrlStartAddr + 32'h${heepatia_ctrl_size};

  // External peripherals address map
  localparam addr_map_rule_t [ExtPeriphNSlave-1:0] ExtPeriphAddrRules = '{
    '{idx: FLLIdx, start_addr: FLLStartAddr, end_addr: FLLEndAddr},
    '{idx: HeepatiaCtrlIdx, start_addr: HeepatiaCtrlStartAddr, end_addr: HeepatiaCtrlEndAddr}
  };
endpackage
