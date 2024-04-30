// Copyright 2023 Politecnico di Torino.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 2.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-2.0. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// File: nm_carus_wrapper.sv
// Author: Michele Caon
// Date: 30/05/2023

module nm_carus_wrapper #(
  parameter int unsigned NUM_BANKS = 4,
  parameter int unsigned BANK_ADDR_WIDTH = 13  // 8kiB
) (
  input logic clk_i,
  input logic rst_ni,

  // Memory retentive mode
  input logic set_retentive_ni,  // active low

  // Operation control
  input  logic imc_i,
  output logic done_o,

  // Bus interface
  input  obi_pkg::obi_req_t  bus_req_i,
  output obi_pkg::obi_resp_t bus_rsp_o
);
  // PARAMETERS
  localparam int unsigned CarusAddrWidth = BANK_ADDR_WIDTH + unsigned'($clog2(NUM_BANKS));


  // -------
  // NM-CARUS
  // -------
  carus_top #(
    .NUM_BANKS      (NUM_BANKS),
    .BANK_ADDR_WIDTH(BANK_ADDR_WIDTH)
  ) u_carus_top (
    .clk_i           (clk_i),
    .rst_ni          (rst_ni),
    .set_retentive_ni(set_retentive_ni),
    .imc_i           (imc_i),
    .done_o          (done_o),
    .bus_req_i       (bus_req_i.req),
    .bus_we_i        (bus_req_i.we),
    .bus_be_i        (bus_req_i.be),
    .bus_addr_i      (bus_req_i.addr[CarusAddrWidth-1:0]),
    .bus_wdata_i     (bus_req_i.wdata),
    .bus_gnt_o       (bus_rsp_o.gnt),
    .bus_rvalid_o    (bus_rsp_o.rvalid),
    .bus_rdata_o     (bus_rsp_o.rdata)
  );
endmodule
