// Copyright(// Copyright) 2022 EPFL
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

module fll_subsystem
  import reg_pkg::*;
(
    input logic ref_clk_i,
    input logic rst_ni,

    output logic system_clk_o,
    input  logic bypass_fll_i,

    //External peripheral(s)
    input  reg_req_t fll_slave_req_i,
    output reg_rsp_t fll_slave_resp_o

);
  //do not change, taken from apb_fll_if
  localparam int unsigned FllIfApbAddrWidth = 12;

  logic        fll_clk;

  logic        fll_req;
  logic        fll_wrn;
  logic [ 1:0] fll_add;
  logic [31:0] fll_data;
  logic        fll_ack;
  logic [31:0] fll_r_data;
  logic        fll_lock;
  logic test_mode, shift_enable;

  logic [FllIfApbAddrWidth-1:0] fll_paddr;
  logic [                 31:0] fll_pwdata;
  logic                         fll_pwrite;
  logic                         fll_psel;
  logic                         fll_penable;
  logic [                 31:0] fll_prdata;
  logic                         fll_pready;
  logic                         fll_pslverr;


  assign test_mode              = 1'b0;
  assign shift_enable           = 1'b0;

  assign fll_paddr              = fll_slave_req_i.addr[FllIfApbAddrWidth-1:0];
  assign fll_pwdata             = fll_slave_req_i.wdata;
  assign fll_pwrite             = fll_slave_req_i.write;
  assign fll_psel               = fll_slave_req_i.valid;
  assign fll_penable            = fll_slave_req_i.valid;

  assign fll_slave_resp_o.rdata = fll_prdata;
  assign fll_slave_resp_o.ready = fll_pready;
  assign fll_slave_resp_o.error = fll_pslverr;

  fll_wrapper u_fll_wrapper (
      .clk_o      (fll_clk),
      .oe_i       (1'b1),
      .ref_clk_i  (ref_clk_i),
      .lock_o     (fll_lock),
      .req_i      (fll_req),
      .ack_o      (fll_ack),
      .addr_i     (fll_add[1:0]),
      .wdata_i    (fll_data),
      .rdata_o    (fll_r_data),
      .wr_ni      (fll_wrn),
      .rst_ni     (rst_ni),
      .pwd_i      (1'b0),
      .test_mode_i(test_mode),
      .shift_en_i (shift_enable),
      .td_i       (1'b0),
      .tq_o       (),
      .jtd_i      (1'b0),
      .jtq_o      ()
  );

  pulp_clock_mux2 clk_mux_fll_soc_i (
      .clk0_i   (fll_clk),
      .clk1_i   (ref_clk_i),
      .clk_sel_i(bypass_fll_i),
      .clk_o    (system_clk_o)
  );

  apb_fll_if apb_fll_if_i (
      .HCLK   (system_clk_o),
      .HRESETn(rst_ni),
      .PADDR  (fll_paddr),
      .PWDATA (fll_pwdata),
      .PWRITE (fll_pwrite),
      .PSEL   (fll_psel),
      .PENABLE(fll_penable),
      .PRDATA (fll_prdata),
      .PREADY (fll_pready),
      .PSLVERR(fll_pslverr),

      .fll1_req_o    (fll_req),
      .fll1_wrn_o    (fll_wrn),
      .fll1_add_o    (fll_add),
      .fll1_data_o   (fll_data),
      .fll1_ack_i    (fll_ack),
      .fll1_r_data_i (fll_r_data),
      .fll1_lock_i   (fll_lock),
      // we only have 1 FLL
      .fll2_req_o    (),
      .fll2_wrn_o    (),
      .fll2_add_o    (),
      .fll2_data_o   (),
      .fll2_ack_i    ('0),
      .fll2_r_data_i ('0),
      .fll2_lock_i   ('0),
      .fll3_req_o    (),
      .fll3_wrn_o    (),
      .fll3_add_o    (),
      .fll3_data_o   (),
      .fll3_ack_i    ('0),
      .fll3_r_data_i ('0),
      .fll3_lock_i   ('0),
      .bbgen_req_o   (),
      .bbgen_wrn_o   (),
      .bbgen_sel_o   (),
      .bbgen_data_o  (),
      .bbgen_ack_i   ('0),
      .bbgen_r_data_i('0),
      .bbgen_lock_i  ('0)
  );



endmodule : fll_subsystem
