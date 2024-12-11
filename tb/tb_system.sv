// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_system.sv
// Author: Michele Caon
// Date: 15/06/2023
// Description: heepatia testbench system

module tb_system #(
    parameter int unsigned CLK_FREQ = 32'd100_000  // kHz
) (
    inout logic ref_clk_i,
    inout logic rst_ni,

    // Static configuration
    inout logic boot_select_i,
    inout logic execute_from_flash_i,
    inout logic bypass_fll_i,

    inout logic jtag_tck_i,
    inout logic jtag_tms_i,
    inout logic jtag_trst_ni,
    inout logic jtag_tdi_i,
    inout logic jtag_tdo_o,

    // Exit signals
    inout logic        exit_valid_o,
    inout logic [31:0] exit_value_o
);
  // Include testbench utils
  `include "tb_util.svh"
`ifndef SYNTHESIS
  `include "nmc_tb_util.svh"
`endif

  // INTERNAL SIGNALS
  // ----------------

  // UART
  wire       heepatia_uart_tx;
  wire       heepatia_uart_rx;

  // GPIO
  wire [6:0] gpio;

  // SPI flash
  wire       spi_flash_sck;
  wire [1:0] spi_flash_csb;
  wire [3:0] spi_flash_sd_io;

  // SPI
  wire       spi_sck;
  wire [1:0] spi_csb;
  wire [3:0] spi_sd_io;

  // GPIO
  wire       clk_div;

  // UART DPI emulator
  uartdpi #(
      .BAUD('d256000),
      .FREQ(CLK_FREQ * 1000),  // Hz
      .NAME("uart")
  ) u_uartdpi (
      .clk_i (u_heepatia_top.u_heepatia_peripherals.system_clk_o),
      .rst_ni(rst_ni),
      .tx_o  (heepatia_uart_rx),
      .rx_i  (heepatia_uart_tx)
  );

  // SPI flash emulator
`ifndef VERILATOR
  spiflash u_flash_boot (
      .csb(spi_flash_csb[0]),
      .clk(spi_flash_sck),
      .io0(spi_flash_sd_io[0]),
      .io1(spi_flash_sd_io[1]),
      .io2(spi_flash_sd_io[2]),
      .io3(spi_flash_sd_io[3])
  );

  spiflash u_flash_device (
      .csb(spi_csb[0]),
      .clk(spi_sck),
      .io0(spi_sd_io[0]),
      .io1(spi_sd_io[1]),
      .io2(spi_sd_io[2]),
      .io3(spi_sd_io[3])
  );
`endif  /* VERILATOR */

  gpio_cnt #(
      .CntMax(32'd16)
  ) u_test_gpio (
      .clk_i (ref_clk_i),
      .rst_ni(rst_ni),
      .gpio_i(gpio[5]),
      .gpio_o(gpio[6])
  );

`ifdef USE_PG_PIN
  import UPF::*;

  supply1 VDD;
  supply0 VSS;

  initial begin
    $display("%t: All Power Supply ON (USE_PG_PIN)", $time);
    supply_on("VDD", 1.2);
    supply_on("VSS", 0);
  end

`endif

`ifdef RTL_SIMULATION

  localparam int unsigned SwitchAckLatency = 45;

  //pretending to be SWITCH CELLs that delay by SwitchAckLatency cycles the ACK signal
  // This is done only for HEEP memories as CPU and Peripherals ACKs are hardwired in HEEPatia top level
  // and Carus and CGRAs memories simulation models delay the ACK signal by 1 cycles

  logic [core_v_mini_mcu_pkg::NUM_BANKS-1:0] tb_memory_subsystem_banks_powergate_switch_ack_n[SwitchAckLatency+1];
  logic [core_v_mini_mcu_pkg::NUM_BANKS-1:0] delayed_tb_memory_subsystem_banks_powergate_switch_ack_n;

  always_ff @(negedge u_heepatia_top.u_heepatia_peripherals.system_clk_o) begin
    tb_memory_subsystem_banks_powergate_switch_ack_n[0] <= u_heepatia_top.u_core_v_mini_mcu.memory_subsystem_banks_powergate_switch_n;
    for (int i = 0; i < SwitchAckLatency; i++) begin
      tb_memory_subsystem_banks_powergate_switch_ack_n[i+1] <= tb_memory_subsystem_banks_powergate_switch_ack_n[i];
    end
  end

  assign delayed_tb_memory_subsystem_banks_powergate_switch_ack_n = tb_memory_subsystem_banks_powergate_switch_ack_n[SwitchAckLatency];

  always_comb begin
`ifndef VERILATOR
    force u_heepatia_top.u_core_v_mini_mcu.memory_subsystem_banks_powergate_switch_ack_n = delayed_tb_memory_subsystem_banks_powergate_switch_ack_n;
`else
    u_heepatia_top.u_core_v_mini_mcu.memory_subsystem_banks_powergate_switch_ack_n = delayed_tb_memory_subsystem_banks_powergate_switch_ack_n;
`endif
  end

`endif

  // DUT
  // ---
  heepatia_top u_heepatia_top (
`ifdef USE_PG_PIN
      .VSS,
      .VDD,
`endif
      .rst_ni              (rst_ni),
      .boot_select_i       (boot_select_i),
      .execute_from_flash_i(execute_from_flash_i),
      .jtag_tck_i          (jtag_tck_i),
      .jtag_tms_i          (jtag_tms_i),
      .jtag_trst_ni        (jtag_trst_ni),
      .jtag_tdi_i          (jtag_tdi_i),
      .jtag_tdo_o          (jtag_tdo_o),
      .uart_rx_i           (heepatia_uart_rx),
      .uart_tx_o           (heepatia_uart_tx),
      .exit_valid_o        (exit_valid_o),
      .gpio_0_io           (gpio[0]),
      .gpio_1_io           (gpio[1]),
      .gpio_2_io           (gpio[2]),
      .gpio_3_io           (gpio[3]),
      .gpio_4_io           (gpio[4]),
      .gpio_5_io           (gpio[5]),
      .gpio_6_io           (gpio[6]),
      .spi_flash_sck_io    (spi_flash_sck),
      .spi_flash_cs_0_io   (spi_flash_csb[0]),
      .spi_flash_cs_1_io   (spi_flash_csb[1]),
      .spi_flash_sd_0_io   (spi_flash_sd_io[0]),
      .spi_flash_sd_1_io   (spi_flash_sd_io[1]),
      .spi_flash_sd_2_io   (spi_flash_sd_io[2]),
      .spi_flash_sd_3_io   (spi_flash_sd_io[3]),
      .spi_sck_io          (spi_sck),
      .spi_cs_0_io         (spi_csb[0]),
      .spi_cs_1_io         (spi_csb[1]),
      .spi_sd_0_io         (spi_sd_io[0]),
      .spi_sd_1_io         (spi_sd_io[1]),
      .spi_sd_2_io         (spi_sd_io[2]),
      .spi_sd_3_io         (spi_sd_io[3]),
      .i2s_sck_io          (),
      .i2s_ws_io           (),
      .i2s_sd_io           (),
      .ref_clk_i           (ref_clk_i),
      .bypass_fll_i        (bypass_fll_i),
      .fll_clk_div_o       (clk_div),
      .exit_value_o        (exit_value_o[0]),
      .CLK_SLOW_i          (),
      .i2c_scl_io          (),
      .i2c_sda_io          ()
  );

  // Exit value
  assign exit_value_o[31:1] = u_heepatia_top.u_core_v_mini_mcu.exit_value_o[31:1];
endmodule
