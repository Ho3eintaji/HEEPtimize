// This example is launching a while loop that never ends on the CGRA to measure it's power consumption
// It executes dummy instruction with a various PE utilization ratios.

#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "heepatia.h"
// #include "heepocrates_ctrl.h"
#include "cgra.h"
#include "cgra_bitstream.h"

#define CGRA_PE_UTILIZATION 100

// Use PRINTF instead of PRINTF to remove print by default
#ifdef DEBUG
  #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
  #define PRINTF(...)
#endif

#define INPUT_LENGTH 4
#define OUTPUT_LENGTH 5

// one dim slot x n input values (data ptrs, constants, ...)
int32_t cgra_input[CGRA_N_COLS][CGRA_N_SLOTS][10] __attribute__ ((aligned (4)));
int8_t cgra_intr_flag;
int32_t cgra_res[CGRA_N_COLS][CGRA_N_ROWS][OUTPUT_LENGTH] = {0};

int32_t stimuli[CGRA_N_ROWS][INPUT_LENGTH] = {
  144, 4, 5, -23463,
  -12, 16, 5, 0, 
  1033, 8, 5, 0, 
  -199, 128, 5, 1, 
};

int32_t exp_rc_c0[CGRA_N_ROWS][OUTPUT_LENGTH] = {0};

plic_result_t plic_res;

static void handler_irq_cgra( uint32_t int_id )
{
  if (int_id == CGRA_INTR) { 
    cgra_intr_flag = 1;
  }
  // plic_irq_complete(&int_id);  //todo: maybe it is already completed
}

int main(void) {

  PRINTF("Init CGRA context memory...\n");
  cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
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

  // Select request slot of CGRA (2 slots)
  uint32_t cgra_slot = cgra_get_slot(&cgra);
  // input data ptr coumn 0
  cgra_input[0][cgra_slot][0] = (int32_t)&stimuli[0][0];
  cgra_input[0][cgra_slot][1] = (int32_t)&stimuli[1][0];
  cgra_input[0][cgra_slot][2] = (int32_t)&stimuli[2][0];
  cgra_input[0][cgra_slot][3] = (int32_t)&stimuli[3][0];

  // input data ptr coumn 1
  cgra_input[1][cgra_slot][0] = (int32_t)&stimuli[0][0];
  cgra_input[1][cgra_slot][1] = (int32_t)&stimuli[1][0];
  cgra_input[1][cgra_slot][2] = (int32_t)&stimuli[2][0];
  cgra_input[1][cgra_slot][3] = (int32_t)&stimuli[3][0];

  // input data ptr coumn 2
  cgra_input[2][cgra_slot][0] = (int32_t)&stimuli[0][0];
  cgra_input[2][cgra_slot][1] = (int32_t)&stimuli[1][0];
  cgra_input[2][cgra_slot][2] = (int32_t)&stimuli[2][0];
  cgra_input[2][cgra_slot][3] = (int32_t)&stimuli[3][0];

  // input data ptr coumn 3
  cgra_input[3][cgra_slot][0] = (int32_t)&stimuli[0][0];
  cgra_input[3][cgra_slot][1] = (int32_t)&stimuli[1][0];
  cgra_input[3][cgra_slot][2] = (int32_t)&stimuli[2][0];
  cgra_input[3][cgra_slot][3] = (int32_t)&stimuli[3][0];

  int8_t column_idx;
  // Set CGRA kernel pointers
  column_idx = 0;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[0][cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res[0][cgra_slot], column_idx);
  // Set CGRA kernel pointers column 1
  column_idx = 1;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[1][cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res[1][cgra_slot], column_idx);
  // Set CGRA kernel pointers column 2
  column_idx = 2;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[2][cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res[2][cgra_slot], column_idx);
  // Set CGRA kernel pointers column 3
  column_idx = 3;
  cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[3][cgra_slot], column_idx);
  cgra_set_write_ptr(&cgra, cgra_slot, (uint32_t) cgra_res[3][cgra_slot], column_idx);
  
  // Launch CGRA kernel which is never returning

  uint8_t cgra_pe_utilization;
  #if CGRA_PE_UTILIZATION==100
    cgra_set_kernel(&cgra, cgra_slot, CGRA_WHILE1_100PERCENT);
    cgra_pe_utilization = 100;
  #elif CGRA_PE_UTILIZATION==75
    cgra_set_kernel(&cgra, cgra_slot, CGRA_WHILE1_75PERCENT);
    cgra_pe_utilization = 80;
  #else
    cgra_set_kernel(&cgra, cgra_slot, CGRA_WHILE1_50PERCENT);
    cgra_pe_utilization = 50;
  #endif

  printf("Run while loop for ever on CGRA with %d\% PE utilization...\n", cgra_pe_utilization);

  // Backup code that should never be executed
  // Wait CGRA is done
  cgra_intr_flag=0;
  while(cgra_intr_flag==0) {
    wait_for_interrupt();
  }

  printf("ERROR: CGRA while1 loop exited for unknown reason\n");

  // This point should never be reached
  return EXIT_FAILURE;
}
