/*
 * This application is a test for the CGRA kernels. It runs two kernels in parallel:
  * - Double minimum search
  * - Double maximum search
*/

#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "heepatia.h"
#include "cgra.h"
#include "cgra_bitstream.h"
#include "stimuli.h"

// Use PRINTF instead of PRINTF to remove print by default
#ifdef DEBUG
  #define PRINTF(fmt, ...)    PRINTF(fmt, ## __VA_ARGS__)
#else
  #define PRINTF(...)
#endif

#define OUTPUT_LENGTH 4

// one dim slot x n input values (data ptrs, constants, ...)
int32_t cgra_input[CGRA_N_SLOTS][10] __attribute__ ((aligned (4)));
int8_t cgra_intr_flag;
volatile int32_t cgra_res[OUTPUT_LENGTH] = {0};
int32_t exp_res[OUTPUT_LENGTH] = {0};


static void handler_irq_cgra( uint32_t int_id )
{
  if (int_id == CGRA_INTR) { //todo: maybe the checking part is not required
    cgra_intr_flag = 1;
  }
  // plic_irq_complete(&int_id);  //todo: maybe it is already completed
}

int main(void) {

  PRINTF("Init CGRA context memory...\n");
  cgra_cmem_init(cgra_imem_bistream, cgra_kmem_bitstream);
  PRINTF("\rdone\n");

  // Init the PLIC
  if (plic_Init() != kPlicOk) {
    printf("PLIC init failed\n;");
    return EXIT_FAILURE;
  }
  // Set CGRA priority to 1 (target threshold is by default 0) to trigger an interrupt to the target (the processor)
  if (plic_irq_set_priority(CGRA_INTR, 1) != kPlicOk) {
    printf("Set CGRA interrupt priority to 1 failed\n;");
    return EXIT_FAILURE;
  }
  // Enable CGRA interrupt
  if (plic_irq_set_enabled(CGRA_INTR, kPlicToggleEnabled) != kPlicOk) {
    printf("Enable CGRA interrupt failed\n;");
    return EXIT_FAILURE;
  }
  // Assign CGRA interrupt handler
  if (plic_assign_external_irq_handler(CGRA_INTR, &handler_irq_cgra) != kPlicOk) {
    printf("Assign CGRA interrupt handler failed\n;");
    return EXIT_FAILURE;
  }

  // Enable interrupt on processor side
  // Enable global interrupt for machine-level interrupts
  CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
  // Set mie.MEIE bit to one to enable machine-level external interrupts
  const uint32_t mask = 1 << 11;//IRQ_EXT_ENABLE_OFFSET;
  CSR_SET_BITS(CSR_REG_MIE, mask);
  cgra_intr_flag = 0;

  cgra_t cgra;
  cgra.base_addr = mmio_region_from_addr((uintptr_t)OECGRA_CONFIG_REGS_START_ADDRESS);

  // Variable for CGRA call anc check
  uint8_t cgra_slot;
  int8_t column_idx;
  int32_t errors;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  _____   ____  _    _ ____  _      ______   __  __ _____ _   _    _____ ______          _____   _____ _    _  //
  // |  __ \ / __ \| |  | |  _ \| |    |  ____| |  \/  |_   _| \ | |  / ____|  ____|   /\   |  __ \ / ____| |  | | //
  // | |  | | |  | | |  | | |_) | |    | |__    | \  / | | | |  \| | | (___ | |__     /  \  | |__) | |    | |__| | //
  // | |  | | |  | | |  | |  _ <| |    |  __|   | |\/| | | | | . ` |  \___ \|  __|   / /\ \ |  _  /| |    |  __  | //
  // | |__| | |__| | |__| | |_) | |____| |____  | |  | |_| |_| |\  |  ____) | |____ / ____ \| | \ \| |____| |  | | //
  // |_____/ \____/ \____/|____/|______|______| |_|  |_|_____|_| \_| |_____/|______/_/    \_\_|  \_\\_____|_|  |_| //
  //                                                                                                               //
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  exp_res[0] = stimuli[0];
  exp_res[1] = INT32_MAX;
  exp_res[2] = 0;
  exp_res[3] = -1;

  PRINTF("Run double minimum search on cpu...\n");
  for(int32_t i=1; i<INPUT_LENGTH; i++) {
    if (stimuli[i] < exp_res[0]) {
      exp_res[1] = exp_res[0];
      exp_res[0] = stimuli[i] ;
      exp_res[3] = exp_res[2];
      exp_res[2] = i;
    } else if (stimuli[i] < exp_res[1]) {
      exp_res[1] = stimuli[i];
      exp_res[3] = i;
    }
  }
  PRINTF("\rdone\n");

  // Select request slot of CGRA (2 slots)
  cgra_slot = cgra_get_slot(&cgra);
  // input data ptr
  cgra_input[cgra_slot][0] = (int32_t)&stimuli[0];
  // input size
  cgra_input[cgra_slot][1] = INPUT_LENGTH-1;

  printf("Run double minimum search on CGRA...\n");
  cgra_perf_cnt_enable(&cgra, 1);
  column_idx;
  // Set CGRA kernel pointers
  column_idx = 0;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res, column_idx);
  // Launch CGRA kernel
  cgra_set_kernel(&cgra, cgra_slot, DBL_MIN_KER_ID);

  // Wait CGRA is done
  cgra_intr_flag=0;
  while(cgra_intr_flag==0) {
    wait_for_interrupt();
  }

  // Check the cgra values are correct
  errors=0;
  for (int i=0; i<OUTPUT_LENGTH; i++) {
    if (cgra_res[i] != exp_res[i]) {
      printf("[%d]: %d != %d\n", i, cgra_res[i], exp_res[i]);
      printf("[%d]: %08x != %08x\n", i, cgra_res[i], exp_res[i]);
      errors++;
    }
  }

  printf("CGRA double minimum check finished with %d errors\n", errors);

  // Performance counter display
  printf("CGRA kernel executed: %d\n", cgra_perf_cnt_get_kernel(&cgra));

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  _____   ____  _    _ ____  _      ______   __  __          __   __   _____ ______          _____   _____ _    _  //
  // |  __ \ / __ \| |  | |  _ \| |    |  ____| |  \/  |   /\    \ \ / /  / ____|  ____|   /\   |  __ \ / ____| |  | | //
  // | |  | | |  | | |  | | |_) | |    | |__    | \  / |  /  \    \ V /  | (___ | |__     /  \  | |__) | |    | |__| | //
  // | |  | | |  | | |  | |  _ <| |    |  __|   | |\/| | / /\ \    > <    \___ \|  __|   / /\ \ |  _  /| |    |  __  | //
  // | |__| | |__| | |__| | |_) | |____| |____  | |  | |/ ____ \  / . \   ____) | |____ / ____ \| | \ \| |____| |  | | //
  // |_____/ \____/ \____/|____/|______|______| |_|  |_/_/    \_\/_/ \_\ |_____/|______/_/    \_\_|  \_\\_____|_|  |_| //
  //                                                                                                                   //
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////                                                                                             

  exp_res[0] = stimuli[0];
  exp_res[1] = INT32_MIN;
  exp_res[2] = 0;
  exp_res[3] = -1;

  PRINTF("Run double maximum search on cpu...\n");
  for(int32_t i=1; i<INPUT_LENGTH; i++) {
    if (stimuli[i] > exp_res[0]) {
      exp_res[1] = exp_res[0];
      exp_res[0] = stimuli[i] ;
      exp_res[3] = exp_res[2];
      exp_res[2] = i;
    } else if (stimuli[i] > exp_res[1]) {
      exp_res[1] = stimuli[i];
      exp_res[3] = i;
    }
  }
  PRINTF("\rdone\n");

  // Select request slot of CGRA (2 slots)
  cgra_slot = cgra_get_slot(&cgra);
  // input data ptr
  cgra_input[cgra_slot][0] = (int32_t)&stimuli[0];
  // input size
  cgra_input[cgra_slot][1] = INPUT_LENGTH-1;

  printf("Run double maximum search on CGRA...\n");
  cgra_perf_cnt_enable(&cgra, 1);
  // Set CGRA kernel pointers
  column_idx = 0;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res, column_idx);
  // Launch CGRA kernel
  cgra_set_kernel(&cgra, cgra_slot, DBL_MAX_KER_ID);

  // Wait CGRA is done
  cgra_intr_flag=0;
  while(cgra_intr_flag==0) {
    wait_for_interrupt();
  }

  // Check the cgra values are correct
  errors=0;
  for (int i=0; i<OUTPUT_LENGTH; i++) {
    if (cgra_res[i] != exp_res[i]) {
      printf("[%d]: %d != %d\n", i, cgra_res[i], exp_res[i]);
      printf("[%d]: %08x != %08x\n", i, cgra_res[i], exp_res[i]);
      errors++;
    }
  }

  printf("CGRA double maximum check finished with %d errors\n", errors);

  // Performance counter display
  printf("CGRA kernel executed: %d\n", cgra_perf_cnt_get_kernel(&cgra));
  column_idx = 0;
  PRINTF("CGRA column %d active cycles: %d\n", column_idx, cgra_perf_cnt_get_col_active(&cgra, column_idx));
  PRINTF("CGRA column %d stall cycles : %d\n", column_idx, cgra_perf_cnt_get_col_stall(&cgra, column_idx));
  column_idx = 1;
  PRINTF("CGRA column %d active cycles: %d\n", column_idx, cgra_perf_cnt_get_col_active(&cgra, column_idx));
  PRINTF("CGRA column %d stall cycles : %d\n", column_idx, cgra_perf_cnt_get_col_stall(&cgra, column_idx));
  column_idx = 2;
  PRINTF("CGRA column %d active cycles: %d\n", column_idx, cgra_perf_cnt_get_col_active(&cgra, column_idx));
  PRINTF("CGRA column %d stall cycles : %d\n", column_idx, cgra_perf_cnt_get_col_stall(&cgra, column_idx));
  column_idx = 3;
  PRINTF("CGRA column %d active cycles: %d\n", column_idx, cgra_perf_cnt_get_col_active(&cgra, column_idx));
  PRINTF("CGRA column %d stall cycles : %d\n", column_idx, cgra_perf_cnt_get_col_stall(&cgra, column_idx));

  return EXIT_SUCCESS;
}
