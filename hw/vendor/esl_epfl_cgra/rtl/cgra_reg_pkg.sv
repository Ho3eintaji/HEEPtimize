// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Register Package auto-generated by `reggen` containing data structure

package cgra_reg_pkg;

  // Address widths within the block
  parameter int BlockAw = 7;

  ////////////////////////////
  // Typedefs for registers //
  ////////////////////////////

  typedef struct packed {logic [3:0] q;} cgra_reg2hw_col_status_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ker_id_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ker_id_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_in_c0_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_out_c0_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_in_c1_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_out_c1_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_in_c2_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_out_c2_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_in_c3_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot0_ptr_out_c3_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_in_c0_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_out_c0_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_in_c1_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_out_c1_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_in_c2_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_out_c2_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_in_c3_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_slot1_ptr_out_c3_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_enable_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_reset_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_total_kernels_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c0_active_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c0_stall_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c1_active_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c1_stall_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c2_active_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c2_stall_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c3_active_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_perf_cnt_c3_stall_cycles_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_reserved30_reg_t;

  typedef struct packed {logic [31:0] q;} cgra_reg2hw_reserved31_reg_t;

  typedef struct packed {
    logic [3:0] d;
    logic       de;
  } cgra_hw2reg_col_status_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_slot0_ker_id_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_slot1_ker_id_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_reset_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_total_kernels_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c0_active_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c0_stall_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c1_active_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c1_stall_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c2_active_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c2_stall_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c3_active_cycles_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } cgra_hw2reg_perf_cnt_c3_stall_cycles_reg_t;

  // Register -> HW type
  typedef struct packed {
    cgra_reg2hw_col_status_reg_t col_status;  // [995:992]
    cgra_reg2hw_slot0_ker_id_reg_t slot0_ker_id;  // [991:960]
    cgra_reg2hw_slot1_ker_id_reg_t slot1_ker_id;  // [959:928]
    cgra_reg2hw_slot0_ptr_in_c0_reg_t slot0_ptr_in_c0;  // [927:896]
    cgra_reg2hw_slot0_ptr_out_c0_reg_t slot0_ptr_out_c0;  // [895:864]
    cgra_reg2hw_slot0_ptr_in_c1_reg_t slot0_ptr_in_c1;  // [863:832]
    cgra_reg2hw_slot0_ptr_out_c1_reg_t slot0_ptr_out_c1;  // [831:800]
    cgra_reg2hw_slot0_ptr_in_c2_reg_t slot0_ptr_in_c2;  // [799:768]
    cgra_reg2hw_slot0_ptr_out_c2_reg_t slot0_ptr_out_c2;  // [767:736]
    cgra_reg2hw_slot0_ptr_in_c3_reg_t slot0_ptr_in_c3;  // [735:704]
    cgra_reg2hw_slot0_ptr_out_c3_reg_t slot0_ptr_out_c3;  // [703:672]
    cgra_reg2hw_slot1_ptr_in_c0_reg_t slot1_ptr_in_c0;  // [671:640]
    cgra_reg2hw_slot1_ptr_out_c0_reg_t slot1_ptr_out_c0;  // [639:608]
    cgra_reg2hw_slot1_ptr_in_c1_reg_t slot1_ptr_in_c1;  // [607:576]
    cgra_reg2hw_slot1_ptr_out_c1_reg_t slot1_ptr_out_c1;  // [575:544]
    cgra_reg2hw_slot1_ptr_in_c2_reg_t slot1_ptr_in_c2;  // [543:512]
    cgra_reg2hw_slot1_ptr_out_c2_reg_t slot1_ptr_out_c2;  // [511:480]
    cgra_reg2hw_slot1_ptr_in_c3_reg_t slot1_ptr_in_c3;  // [479:448]
    cgra_reg2hw_slot1_ptr_out_c3_reg_t slot1_ptr_out_c3;  // [447:416]
    cgra_reg2hw_perf_cnt_enable_reg_t perf_cnt_enable;  // [415:384]
    cgra_reg2hw_perf_cnt_reset_reg_t perf_cnt_reset;  // [383:352]
    cgra_reg2hw_perf_cnt_total_kernels_reg_t perf_cnt_total_kernels;  // [351:320]
    cgra_reg2hw_perf_cnt_c0_active_cycles_reg_t perf_cnt_c0_active_cycles;  // [319:288]
    cgra_reg2hw_perf_cnt_c0_stall_cycles_reg_t perf_cnt_c0_stall_cycles;  // [287:256]
    cgra_reg2hw_perf_cnt_c1_active_cycles_reg_t perf_cnt_c1_active_cycles;  // [255:224]
    cgra_reg2hw_perf_cnt_c1_stall_cycles_reg_t perf_cnt_c1_stall_cycles;  // [223:192]
    cgra_reg2hw_perf_cnt_c2_active_cycles_reg_t perf_cnt_c2_active_cycles;  // [191:160]
    cgra_reg2hw_perf_cnt_c2_stall_cycles_reg_t perf_cnt_c2_stall_cycles;  // [159:128]
    cgra_reg2hw_perf_cnt_c3_active_cycles_reg_t perf_cnt_c3_active_cycles;  // [127:96]
    cgra_reg2hw_perf_cnt_c3_stall_cycles_reg_t perf_cnt_c3_stall_cycles;  // [95:64]
    cgra_reg2hw_reserved30_reg_t reserved30;  // [63:32]
    cgra_reg2hw_reserved31_reg_t reserved31;  // [31:0]
  } cgra_reg2hw_t;

  // HW -> register type
  typedef struct packed {
    cgra_hw2reg_col_status_reg_t col_status;  // [400:396]
    cgra_hw2reg_slot0_ker_id_reg_t slot0_ker_id;  // [395:363]
    cgra_hw2reg_slot1_ker_id_reg_t slot1_ker_id;  // [362:330]
    cgra_hw2reg_perf_cnt_reset_reg_t perf_cnt_reset;  // [329:297]
    cgra_hw2reg_perf_cnt_total_kernels_reg_t perf_cnt_total_kernels;  // [296:264]
    cgra_hw2reg_perf_cnt_c0_active_cycles_reg_t perf_cnt_c0_active_cycles;  // [263:231]
    cgra_hw2reg_perf_cnt_c0_stall_cycles_reg_t perf_cnt_c0_stall_cycles;  // [230:198]
    cgra_hw2reg_perf_cnt_c1_active_cycles_reg_t perf_cnt_c1_active_cycles;  // [197:165]
    cgra_hw2reg_perf_cnt_c1_stall_cycles_reg_t perf_cnt_c1_stall_cycles;  // [164:132]
    cgra_hw2reg_perf_cnt_c2_active_cycles_reg_t perf_cnt_c2_active_cycles;  // [131:99]
    cgra_hw2reg_perf_cnt_c2_stall_cycles_reg_t perf_cnt_c2_stall_cycles;  // [98:66]
    cgra_hw2reg_perf_cnt_c3_active_cycles_reg_t perf_cnt_c3_active_cycles;  // [65:33]
    cgra_hw2reg_perf_cnt_c3_stall_cycles_reg_t perf_cnt_c3_stall_cycles;  // [32:0]
  } cgra_hw2reg_t;

  // Register offsets
  parameter logic [BlockAw-1:0] CGRA_COL_STATUS_OFFSET = 7'h0;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_KER_ID_OFFSET = 7'h4;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_KER_ID_OFFSET = 7'h8;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_IN_C0_OFFSET = 7'hc;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_OUT_C0_OFFSET = 7'h10;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_IN_C1_OFFSET = 7'h14;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_OUT_C1_OFFSET = 7'h18;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_IN_C2_OFFSET = 7'h1c;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_OUT_C2_OFFSET = 7'h20;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_IN_C3_OFFSET = 7'h24;
  parameter logic [BlockAw-1:0] CGRA_SLOT0_PTR_OUT_C3_OFFSET = 7'h28;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_IN_C0_OFFSET = 7'h2c;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_OUT_C0_OFFSET = 7'h30;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_IN_C1_OFFSET = 7'h34;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_OUT_C1_OFFSET = 7'h38;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_IN_C2_OFFSET = 7'h3c;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_OUT_C2_OFFSET = 7'h40;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_IN_C3_OFFSET = 7'h44;
  parameter logic [BlockAw-1:0] CGRA_SLOT1_PTR_OUT_C3_OFFSET = 7'h48;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_ENABLE_OFFSET = 7'h4c;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_RESET_OFFSET = 7'h50;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_TOTAL_KERNELS_OFFSET = 7'h54;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C0_ACTIVE_CYCLES_OFFSET = 7'h58;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C0_STALL_CYCLES_OFFSET = 7'h5c;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C1_ACTIVE_CYCLES_OFFSET = 7'h60;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C1_STALL_CYCLES_OFFSET = 7'h64;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C2_ACTIVE_CYCLES_OFFSET = 7'h68;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C2_STALL_CYCLES_OFFSET = 7'h6c;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C3_ACTIVE_CYCLES_OFFSET = 7'h70;
  parameter logic [BlockAw-1:0] CGRA_PERF_CNT_C3_STALL_CYCLES_OFFSET = 7'h74;
  parameter logic [BlockAw-1:0] CGRA_RESERVED30_OFFSET = 7'h78;
  parameter logic [BlockAw-1:0] CGRA_RESERVED31_OFFSET = 7'h7c;

  // Register index
  typedef enum int {
    CGRA_COL_STATUS,
    CGRA_SLOT0_KER_ID,
    CGRA_SLOT1_KER_ID,
    CGRA_SLOT0_PTR_IN_C0,
    CGRA_SLOT0_PTR_OUT_C0,
    CGRA_SLOT0_PTR_IN_C1,
    CGRA_SLOT0_PTR_OUT_C1,
    CGRA_SLOT0_PTR_IN_C2,
    CGRA_SLOT0_PTR_OUT_C2,
    CGRA_SLOT0_PTR_IN_C3,
    CGRA_SLOT0_PTR_OUT_C3,
    CGRA_SLOT1_PTR_IN_C0,
    CGRA_SLOT1_PTR_OUT_C0,
    CGRA_SLOT1_PTR_IN_C1,
    CGRA_SLOT1_PTR_OUT_C1,
    CGRA_SLOT1_PTR_IN_C2,
    CGRA_SLOT1_PTR_OUT_C2,
    CGRA_SLOT1_PTR_IN_C3,
    CGRA_SLOT1_PTR_OUT_C3,
    CGRA_PERF_CNT_ENABLE,
    CGRA_PERF_CNT_RESET,
    CGRA_PERF_CNT_TOTAL_KERNELS,
    CGRA_PERF_CNT_C0_ACTIVE_CYCLES,
    CGRA_PERF_CNT_C0_STALL_CYCLES,
    CGRA_PERF_CNT_C1_ACTIVE_CYCLES,
    CGRA_PERF_CNT_C1_STALL_CYCLES,
    CGRA_PERF_CNT_C2_ACTIVE_CYCLES,
    CGRA_PERF_CNT_C2_STALL_CYCLES,
    CGRA_PERF_CNT_C3_ACTIVE_CYCLES,
    CGRA_PERF_CNT_C3_STALL_CYCLES,
    CGRA_RESERVED30,
    CGRA_RESERVED31
  } cgra_id_e;

  // Register width information to check illegal writes
  parameter logic [3:0] CGRA_PERMIT[32] = '{
      4'b0001,  // index[ 0] CGRA_COL_STATUS
      4'b1111,  // index[ 1] CGRA_SLOT0_KER_ID
      4'b1111,  // index[ 2] CGRA_SLOT1_KER_ID
      4'b1111,  // index[ 3] CGRA_SLOT0_PTR_IN_C0
      4'b1111,  // index[ 4] CGRA_SLOT0_PTR_OUT_C0
      4'b1111,  // index[ 5] CGRA_SLOT0_PTR_IN_C1
      4'b1111,  // index[ 6] CGRA_SLOT0_PTR_OUT_C1
      4'b1111,  // index[ 7] CGRA_SLOT0_PTR_IN_C2
      4'b1111,  // index[ 8] CGRA_SLOT0_PTR_OUT_C2
      4'b1111,  // index[ 9] CGRA_SLOT0_PTR_IN_C3
      4'b1111,  // index[10] CGRA_SLOT0_PTR_OUT_C3
      4'b1111,  // index[11] CGRA_SLOT1_PTR_IN_C0
      4'b1111,  // index[12] CGRA_SLOT1_PTR_OUT_C0
      4'b1111,  // index[13] CGRA_SLOT1_PTR_IN_C1
      4'b1111,  // index[14] CGRA_SLOT1_PTR_OUT_C1
      4'b1111,  // index[15] CGRA_SLOT1_PTR_IN_C2
      4'b1111,  // index[16] CGRA_SLOT1_PTR_OUT_C2
      4'b1111,  // index[17] CGRA_SLOT1_PTR_IN_C3
      4'b1111,  // index[18] CGRA_SLOT1_PTR_OUT_C3
      4'b1111,  // index[19] CGRA_PERF_CNT_ENABLE
      4'b1111,  // index[20] CGRA_PERF_CNT_RESET
      4'b1111,  // index[21] CGRA_PERF_CNT_TOTAL_KERNELS
      4'b1111,  // index[22] CGRA_PERF_CNT_C0_ACTIVE_CYCLES
      4'b1111,  // index[23] CGRA_PERF_CNT_C0_STALL_CYCLES
      4'b1111,  // index[24] CGRA_PERF_CNT_C1_ACTIVE_CYCLES
      4'b1111,  // index[25] CGRA_PERF_CNT_C1_STALL_CYCLES
      4'b1111,  // index[26] CGRA_PERF_CNT_C2_ACTIVE_CYCLES
      4'b1111,  // index[27] CGRA_PERF_CNT_C2_STALL_CYCLES
      4'b1111,  // index[28] CGRA_PERF_CNT_C3_ACTIVE_CYCLES
      4'b1111,  // index[29] CGRA_PERF_CNT_C3_STALL_CYCLES
      4'b1111,  // index[30] CGRA_RESERVED30
      4'b1111  // index[31] CGRA_RESERVED31
  };

endpackage

