// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_top.sv
// Author: Michele Caon
// Date: 15/06/2023
// Description: heepatia testbench top-level module

module tb_top;
  // Include testbench utils
  `include "tb_util.svh"
`ifndef SYNTHESIS
  `include "nmc_tb_util.svh"
`endif
  import srec_pkg::*;
  import jtag_pkg::*;
  import riscv_pkg::*;

  // PARAMETERS
  parameter string VCD_BASENAME = "waves-";
  parameter string VCD_DIR = "logs/";

  // Reference clock (32.769kHz)
  const time REF_CLK_PERIOD = 30517ns;
  const time REF_CLK_PHASE_HI = REF_CLK_PERIOD / 2;
  const time REF_CLK_PHASE_LO = REF_CLK_PERIOD / 2;

  // Simulation clock (100MHz)
  const time SIM_CLK_PERIOD = 10ns;
  const time SIM_CLK_PHASE_HI = SIM_CLK_PERIOD / 2;
  const time SIM_CLK_PHASE_LO = SIM_CLK_PERIOD / 2;
  localparam int unsigned ClkFrequencykHz = 100_000;  // KHz

  // Timing delays
  const time         RESET_DEL = 1us;
  const int unsigned ResetWaitCycles = 50;

  // Watchdog heartbeat cycles
  localparam int unsigned WatchdogHeartbeatCycles = 100_000;

  // NM-Caesar last writeback cycles
  localparam int unsigned CaesarLatency = 5;

  // exit
  localparam int EXIT_SUCCESS = 0;
  localparam int EXIT_FAIL = 1;
  localparam int EXIT_ERROR = -1;

  // INTERNAL SIGNALS
  // ----------------
  // TB signals
  logic        ref_clk;
  logic        sim_clk;
  logic        tb_rst_n;

  // System clock and reset
  logic        sys_clk;
  logic        sys_rst_n;

  // Wires to DUT
  wire         ref_clk_w;
  wire         tb_rst_nw;
  wire         boot_select;
  wire         exec_from_flash;
  wire         bypass_fll;
  wire         exit_valid;
  wire  [31:0] exit_value;

  // TB configuration
  typedef enum int {
    BOOT_MODE_JTAG  = 0,
    BOOT_MODE_FLASH = 1,
    BOOT_MODE_FORCE = 2
  } boot_mode_t;
  int          vcd_mode_opt;  // 0: no dump, 1: unconditional dump, 2: triggered by GPIO[0]
  int          boot_mode;  // 0: wait JTAG, 1: boot from flash, 2: load SRAM
  string       boot_mode_opt = "jtag";
  int          exec_from_flash_opt;
  string       firmware_file_opt;
  int          bypass_fll_opt;
  int          verbose;
  int unsigned max_cycles;

  // Cycle count
  int unsigned cycle_cnt_q;

  // NM-Carus monitor
  typedef enum shortint {
    CARUS_CFG_START,
    CARUS_CFG_DONE,
    CARUS_CFG_FETCH_EN,
    CARUS_CFG_BOOT_PC
  } carus_cfg_field_t;

  event rst_event;

  // VCD dump
  typedef enum logic [1:0] {
    IDLE,
    DUMP_INIT,
    WAIT,
    DUMP_OFF
  } fsm_state_t;
  enum int unsigned {
    VCD_MODE_OFF  = 0,  // no dump
    VCD_MODE_ON   = 1,  // unconditional dump
    VCD_MODE_TRIG = 2   // triggered dump
  } vcd_mode_e;
  fsm_state_t curr_state, next_state;
  logic vcd_trigger;
  bit   vcd_cnt_en;
  string vcd_filename_d, vcd_filename_q;
  int unsigned vcd_cnt;

  // JTAG signals
  logic s_tck = 1'b0;
  logic s_trstn = 1'b0;
  logic s_tms = 1'b0;
  logic s_tdi = 1'b0;
  logic s_tdo;

  wire jtag_tck;
  wire jtag_trst_n;
  wire jtag_tms;
  wire jtag_tdi;
  wire jtag_tdo;


  int entry_point;
  int rd_cnt;
  int num_stim;
  // array for the stimulus vectors
  logic [95:0] stimuli[$];
  logic [255:0][31:0] jtag_data;

  jtag_pkg::debug_mode_if_t debug_mode_if = new;

  // modelsim exit code, will be overwritten when successfull
  int exit_value_jtag = EXIT_ERROR;
  int exit_valid_jtag = 0;


  // ----------------
  // TB CONFIGURATION
  // ----------------

  // Parse command-line arguments
  // ----------------------------
  initial begin : cmd_args
    firmware_file_opt   = "";
    vcd_mode_opt        = 0;
    boot_mode           = 0;
    exec_from_flash_opt = 0;
    bypass_fll_opt      = 0;
    verbose             = 0;
    max_cycles          = 0;

    // Firmware file
    if ($value$plusargs("firmware=%s", firmware_file_opt)) begin
      $display("[CONFIG] Firmware file: %s", firmware_file_opt);
    end else begin
      $fatal("ERR! No firmware file specified");
    end

    // VCD dump
    $value$plusargs("vcd_mode=%d", vcd_mode_opt);
    $display("[CONFIG] VCD dump mode: %0d", vcd_mode_opt);

    // Boot select
    $value$plusargs("boot_mode=%s", boot_mode_opt);
    case (boot_mode_opt)
      "0", "jtag": begin
        $display("[CONFIG] Boot mode: wait JTAG");
        boot_mode = BOOT_MODE_JTAG;
      end
      "1", "flash": begin
        $display("[CONFIG] Boot mode: boot from flash");
        boot_mode = BOOT_MODE_FLASH;
      end
      "2", "force": begin
        $display("[CONFIG] Boot mode: force SRAM signals");
        boot_mode = BOOT_MODE_FORCE;
      end
      default: begin
        $error("ERR! Invalid boot mode. Defualting to JTAG");
        boot_mode = BOOT_MODE_JTAG;
      end
    endcase

    // Execute from flash
    if (boot_mode == BOOT_MODE_FLASH) begin
      $value$plusargs("exec_from_flash_opt=%d", exec_from_flash_opt);
      $display("[CONFIG] Execute from flash: %s", (exec_from_flash_opt ? "yes" : "no"));
    end

