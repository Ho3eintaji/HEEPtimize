#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "rv_plic_regs.h"
#include "heepatia.h"
#include "cgra.h"
#include "cgra_bitstream.h"
#include "fxp.h"
#include "defines.h"
#include "timer_sdk.h"

#if FFT_SIZE==512
  #include "fft_factors_512_32b_int.h"
#endif
#if FFT_SIZE==1024
  #include "fft_factors_1024_32b_int.h"
#endif
#if FFT_SIZE==2048
  #include "fft_factors_2048_32b_int.h"
#endif

#define DATA_SIZE 30*512

fxp input_signal[2*DATA_SIZE] = { 1 };
// FFT radix-2 variables
// fxp RealOut_fft0_fxp[FFT_SIZE] __attribute__ ((aligned (4))) __attribute__((section(".xheep_data_interleaved"))) = { 0 };
// fxp ImagOut_fft0_fxp[FFT_SIZE] __attribute__ ((aligned (4))) __attribute__((section(".xheep_data_interleaved"))) = { 0 };
fxp RealOut_fft0_fxp[DATA_SIZE] __attribute__ ((aligned (4))) __attribute__((section(".xheep_data_interleaved"))) = { 0 };
fxp ImagOut_fft0_fxp[DATA_SIZE] __attribute__ ((aligned (4))) __attribute__((section(".xheep_data_interleaved"))) = { 0 };


uint16_t ReverseBits (uint16_t index, uint16_t numBits);
uint16_t NumberOfBitsNeeded (uint16_t powerOfTwo);
void cpu_fft_radix2(fxp *real, fxp *imag, int n);

uint32_t t1, t2, t1_total, t2_total;




// one dim per core x n input values (data ptrs, constants, ...)
int32_t cgra_input[CGRA_N_COLS][CGRA_N_SLOTS][10] __attribute__ ((aligned (4))) = { 0 };
static int8_t cgra_intr_flag;

static void handler_irq_cgra( uint32_t int_id )
{
  if (int_id == CGRA_INTR) {
    cgra_intr_flag = 1;
  }
}


int main(void) {

  /* --------------------------------------------------------------------------
   *                    Initialization
   * --------------------------------------------------------------------------*/
  
  timer_cycles_init();
  timer_start();
  

  cgra_cmem_init(cgra_imem_bitstream, cgra_kmem_bitstream);
  uint32_t t_cgra_init = timer_get_cycles() - t1;


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

  //////////////////////////////////////////////////////////
  //
  // COMPLEX FFT radix-2 (Butterfy) implementation
  //
  //////////////////////////////////////////////////////////

  t1 = timer_get_cycles();
  
  cgra_perf_cnt_enable(&cgra, 1);
  uint16_t numBits = NumberOfBitsNeeded ( FFT_SIZE );
  int8_t column_idx;


  for (int i = 0; i < DATA_SIZE; i+=FFT_SIZE) {

    // STEP 1: bit reverse
    // PRINTF("Run input bit reverse reordering on %d points on CGRA...\n", FFT_SIZE);

    // Select request slot of CGRA (2 slots)
    uint32_t cgra_slot = cgra_get_slot(&cgra);
    column_idx = 0;
    cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[column_idx][cgra_slot], column_idx);

    // input data ptr column 0
    cgra_input[column_idx][cgra_slot][0] = (int32_t)&input_signal[i+1]; // imaginary part is given second
    cgra_input[column_idx][cgra_slot][1] = (int32_t)&input_signal[i]; // imaginary part is given first
    cgra_input[column_idx][cgra_slot][2] = (int32_t)FFT_SIZE/2; // idx end
    cgra_input[column_idx][cgra_slot][3] = (int32_t)numBits;
    cgra_input[column_idx][cgra_slot][4] = (int32_t)&ImagOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][5] = (int32_t)&RealOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][6] = 0; // idx start

    // Launch CGRA kernel
    cgra_set_kernel(&cgra, cgra_slot, CGRA_FTT_BITREV_ID);
    cgra_slot = cgra_get_slot(&cgra);
    column_idx = 0;
    cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[column_idx][cgra_slot], column_idx);

    // input data ptr column 0
    cgra_input[column_idx][cgra_slot][0] = (int32_t)&input_signal[i+FFT_SIZE/2+1]; // imaginary part is given second
    cgra_input[column_idx][cgra_slot][1] = (int32_t)&input_signal[i+FFT_SIZE/2]; // imaginary part is given first
    cgra_input[column_idx][cgra_slot][2] = (int32_t)FFT_SIZE; // idx end
    cgra_input[column_idx][cgra_slot][3] = (int32_t)numBits;
    cgra_input[column_idx][cgra_slot][4] = (int32_t)&ImagOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][5] = (int32_t)&RealOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][6] = FFT_SIZE/2; // idx start

    // Launch CGRA kernel
    cgra_set_kernel(&cgra, cgra_slot, CGRA_FTT_BITREV_ID);
    cgra_intr_flag=0;
    while(cgra_intr_flag==0) {wait_for_interrupt();}


    cgra_slot = cgra_get_slot(&cgra);
    column_idx = 0;
    cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[column_idx][cgra_slot], column_idx);

    // input data ptr column 0
    cgra_input[column_idx][cgra_slot][0] = (int32_t)&RealOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][1] = (int32_t)&f_real[0];
    cgra_input[column_idx][cgra_slot][2] = (int32_t)FFT_SIZE;

    column_idx = 1;
    cgra_set_read_ptr(&cgra, cgra_slot, (uint32_t) cgra_input[column_idx][cgra_slot], column_idx);

    // input data ptr column 0
    cgra_input[column_idx][cgra_slot][0] = (int32_t)&f_imag[0];
    cgra_input[column_idx][cgra_slot][1] = (int32_t)&ImagOut_fft0_fxp[i];
    cgra_input[column_idx][cgra_slot][2] = (int32_t)numBits;

    // Launch CGRA kernel
    cgra_set_kernel(&cgra, cgra_slot, CGRA_FTT_CPLX_ID);

    // Wait CGRA is done
    cgra_intr_flag=0;
    while(cgra_intr_flag==0) {
      wait_for_interrupt();
    }

}

