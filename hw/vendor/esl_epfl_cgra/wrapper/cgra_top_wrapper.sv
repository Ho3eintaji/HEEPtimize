
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Author:         Benoît Denkinger - benoit.denkinger@epfl.ch                //
//                                                                            //
// Additional contributions by:                                               //
//                 Simone Machetti - simone.machetti@epfl.ch                  //
//                                                                            //
// Design Name:    cgra_top_wrapper                                           //
//                                                                            //
// Project Name:   HealWear-V                                                 //
//                                                                            //
// Language:       SystemVerilog                                              //
//                                                                            //
// Description:    CGRA top level wrapper module.                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

module cgra_top_wrapper
  import cgra_pkg::*;
  import obi_pkg::*;
  import reg_pkg::*;
(
  input  logic               clk_i,
  input  logic               rst_ni,
  input  logic               rst_logic_ni,
  // Top level clock gating unit enable
  input  logic               cgra_enable_i,
  // AHB Masters
  output obi_req_t  [MP-1:0] masters_req_o,
  input  obi_resp_t [MP-1:0] masters_resp_i,
  // APB interface
  input  reg_req_t           reg_req_i,
  output reg_rsp_t           reg_rsp_o,
  // AHB Slave (Context memory)
  input  obi_req_t           slave_req_i,
  output obi_resp_t          slave_resp_o,
  // Set configuration memory in retentive mode
  input  logic               cmem_set_retentive_i,
  // interrupt
  output logic               cgra_int_o
);

  logic [                 MP-1:0] tcdm_req;
  logic [ DATA_BUS_ADD_WIDTH-1:0] tcdm_add        [   0:MP-1];
  logic [                 MP-1:0] tcdm_wen;
  logic [                  4-1:0] tcdm_be         [   0:MP-1];
  logic [DATA_BUS_DATA_WIDTH-1:0] tcdm_wdata      [   0:MP-1];
  logic [                 MP-1:0] tcdm_gnt;
  logic [DATA_BUS_DATA_WIDTH-1:0] tcdm_rdata      [   0:MP-1];
  logic [                 MP-1:0] tcdm_r_valid;

  logic                           cm_req;
  logic [ DATA_BUS_ADD_WIDTH-1:0] cm_add;
  logic                           cm_we;
  logic [                  4-1:0] cm_be;
  logic [DATA_BUS_DATA_WIDTH-1:0] cm_wdata;
  logic                           cm_gnt;
  logic [DATA_BUS_DATA_WIDTH-1:0] cm_rdata;
  logic                           cm_rvalid;

  logic [            N_SLOTS-1:0] cgra_evt;

  // Context memory decoder to actual SRAM macro
  logic                           cm_cg_s;
  logic [              N_ROW-1:0] cm_row_req_s;
  logic                           cm_we_s;
  logic [  IMEM_N_LINES_LOG2-1:0] cm_addr_s;
  logic [        INSTR_WIDTH-1:0] rcs_cmem_rdata_s[0:N_ROW-1];

  // MP 0
  assign masters_req_o[0].req   = tcdm_req[0];
  assign masters_req_o[0].addr  = tcdm_add[0];
  assign masters_req_o[0].we    = ~tcdm_wen[0];
  assign masters_req_o[0].be    = tcdm_be[0];
  assign masters_req_o[0].wdata = tcdm_wdata[0];
  assign tcdm_gnt[0]            = masters_resp_i[0].gnt;
  assign tcdm_rdata[0]          = masters_resp_i[0].rdata;
  assign tcdm_r_valid[0]        = masters_resp_i[0].rvalid;

  // MP 1
  assign masters_req_o[1].req   = tcdm_req[1];
  assign masters_req_o[1].addr  = tcdm_add[1];
  assign masters_req_o[1].we    = ~tcdm_wen[1];
  assign masters_req_o[1].be    = tcdm_be[1];
  assign masters_req_o[1].wdata = tcdm_wdata[1];
  assign tcdm_gnt[1]            = masters_resp_i[1].gnt;
  assign tcdm_rdata[1]          = masters_resp_i[1].rdata;
  assign tcdm_r_valid[1]        = masters_resp_i[1].rvalid;

  // MP 2
  assign masters_req_o[2].req   = tcdm_req[2];
  assign masters_req_o[2].addr  = tcdm_add[2];
  assign masters_req_o[2].we    = ~tcdm_wen[2];
  assign masters_req_o[2].be    = tcdm_be[2];
  assign masters_req_o[2].wdata = tcdm_wdata[2];
  assign tcdm_gnt[2]            = masters_resp_i[2].gnt;
  assign tcdm_rdata[2]          = masters_resp_i[2].rdata;
  assign tcdm_r_valid[2]        = masters_resp_i[2].rvalid;

  // MP 3
  assign masters_req_o[3].req   = tcdm_req[3];
  assign masters_req_o[3].addr  = tcdm_add[3];
  assign masters_req_o[3].we    = ~tcdm_wen[3];
  assign masters_req_o[3].be    = tcdm_be[3];
  assign masters_req_o[3].wdata = tcdm_wdata[3];
  assign tcdm_gnt[3]            = masters_resp_i[3].gnt;
  assign tcdm_rdata[3]          = masters_resp_i[3].rdata;
  assign tcdm_r_valid[3]        = masters_resp_i[3].rvalid;

  // Context memory slave
  assign slave_resp_o.gnt       = cm_gnt;
  assign slave_resp_o.rdata     = cm_rdata;
  assign slave_resp_o.rvalid    = cm_rvalid;
  assign cm_req                 = slave_req_i.req;
  assign cm_add                 = slave_req_i.addr;
  assign cm_we                  = slave_req_i.we;
  assign cm_be                  = slave_req_i.be;
  assign cm_wdata               = slave_req_i.wdata;

  // all request merged for now
  assign cgra_int_o             = |cgra_evt;

  // No actual need to read this memory so only cgra_ram0 is connected for read
  /* verilator lint_off IMPLICIT */
  assign cm_rdata_o             = rcs_cmem_rdata_s[0];
  /* verilator lint_on IMPLICIT */

  logic clk_mem_cg, clk_mem_en;
  logic clk_logic_cg;

  cgra_clock_gate clk_gate_mem_i (
    .clk_i    (clk_i),
    .test_en_i(1'b0),
    .en_i     (clk_mem_en),
    .clk_o    (clk_mem_cg)
  );

  cgra_clock_gate clk_gate_logic_i (
    .clk_i    (clk_i),
    .test_en_i(1'b0),
    .en_i     (cgra_enable_i),
    .clk_o    (clk_logic_cg)
  );

  cgra_top cgra_top_i (
    .clk_i           (clk_logic_cg),
    .rst_ni          (rst_logic_ni),
    // APB interface
    .reg_req_i       (reg_req_i),
    .reg_rsp_o       (reg_rsp_o),
    // AHB Master port
    .tcdm_req_o      (tcdm_req),
    .tcdm_add_o      (tcdm_add),
    .tcdm_wen_o      (tcdm_wen),
    .tcdm_be_o       (tcdm_be),
    .tcdm_wdata_o    (tcdm_wdata),
    .tcdm_gnt_i      (tcdm_gnt),
    .tcdm_rdata_i    (tcdm_rdata),
    .tcdm_r_valid_i  (tcdm_r_valid),
    // AHB Slave port
    .cm_req_i        (cm_req),
    .cm_add_i        (cm_add),
    .cm_we_i         (cm_we),
    .cm_be_i         (cm_be),
    .cm_wdata_i      (cm_wdata),
    .cm_gnt_o        (cm_gnt),
    .cm_rvalid_o     (cm_rvalid),
    // Context memory decoder to actual SRAM macro
    .clk_mem_cg_i    (clk_mem_cg),
    .clk_mem_en_o    (clk_mem_en),
    .cm_row_req_o    (cm_row_req_s),
    .cm_we_o         (cm_we_s),
    .cm_addr_o       (cm_addr_s),
    .rcs_cmem_rdata_i(rcs_cmem_rdata_s),
    // CGRA interrupts
    .evt_o           (cgra_evt)
  );

  context_memory cgra_context_memory_i (
    .clk_i             (clk_i),
    .rst_ni            (rst_ni),
    .cm_cg_i           (clk_mem_cg),
    .cm_set_retentive_i(cmem_set_retentive_i),
    .cm_row_req_i      (cm_row_req_s),
    .cm_we_i           (cm_we_s),
    .cm_addr_i         (cm_addr_s),
    .cm_wdata_i        (cm_wdata),
    .rcs_cmem_rdata_o  (rcs_cmem_rdata_s)
  );

endmodule
