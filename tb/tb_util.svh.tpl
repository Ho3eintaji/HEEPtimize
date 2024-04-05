// Copyright 2022 OpenHW Group
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

// HEEPerator top-level
`ifdef VERILATOR
`define TOP u_heeperator_top
`else
`define TOP u_tb_system.u_heeperator_top
`endif

// Task for loading 'mem' with SystemVerilog system task $readmemh()
export "DPI-C" task tb_readHEX;
export "DPI-C" task tb_loadHEX;
export "DPI-C" task tb_getMemSize;
export "DPI-C" task tb_set_exit_loop;

import core_v_mini_mcu_pkg::*;

function int tb_check_if_any_not_X(logic [31:0] input_word);
  for(int unsigned i = 0; i < 32; i=i+1) begin
    if ( input_word[i] !== 1'bx ) return 1;
  end
  return 0;
endfunction


task tb_getMemSize;
  output int mem_size;
  output int num_banks;
  mem_size  = core_v_mini_mcu_pkg::MEM_SIZE;
  num_banks = core_v_mini_mcu_pkg::NUM_BANKS;
endtask

task tb_readHEX;
  input string file;
  output logic [7:0] stimuli[core_v_mini_mcu_pkg::MEM_SIZE];
  $readmemh(file, stimuli);
endtask

task tb_loadHEX;
  input string file;
  //whether to use debug to write to memories
  logic [7:0] stimuli[core_v_mini_mcu_pkg::MEM_SIZE];
  int i, stimuli_counter, bank, num_bytes, num_banks;
  logic [31:0] addr;
  logic [31:0] word;
  int write_res;

  tb_readHEX(file, stimuli);
  tb_getMemSize(num_bytes, num_banks);

`ifdef LOADHEX_DBG

`ifdef VERILATOR
  $fatal("ERR! LOADHEX_DBG not supported in Verilator");
`endif

  for (i = 0; i < num_bytes; i = i + 4) begin

    if( tb_check_if_any_not_X({stimuli[i+3], stimuli[i+2], stimuli[i+1], stimuli[i]}) ) begin
      @(posedge `TOP.u_core_v_mini_mcu.clk_i);
      addr = i;
      #1;
      // write to memory
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o = 1'b1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_addr_o = addr;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_we_o = 1'b1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_be_o = 4'b1111;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_wdata_o = {
        stimuli[i+3], stimuli[i+2], stimuli[i+1], stimuli[i]
      };

      while(!`TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_gnt_i) begin
        @(posedge `TOP.u_core_v_mini_mcu.clk_i);
      end

      #1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o = 1'b0;

      wait (`TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_rvalid_i);

      #1;
    end

  end

  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_addr_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_we_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_be_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_wdata_o;

`else // LOADHEX_DBG

`ifndef RTL_SIMULATION
  @(posedge `TOP.u_core_v_mini_mcu.clk_i);
  #1;
`endif // RTL_SIMULATION

  for (i = 0; i < num_bytes / num_banks; i += 4) begin
    addr = i / 4;
    write_res = 0;

% for bank in range(ram_numbanks_cont):
    assign word = {
      stimuli[${bank*32*1024}+i+3],
      stimuli[${bank*32*1024}+i+2],
      stimuli[${bank*32*1024}+i+1],
      stimuli[${bank*32*1024}+i]
    };
    write_res += writeSram${bank}(addr, word);
% endfor
% if ram_numbanks_il != 0:
% for bank in range(ram_numbanks_il):
    assign word = {
      stimuli[${int(ram_numbanks_cont)*32*1024}+i*${ram_numbanks_il}+${bank*4}+3],
      stimuli[${int(ram_numbanks_cont)*32*1024}+i*${ram_numbanks_il}+${bank*4}+2],
      stimuli[${int(ram_numbanks_cont)*32*1024}+i*${ram_numbanks_il}+${bank*4}+1],
      stimuli[${int(ram_numbanks_cont)*32*1024}+i*${ram_numbanks_il}+${bank*4}]
    };
    write_res += writeSram${int(ram_numbanks_cont) + bank}(addr, word);
% endfor
% endif

`ifndef RTL_SIMULATION
    if (write_res < ${ram_numbanks}) begin
      @(posedge `TOP.u_core_v_mini_mcu.clk_i);
      #1;
    end
`endif // RTL_SIMULATION
  end

`ifndef VERILATOR
  // Release memory signals
% for bank in range(ram_numbanks):
  releaseSram${bank}();
% endfor

`endif // VERILATOR

`endif // LOADHEX_DBG
endtask

`ifdef RTL_SIMULATION

% for bank in range(ram_numbanks):
function int writeSram${bank}(int unsigned addr, logic [31:0] data);
  `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram[${bank}].ram_i.tc_ram_i.sram[addr] = data;
  return 0;
endfunction: writeSram${bank}
% endfor

`ifndef VERILATOR

% for bank in range(ram_numbanks):
function void releaseSram${bank}();
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram[${bank}].ram_i.tc_ram_i.sram;
endfunction: releaseSram${bank}
% endfor

`endif // VERILATOR

`else // RTL_SIMULATION

// TODO: find a better, implementation-independent way to write SRAMs in
// post-syntheis/post-layout. Although these routines directly write the
// interface signals of the SRAM models, it may happen that these signals get
// inverted at some implementation stage.
% for bank in range(ram_numbanks):
function int writeSram${bank}(int unsigned addr, logic [31:0] data);
  if (!tb_check_if_any_not_X(data)) begin
    force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.CEN = 1'b1;
    return 1;
  end
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.CEN = 1'b0;
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.WEN = 32'h0;
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.GWEN = 1'b0;
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.A = addr[12:0];
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.D = data;
  return 0;
endfunction: writeSram${bank}
% endfor

// Release memory signals
% for bank in range(ram_numbanks):
function void releaseSram${bank}();
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.CEN;
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.WEN;
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.GWEN;
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.A;
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.gen_sram_${bank}__ram_i.gen_32kB_mem_mem_bank.D;
endfunction: releaseSram${bank}
% endfor

`endif // VERILATOR

task tb_set_exit_loop;
`ifdef VCS
  force `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
  release `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0];
`elsif VERILATOR
  `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
`else
  `ifndef SYNTHESIS
    `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
  `else
    // careful this code may change after synthesis
    while ( 1 ) begin
      // NOTE: do not remove the following line. It prevents the wait()
      // statement from detecting the request on a clock edge, that results in
      // the rdata signal being set for the *next* cycle instead of the current
      // one.
      #1;
      wait ( `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_req_i[69] );  //this should be the valid signals
      if ( `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_req_i[35:34] == 2'b11 ) begin //this should be the address 2000_000C
        force `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_rsp_o[0] = 1'b1;
        @(posedge `TOP.u_core_v_mini_mcu.clk_i);
        release `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_rsp_o;
        break;
      end else @(posedge `TOP.u_core_v_mini_mcu.clk_i);
    end
  `endif
`endif
endtask