uint32_t t2 = timer_get_cycles() - t1;

PRINTF("compute_fft_complex CGRA for %d data with size %d took %d cycles\n", DATA_SIZE, FFT_SIZE, t2);

  return EXIT_SUCCESS;
}

uint16_t ReverseBits (uint16_t index, uint16_t numBits)
{
  uint16_t i, rev;

  for (i=rev=0; i<numBits; i++) {
    rev = (rev << 1) | (index & 1);
    index >>= 1;
  }

  return rev;
}

uint16_t NumberOfBitsNeeded (uint16_t powerOfTwo)
{
  uint16_t i;

  if (powerOfTwo < 2) {
   return 0; // should not happen
  }

  for (i=0;; i++) {
    if (powerOfTwo & (1 << i))
      return i;
  }
}


// --- CPU FFT Implementation ---
void cpu_fft_radix2(fxp *real, fxp *imag, int n) {
  if (n < 2) {
      // Base case: nothing to do for n = 1
      return;
  }

  int numBits = NumberOfBitsNeeded(n);

  // Bit-reversal permutation
  for (int i = 0; i < n; i++) {
      int j = ReverseBits(i, numBits);
      if (j > i) {
          // Swap real and imaginary parts
          fxp temp_real = real[i];
          fxp temp_imag = imag[i];
          real[i] = real[j];
          imag[i] = imag[j];
          real[j] = temp_real;
          imag[j] = temp_imag;
      }
  }

  // Cooley-Tukey algorithm
  for (int s = 1; s <= numBits; s++) {
      int m = 1 << s;       // Butterfly size (2, 4, 8, ..., n)
      int half_m = m >> 1;  // m/2

      for (int k = 0; k < n; k += m) {
          for (int j = 0; j < half_m; j++) {
              // Calculate twiddle factor index (using lookup table)
              int twiddle_index = j * (FFT_SIZE / m);

              fxp w_real, w_imag;

              //important part for handling 512, 1024, ... FFT sizes
              if (twiddle_index < FFT_SIZE / 2) {
                  w_real = f_real[twiddle_index];
                  w_imag = f_imag[twiddle_index];
              }
              else
              {
                  w_real = f_real[twiddle_index - FFT_SIZE / 2];
                  w_imag = -f_imag[twiddle_index - FFT_SIZE / 2];
              }

              // Butterfly operation
              fxp t_real = fxp_mult(w_real, real[k + j + half_m]) - fxp_mult(w_imag, imag[k + j + half_m]);
              fxp t_imag = fxp_mult(w_real, imag[k + j + half_m]) + fxp_mult(w_imag, real[k + j + half_m]);

              fxp u_real = real[k + j];
              fxp u_imag = imag[k + j];

              real[k + j] = u_real + t_real;
              imag[k + j] = u_imag + t_imag;
              real[k + j + half_m] = u_real - t_real;
              imag[k + j + half_m] = u_imag - t_imag;
          }
      }
  }
}