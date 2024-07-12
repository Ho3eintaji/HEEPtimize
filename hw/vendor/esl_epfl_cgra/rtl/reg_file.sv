////////////////////////////////////////////////////////////////////////////////
// Author:         Benoît Denkinger - benoit.denkinger@epfl.ch                //
//                                                                            //
// Additional contributions by:                                               //
//                                                                            //
//                                                                            //
// Design Name:    RCs REGISTER FILE                                          //
// Project Name:   CGRA                                                       //
// Language:       SystemVerilog                                              //
//                                                                            //
// Description:    Register file used for computation.                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

module reg_file
  import cgra_pkg::*;
#(
  parameter REGFILE_DEPTH = 4,
  parameter REGFILE_NSEL  = $clog2(REGFILE_DEPTH),
  parameter REGFILE_WIDTH = 32
) (
  input  logic                     clk_i,
  input  logic                     rst_i,
  input  logic                     ce_i,
  input  logic                     we_i,
  input  logic [ REGFILE_NSEL-1:0] wsel_i,
  input  logic [REGFILE_WIDTH-1:0] reg_i,
  output logic [REGFILE_WIDTH-1:0] regs_o[0:REGFILE_DEPTH-1]
);

  logic [REGFILE_WIDTH-1:0] reg_file_mem[0:REGFILE_DEPTH-1];

  assign regs_o = reg_file_mem;

  always_ff @(posedge clk_i) begin
    if (rst_i == 1'b1) begin
      reg_file_mem <= '{default: '0};
    end else if (we_i & ce_i) begin
      reg_file_mem[wsel_i] <= reg_i;
    end
  end

endmodule
