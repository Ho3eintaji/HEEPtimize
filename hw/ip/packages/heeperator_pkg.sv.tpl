// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heeperator_pkg.sv
// Author: Michele Caon, Luigi Giuffrida
// Date: 29/04/2024
// Description: Package containing memory map and other definitions.

package heeperator_pkg;
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

  // Memory map
  // ----------

  // NM-Carus
  localparam int unsigned CarusNum = 32'd${carus_num};
  localparam int unsigned LogCarusNum = CarusNum > 32'd1 ? $clog2(CarusNum) : 32'd1;
  localparam int unsigned CarusNumBanks = 32'd${carus_num_banks};
  localparam int unsigned CarusBankAddrWidth = 32'd${carus_bank_addr_width};
% for inst in range(carus_num):
  localparam int unsigned Carus${inst}Idx = 32'd${inst};
  localparam logic [31:0] Carus${inst}StartAddr = EXT_SLAVE_START_ADDRESS + 32'h${carus_start_address};
  localparam logic [31:0] Carus${inst}EndAddr = Carus${inst}StartAddr + 32'h${carus_size};
% endfor

  // Near-memory computing IPs address map
  localparam addr_map_rule_t [ExtXbarNSlave-1:0] ExtSlaveAddrRules = '{
  % for inst in range(carus_num-1):
    '{idx: Carus${inst}Idx, start_addr: Carus${inst}StartAddr, end_addr: Carus${inst}EndAddr}
  % endfor
    '{idx: Carus${carus_num-1}Idx, start_addr: Carus${carus_num-1}StartAddr, end_addr: Carus${carus_num-1}EndAddr}
  };
  localparam int unsigned ExtSlaveDefaultIdx = 32'd0;

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

  // HEEPerator external subsystem control
  localparam int unsigned HeeperatorCtrlIdx = 32'd1;
  localparam logic [31:0] HeeperatorCtrlStartAddr = EXT_PERIPHERAL_START_ADDRESS + 32'h${heeperator_ctrl_start_address};
  localparam logic [31:0] HeeperatorCtrlEndAddr = HeeperatorCtrlStartAddr + 32'h${heeperator_ctrl_size};

  // External peripherals address map
  localparam addr_map_rule_t [ExtPeriphNSlave-1:0] ExtPeriphAddrRules = '{
    '{idx: FLLIdx, start_addr: FLLStartAddr, end_addr: FLLEndAddr},
    '{idx: HeeperatorCtrlIdx, start_addr: HeeperatorCtrlStartAddr, end_addr: HeeperatorCtrlEndAddr}
  };
endpackage