`ifndef RTL_SIMULATION
    if (boot_mode == BOOT_MODE_FORCE) begin
      $fatal("ERR! Cannot run boot_mode force when not in RTL simulation. Use boot_mode flash instead.");
    end
`endif  //RTL_SIMULATION

    // Bypass FLL
    $value$plusargs("bypass_fll_opt=%d", bypass_fll_opt);
    $display("[CONFIG] Bypass FLL: %s", (bypass_fll_opt ? "yes" : "no"));

    // Verbose mode
    if ($test$plusargs("verbose")) begin
      $display("[CONFIG] Verbose mode enabled");
      verbose = 1;
    end

    // Max cycles
    if ($value$plusargs("max_cycles=%d", max_cycles)) begin
      $display("[CONFIG] Max cycles: %0d", max_cycles);
    end else begin
      $display("[CONFIG] Max cycles: unlimited");
    end
  end

  // Set timing format
  // -----------------
  initial begin : timing_format
    $timeformat(-9, 0, " ns", 9);
  end

  // -------------
  // LOAD FIRMWARE
  // -------------
  initial begin : load_firmware
    automatic srec_pkg::srec_record_t records[$];

    // Wait end of reset
    @(rst_event);
    @(posedge sys_clk);

    // Load firmware
    case (boot_mode)
      // Boot from JTAG
      // --------------
      BOOT_MODE_JTAG: begin
        // Adapted from HEEPocrates

        // Wait for the actual reset that is synchronized through 4 flip-flops (i.e., delayed by 4 cycles) to be deasserted
        // Wait 4+1 cycles to be safe before starting jtag transactions
        #(5 * REF_CLK_PERIOD);

        if (verbose) $display("[%t] starting JTAG...", $time);

        jtag_pkg::jtag_get_idcode(s_tck, s_tms, s_trstn, s_tdi, s_tdo);
        #5us;

        // Read in the stimuli vectors
        //
        // 1. *.srec Srecords is a standardized format to represent binary data
        // in ascii text format. Notably, it also encodes also the entry point
        // so we don't have to supply it manully with +ENTRY_POINT. GNU objcopy
        // (part of binutils) can easily convert and elf file to this format.
        $display("[TESTBENCH] %t - Loading Srecord from %s", $realtime, firmware_file_opt);
        srec_read(firmware_file_opt, records);
        srec_records_to_stimuli(records, stimuli, entry_point);

        $display("[TESTBENCH] %t - Init dmi access", $realtime);
        debug_mode_if.init_dmi_access(s_tck, s_tms, s_trstn, s_tdi);
        $display("[TESTBENCH] %t - Set dm access active", $realtime);
        debug_mode_if.set_dmactive(1'b1, s_tck, s_tms, s_trstn, s_tdi, s_tdo);

        $display("[TESTBENCH] %t - Loading memory via JTAG", $realtime);
        debug_mode_if.load_memory(num_stim, stimuli, s_tck, s_tms, s_trstn, s_tdi, s_tdo);

        // configure for debug module dmi access again
        debug_mode_if.init_dmi_access(s_tck, s_tms, s_trstn, s_tdi);
        // enable sb access for subsequent read/writeMem calls
        debug_mode_if.set_sbreadonaddr(1'b1, s_tck, s_tms, s_trstn, s_tdi, s_tdo);

        // From here on starts the actual jtag booting
        $display("[TESTBENCH] %t - Setting boot enable in ROM", $realtime);

        // SoC Controller boot exit loop value
        debug_mode_if.writeMem(core_v_mini_mcu_pkg::SOC_CTRL_START_ADDRESS + 32'hC, 32'd1, s_tck,
                               s_tms, s_trstn, s_tdi, s_tdo);

        #500us;

        // wait for end of computation signal
        $display("[TESTBENCH] %t - Waiting for end of computation", $realtime);

        rd_cnt = 0;
        jtag_data[0] = 0;
        while (jtag_data[0][0] == 0) begin
          // every 10th loop iteration, clear the debug module's SBA unit CSR to make
          // sure there's no error blocking our reads. Sometimes a TCDM read
          // request issued by the debug module takes longer than it takes
          // for the read request to the debug module to arrive and it
          // stores an error in the SBCS register. By clearing it
          // periodically we make sure the test can terminate.
          if (rd_cnt % 10 == 0) begin
            debug_mode_if.clear_sbcserrors(s_tck, s_tms, s_trstn, s_tdi, s_tdo);
          end
          // SoC Controller exit_valid address
          debug_mode_if.readMem(core_v_mini_mcu_pkg::SOC_CTRL_START_ADDRESS, jtag_data[0], s_tck,
                                s_tms, s_trstn, s_tdi, s_tdo);
          rd_cnt++;
          #50us;
        end

        // SoC Controller exit_value address
        debug_mode_if.readMem(core_v_mini_mcu_pkg::SOC_CTRL_START_ADDRESS + 32'h4, jtag_data[0],
                              s_tck, s_tms, s_trstn, s_tdi, s_tdo);

        exit_value_jtag = jtag_data[0][31:0];
        exit_valid_jtag = 1;

      end
      // Boot from SPI flash
      // -------------------
      BOOT_MODE_FLASH: begin
        if (verbose) $display("[%t] starting SPI flash...", $time);
      end
      // SRAM load
      // ---------
      BOOT_MODE_FORCE: begin
`ifdef RTL_SIMULATION
        $display("[%t] loading SRAM content...", $time);
        tb_loadHEX(firmware_file_opt);
        @(posedge sys_clk);
        $display("[%t] triggering boot loop exit...", $time);
        tb_set_exit_loop();
        $display("[%t] firmware loaded", $time);
`endif  //RTL_SIMULATION
      end
      default: begin
        $fatal("ERR! Invalid boot mode: %d", boot_mode);
      end
    endcase
  end

  // --------
  // VCD DUMP
  // --------
  // VCD dump FSM control signals
  assign vcd_trigger = u_tb_system.u_heepatia_top.gpio_0_io;

  // VCD dump FSM
  // ------------
  // FSM state progression
  always_comb begin : fsm_state_prog
    // Default values
    vcd_cnt_en     = 1'b0;
    vcd_filename_d = "";

    // State progression
    case (curr_state)
      IDLE: begin
        case (vcd_mode_opt)
          VCD_MODE_ON: next_state = DUMP_INIT;  // unconditional dump
          VCD_MODE_TRIG: begin  // triggered dump
            if (vcd_trigger) begin
              next_state = DUMP_INIT;
            end else begin
              next_state = IDLE;
            end
          end
          default:     next_state = IDLE;  // no dump
        endcase
      end
      DUMP_INIT: begin
        next_state     = WAIT;
        // Increment VCD sequence counter
        vcd_cnt_en     = 1'b1;
        // Create VCD file and start dumping
        vcd_filename_d = {VCD_DIR, VCD_BASENAME, $sformatf("%0d", vcd_cnt), ".vcd"};
        // NOTE: the following system tasks are specific to QuestaSim and not
        // part of the Verilog IEEE1364 standard, so may be not portable to
        // other simulators
        $fdumpfile(vcd_filename_d);
        $fdumpvars(0, u_tb_system.u_heepatia_top, vcd_filename_d);
        $display("[%t] VCD file initialized: %s", $time, vcd_filename_d);
        $display("[%t] VCD dump ON", $time);
      end
      WAIT: begin
        case (vcd_mode_opt)
          VCD_MODE_ON: begin  // unconditional dump
            next_state = WAIT;
          end
          VCD_MODE_TRIG: begin  // triggered dump
            if (vcd_trigger) begin
              next_state = WAIT;
            end else begin
              next_state = DUMP_OFF;
            end
          end
          default: next_state = IDLE;  // no dump
        endcase
      end
      DUMP_OFF: begin
        next_state = IDLE;
        $fdumpoff(vcd_filename_q);
        $display("[%t] VCD dump OFF", $time);
      end
      default: next_state = IDLE;
    endcase
  end

  // FSM state register
  always_ff @(posedge sys_clk or negedge sys_rst_n) begin : fsm_state_reg
    if (!sys_rst_n) curr_state <= IDLE;
    else curr_state <= next_state;
  end

  // VCD sequence counter
  always_ff @(posedge sys_clk or negedge sys_rst_n) begin : vcd_cnt_reg
    if (!sys_rst_n) vcd_cnt <= 0;
    else if (vcd_cnt_en) vcd_cnt <= vcd_cnt + 1;
  end

  // VCD filename register
  always_ff @(posedge sys_clk) begin : vcd_filename_reg
    if (vcd_cnt_en) vcd_filename_q <= vcd_filename_d;
  end

  // --------------------
  // TESTBENCH COMPONENTS
  // --------------------

  // Clock & reset generator
  // -----------------------
  initial begin : ref_clk_gen
    ref_clk = 1'b1;
    forever begin
      #REF_CLK_PHASE_HI ref_clk = 1'b0;
      #REF_CLK_PHASE_LO ref_clk = 1'b1;
    end
  end
  initial begin : sim_clk_gen
    sim_clk = 1'b1;
    forever begin
      #SIM_CLK_PHASE_HI sim_clk = 1'b0;
      #SIM_CLK_PHASE_LO sim_clk = 1'b1;
    end
  end
  initial begin : rst_gen
    tb_rst_n = 1'b1;

    // Wait a few cycles
    repeat (ResetWaitCycles) @(posedge ref_clk_w);

    tb_rst_n = 1'b0;

    // Wait a few cycles
    repeat (ResetWaitCycles) @(posedge ref_clk_w);

    // Release reset
    #RESET_DEL tb_rst_n = 1'b1;
    $display("[%t] Reset released", $time);

    #RESET_DEL -> rst_event;

  end

  // System clock and reset
  // ----------------------
  assign sys_clk   = u_tb_system.u_heepatia_top.u_heepatia_peripherals.system_clk_o;
  assign sys_rst_n = u_tb_system.u_heepatia_top.u_rstgen.rst_no;

  // TB Monitor
  // ----------
  // Watchdog counter
  always_ff @(posedge ref_clk_w or negedge tb_rst_n) begin : watchdog_cnt
    if (max_cycles != 0) begin
      if (~tb_rst_n) begin
        cycle_cnt_q <= 0;
      end else begin
        cycle_cnt_q <= cycle_cnt_q + 1;
        if (cycle_cnt_q >= max_cycles) begin
          $dumpoff;
          $fatal(2, "Simulation aborted due to maximum cycle limit");
        end else if (cycle_cnt_q % WatchdogHeartbeatCycles == 0) begin
          $display("[%t] Watchdog heartbeat: %0d cycles", $time, cycle_cnt_q);
        end
      end
    end
  end

  // NM-Carus execution time monitor
  bit carus_start, carus_prev_start = 0;
  bit carus_done, carus_prev_done = 0;
  longint carus_start_time = 0;
  longint carus_done_time = 0;
`ifndef SYNTHESIS
  always_ff @(posedge sys_clk) begin : carus_exec_mon
    carus_start      <= tb_get_carus_cfg(CARUS_CFG_START);
    carus_done       <= tb_get_carus_cfg(CARUS_CFG_DONE);

    carus_prev_start <= carus_start;
    carus_prev_done  <= carus_done;
    if (carus_start && !carus_prev_start) begin
      carus_start_time <= $time;
    end
    if (carus_done && !carus_prev_done) begin
      carus_done_time <= $time;
    end
  end
`endif

  // NM-Caesar execution time monitor (GPIO triggered)
  logic caesar_trig, caesar_prev_trig = 0;
  longint      caesar_start_time = 0;
  longint      caesar_done_time = 0;
  int unsigned caesar_ex_count = 0;
`ifndef SYNTHESIS
  always_ff @(posedge sys_clk) begin : caesar_exec_mon
    caesar_trig      <= tb_get_caesar_timer_trigger();
    caesar_prev_trig <= caesar_trig;

    if (caesar_trig && !caesar_prev_trig) begin
      caesar_start_time <= $time;
      $display("[%t] Caesar start", $time);
    end else if (!caesar_trig && caesar_prev_trig) begin
      caesar_done_time <= $time + CaesarLatency * SIM_CLK_PERIOD;  // add last writeback latency
      caesar_ex_count  <= caesar_ex_count + 1;
      $display("[%t] Caesar end", $time);
    end
  end
`endif

  // Exit monitor
  always_ff @(posedge sys_clk or negedge sys_rst_n) begin : exit_monitor
    if (exit_valid && (boot_mode == BOOT_MODE_FLASH || boot_mode == BOOT_MODE_FORCE)) begin
      if (exit_value == 0) begin
        $display("[%t] TEST SUCCEEDED", $time);
      end else begin
        $display("[%t] TEST FAILED", $time);
      end
      if (caesar_ex_count > 1) begin
        $display("[%t] WARNING: multiple Caesar executions detected. Cycle count is NOT accurate",
                 $time);
      end
      $display("[%t] - return value: %0d", $time, $signed(exit_value));
      $display("[%t] - NM-Carus kernel execution time: %0d ns", $time,
               carus_done_time - carus_start_time);
      $display("[%t] - NM-Caesar kernel execution time: %0d ns", $time,
               caesar_done_time - caesar_start_time);
      $dumpoff;
      if (exit_value == 0) begin
        $finish;
      end else begin
        $fatal(2, "[%t] TEST FAILED with value: %0d", $time, exit_value);
      end
    end else if (exit_valid_jtag && (boot_mode == BOOT_MODE_JTAG)) begin
      if (exit_value_jtag == 0) begin
        $display("[%t] TEST SUCCEEDED", $time);
      end else begin
        $display("[%t] TEST FAILED", $time);
      end
      $display("[%t] - return value: %0d", $time, $signed(exit_value_jtag));
      $dumpoff;
      if (exit_value_jtag == 0) begin
        $finish;
      end else begin
        $fatal(2, "[%t] TEST FAILED with value: %0d", $time, exit_value_jtag);
      end
    end
  end

  // Testbench system
  // ----------------
  // Set static onfiguration
  assign boot_select     = boot_mode == BOOT_MODE_FLASH;
  assign exec_from_flash = exec_from_flash_opt != 0;
  assign bypass_fll      = bypass_fll_opt != 0;

  // Clock and reset
  assign ref_clk_w       = (bypass_fll) ? sim_clk : ref_clk;
  assign tb_rst_nw       = tb_rst_n;

  assign jtag_tck             = s_tck;
  assign jtag_trst_n          = s_trstn;
  assign jtag_tms             = s_tms;
  assign jtag_tdi             = s_tdi;
  assign s_tdo                = jtag_tdo;

  // Instantiate TB system
  tb_system #(
      .CLK_FREQ(ClkFrequencykHz)
  ) u_tb_system (
      .ref_clk_i           (ref_clk_w),
      .rst_ni              (tb_rst_nw),
      .boot_select_i       (boot_select),
      .execute_from_flash_i(exec_from_flash),
      .bypass_fll_i        (bypass_fll),
      .exit_valid_o        (exit_valid),
      .exit_value_o        (exit_value),
      .jtag_tck_i          (jtag_tck),
      .jtag_trst_ni        (jtag_trst_n),
      .jtag_tms_i          (jtag_tms),
      .jtag_tdi_i          (jtag_tdi),
      .jtag_tdo_o          (jtag_tdo)
  );
endmodule
