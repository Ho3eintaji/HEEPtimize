////////////////////////////////////////////////////////////////////////////////
// Author:         Benoît Denkinger - benoit.denkinger@epfl.ch                //
//                                                                            //
// Additional contributions by:                                               //
//                                                                            //
//                                                                            //
// Design Name:    CGRA CONFIG PACKAGE                                        //
// Project Name:   CGRA                                                       //
// Language:       SystemVerilog                                              //
//                                                                            //
// Description:    CGRA configuration file (#regiters, #columns, #rows, ...). //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

package cgra_pkg;

  // This configures the number of peripheral registers
  // in order to have independent set of registers for each slot
  localparam N_SLOTS = 2;
  localparam N_SLOTS_LOG2 = $clog2(N_SLOTS);

  // CGRA GRID CONFIGURATION
  localparam N_ROW = 4;
  localparam N_ROW_LOG2 = $clog2(N_ROW);
  localparam N_COL = 4;
  localparam N_COL_LOG2 = $clog2(N_COL);

  // Data bus master ports
  localparam MP = N_COL;
  localparam ID = 9;

  // CGRA APB REGISTERS
  localparam N_PERIPH_REGS = 32;
  localparam N_PERIPH_REGS_LOG2 = $clog2(N_PERIPH_REGS);

  localparam CGRA_APB_REG_STATE = 0;

  // RCs CONFIGURATION
  localparam RC_NUM_REG = 4;
  localparam RC_NUM_REG_LOG = $clog2(RC_NUM_REG);

  localparam RCS_NUM_CREG = 32;
  localparam RCS_NUM_CREG_LOG2 = $clog2(RCS_NUM_CREG);

  localparam DP_WIDTH = 32;
  localparam DP_WIDTH_LOG2 = $clog2(DP_WIDTH);
  localparam INSTR_WIDTH = 32;

  // CGRA INSTRUCTION MEMORY
  localparam N_MEM_BANKS = 5;  // 4 INSTRS + 1 KERNEL CONF
  localparam N_MEM_BANKS_LOG2 = $clog2(N_MEM_BANKS);  // 4 INSTRS + 1 KERNEL CONF

  localparam IMEM_N_LINES = 128;  // per RC
  localparam IMEM_N_LINES_LOG2 = $clog2(IMEM_N_LINES);

  // for compatibilty reason
  localparam RC_INSTR_N_REG = IMEM_N_LINES;
  localparam RC_INSTR_N_REG_LOG2 = IMEM_N_LINES_LOG2;

  localparam KER_CONF_N_REG = 16;
  localparam KER_CONF_N_REG_LOG2 = $clog2(KER_CONF_N_REG);

  // Number of bits needed to write to the multi-banks instruction memory
  localparam WR_INSTR_ADD_LEN = N_MEM_BANKS_LOG2 + RC_INSTR_N_REG_LOG2;

  // PLATFORM SPECIFIC CONFIGURATION
  localparam CGRA_BITSTREAM_FILENAME = "$IPS_ROOT/cgra/mem/cgra_imem.bit";
  localparam CGRA_KER_MEM_FILENAME = "$IPS_ROOT/cgra/mem/cgra_kmem.bit";

  // AHB MEMORY INTERFACE
  localparam DATA_BUS_DATA_WIDTH = 32;
  localparam DATA_BUS_ADD_WIDTH = 32;

  // Peripheral bus configuration
  localparam PERIPH_ADD_WIDTH = 32;
  localparam PERIPH_ID_WIDTH = 9;
  localparam PERIPH_DATA_WIDTH = 32;
  localparam PERIPH_BE_WIDTH = 4;

  // DATAPATH OPTIONS

  localparam RC_MUX_A_NUM_INPUTS = 16;
  localparam RC_MUX_A_NSEL = $clog2(RC_MUX_A_NUM_INPUTS);
  localparam RC_MUX_A_INPUTS_WIDTH = DP_WIDTH;

  localparam RC_MUX_B_NUM_INPUTS = 16;
  localparam RC_MUX_B_NSEL = $clog2(RC_MUX_B_NUM_INPUTS);
  localparam RC_MUX_B_INPUTS_WIDTH = DP_WIDTH;

  localparam RC_MUX_FLAG_NUM_INPUTS = 8;
  localparam RC_MUX_FLAG_NSEL = $clog2(RC_MUX_FLAG_NUM_INPUTS);
  localparam RC_MUX_FLAG_INPUTS_WIDTH = 2;  // ALU_N_FLAG

  // ALU CONFIGURATION
  localparam CGRA_ALU_OP_WIDTH = 5;
  // sign flag, zero flag
  localparam ALU_N_FLAG = RC_MUX_FLAG_INPUTS_WIDTH;

  localparam RC_CONST_WIDTH = 13;

  localparam CGRA_ALU_NOP = 5'b00000;
  localparam CGRA_ALU_SADD = 5'b00001;
  localparam CGRA_ALU_SSUB = 5'b00010;
  localparam CGRA_ALU_SMUL = 5'b00011;
  localparam CGRA_ALU_FXPMUL = 5'b00100;
  localparam CGRA_ALU_SLL = 5'b00101;
  localparam CGRA_ALU_SRL = 5'b00110;
  localparam CGRA_ALU_SRA = 5'b00111;
  localparam CGRA_ALU_LAND = 5'b01000;
  localparam CGRA_ALU_LOR = 5'b01001;
  localparam CGRA_ALU_LXOR = 5'b01010;
  localparam CGRA_ALU_LNAND = 5'b01011;
  localparam CGRA_ALU_LNOR = 5'b01100;
  localparam CGRA_ALU_LNXOR = 5'b01101;
  localparam CGRA_ALU_BSFA = 5'b01110;
  localparam CGRA_ALU_BZFA = 5'b01111;
  localparam CGRA_ALU_BEQ = 5'b10000;
  localparam CGRA_ALU_BNE = 5'b10001;
  localparam CGRA_ALU_BLT = 5'b10010;
  localparam CGRA_ALU_BGE = 5'b10011;
  localparam CGRA_ALU_JUMP = 5'b10100;
  localparam CGRA_ALU_LWD = 5'b10101;
  localparam CGRA_ALU_SWD = 5'b10110;
  localparam CGRA_ALU_LWI = 5'b10111;
  localparam CGRA_ALU_SWI = 5'b11000;
  localparam CGRA_ALU_EXIT = 5'b11001;

  // Number of columns needed  : 1 bit per column      :  4
  // RCs kernel start address  : clog2(RC_INSTR_N_REG) :  7
  // Number of RCs instr.      : clog2(RCS_NUM_CREG)   :  5
  //---------------------------------------------------------
  //                                             TOTAL : 16

  localparam KMEM_WIDTH = N_COL + RC_INSTR_N_REG_LOG2 + RCS_NUM_CREG_LOG2;

  // Start/end bit of the different part of the word
  // RCS/MUX number of configuration words
  localparam RCS_N_INSTR_LB = 0;
  localparam RCS_N_INSTR_HB = RCS_N_INSTR_LB + RCS_NUM_CREG_LOG2 - 1;
  // RCS/MUX start address in instruction memory
  localparam RCS_IMEM_ADD_LB = RCS_N_INSTR_HB + 1;
  localparam RCS_IMEM_ADD_HB = RCS_IMEM_ADD_LB + IMEM_N_LINES_LOG2 - 1;
  // Number of column needed (one-hot encoding)
  localparam KER_N_COL_LB = RCS_IMEM_ADD_HB + 1;
  localparam KER_N_COL_HB = KER_N_COL_LB + N_COL - 1;

  // Maximum number of columns a kernel can use
  localparam MAX_COL_REQ = 4;
  localparam MAX_COL_REQ_LOG2 = $clog2(MAX_COL_REQ);

endpackage
