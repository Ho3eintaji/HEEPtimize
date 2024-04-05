// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_system.sv
// Author: Michele Caon
// Date: 15/06/2023
// Description: HEEPerator testbench system

module tb_system #(
  parameter int unsigned CLK_FREQ = 32'd100_000  // kHz
) (
  inout logic ref_clk_i,
  inout logic rst_ni,

  // Static configuration
  inout logic boot_select_i,
  inout logic execute_from_flash_i,
  inout logic bypass_fll_i,

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
  // JTAG
  wire jtag_tck = '0;
  wire jtag_tms = '0;
  wire jtag_trst_n = '0;
  wire jtag_tdi = '0;
  wire jtag_tdo = '0;

  // UART
  wire                          heeperator_uart_tx;
  wire                          heeperator_uart_rx;

  // GPIO
  wire                   [31:0] gpio;

  // SPI flash
  wire                          spi_flash_sck;
  wire                   [ 1:0] spi_flash_csb;
  wire                   [ 3:0] spi_flash_sd_io;

  // SPI
  wire                          spi_sck;
  wire                   [ 1:0] spi_csb;
  wire                   [ 3:0] spi_sd_io;

  // GPIO
  wire                          clk_div;

  // UART DPI emulator
  uartdpi #(
    .BAUD('d256000),
    .FREQ(CLK_FREQ * 1000),  // Hz
    .NAME("uart")
  ) u_uartdpi (
    .clk_i (u_heeperator_top.u_heeperator_peripherals.system_clk_o),
    .rst_ni(rst_ni),
    .tx_o  (heeperator_uart_rx),
    .rx_i  (heeperator_uart_tx)
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
    .gpio_i(gpio[30]),
    .gpio_o(gpio[31])
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

  // DUT
  // ---
  heeperator_top u_heeperator_top (
`ifdef USE_PG_PIN
    .VSS,
    .VDD,
`endif
    .rst_ni              (rst_ni),
    .boot_select_i       (boot_select_i),
    .execute_from_flash_i(execute_from_flash_i),
    .jtag_tck_i          (jtag_tck),
    .jtag_tms_i          (jtag_tms),
    .jtag_trst_ni        (jtag_trst_n),
    .jtag_tdi_i          (jtag_tdi),
    .jtag_tdo_o          (jtag_tdo),
    .uart_rx_i           (heeperator_uart_rx),
    .uart_tx_o           (heeperator_uart_tx),
    .exit_valid_o        (exit_valid_o),
    .gpio_0_io           (gpio[0]),
    .gpio_1_io           (gpio[1]),
    .gpio_2_io           (gpio[2]),
    .gpio_3_io           (gpio[3]),
    .gpio_4_io           (gpio[4]),
    .gpio_5_io           (gpio[5]),
    .gpio_6_io           (gpio[6]),
    .gpio_7_io           (gpio[7]),
    .gpio_8_io           (gpio[8]),
    .gpio_9_io           (gpio[9]),
    .gpio_10_io          (gpio[10]),
    .gpio_11_io          (gpio[11]),
    .gpio_12_io          (gpio[12]),
    .gpio_13_io          (gpio[13]),
    .gpio_14_io          (gpio[14]),
    .gpio_15_io          (gpio[15]),
    .gpio_16_io          (gpio[16]),
    .gpio_17_io          (gpio[17]),
    .gpio_18_io          (gpio[18]),
    .gpio_19_io          (gpio[19]),
    .gpio_20_io          (gpio[20]),
    .gpio_21_io          (gpio[21]),
    .gpio_22_io          (gpio[22]),
    .gpio_23_io          (gpio[23]),
    .gpio_24_io          (gpio[24]),
    .gpio_25_io          (gpio[25]),
    .gpio_26_io          (gpio[26]),
    .gpio_27_io          (gpio[27]),
    .gpio_28_io          (gpio[28]),
    .gpio_29_io          (gpio[29]),
    .gpio_30_io          (gpio[30]),
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
    .exit_value_o        (exit_value_o[0])
  );

  // Exit value
  assign exit_value_o[31:1] = u_heeperator_top.u_core_v_mini_mcu.exit_value_o[31:1];
endmodule
