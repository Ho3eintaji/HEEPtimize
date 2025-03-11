#include <stdio.h>
#include <stdlib.h>

#include "heepatia.h"
#include "fft.h"
#include "defines.h"
#include "fxp.h"
#include "data.h"
#include "timer_sdk.h"


fxp fft_re_out[FFT_SIZE] = { 0 };
fxp fft_im_out[FFT_SIZE] = { 0 };

int main(void) {
  printf("FFT kernel execution on Core-V-mini-MCU of size %d\n", FFT_SIZE);
  uint32_t timer_cycles;

  

  timer_cycles_init();
  timer_start();

  // FFT execution
  uint16_t NumBits = NumberOfBitsNeeded ( FFT_SIZE );
  fft_cplx_radix2_nbrev(&dsip_data_fft[0], &dsip_data_fft[FFT_SIZE], &dsip_data_fft[2*FFT_SIZE], &dsip_data_fft[2*FFT_SIZE+FFT_SIZE/2], FFT_SIZE, NumBits);

  // Require a bit reverse for output
  fxp tmp;
  for (int32_t i=0; i<FFT_SIZE; i++ ) {
    uint16_t revBits = ReverseBits ( i, NumBits );
    if (revBits < i) {
      continue;
    }
    tmp = dsip_data_fft[i];
    dsip_data_fft[i] = dsip_data_fft[revBits];
    dsip_data_fft[revBits] = tmp;

    tmp = dsip_data_fft[FFT_SIZE+i];
    dsip_data_fft[FFT_SIZE+i] = dsip_data_fft[FFT_SIZE+revBits];
    dsip_data_fft[FFT_SIZE+revBits] = tmp;
  }
  
  timer_cycles = timer_stop();
  PRINTF("compute_fft_complex took %d cycles\n", timer_cycles);

  int32_t errors;

  #ifdef CHECK_ERRORS
    fxp* RealOut_fxp_ptr;
    fxp* ImagOut_fxp_ptr;
    errors=0;
    #ifdef CPLX_FFT
      RealOut_fxp_ptr = &dsip_data_fft[0];
      ImagOut_fxp_ptr = &dsip_data_fft[FFT_SIZE];
      for(int i=0; i<FFT_SIZE; i++) {
    #else // REAL_FFT
      RealOut_fxp_ptr = &fft_re_out[0];
      ImagOut_fxp_ptr = &fft_im_out[0];
      int i;
      for(i=0; i<FFT_SIZE/2; i++) {
    #endif
      if (RealOut_fxp_ptr[i] != exp_output_real[i] || ImagOut_fxp_ptr[i] != exp_output_imag[i]) {
        PRINTF("output[%3d]: %08x + %08xi != %08x + %08xi\n", i, RealOut_fxp_ptr[i], ImagOut_fxp_ptr[i], exp_output_real[i], exp_output_imag[i]);
        errors++; // 0 errors expected
      }
    }
    printf("MCU %d FFT check finished with %d errors\n", FFT_SIZE, errors);
  #endif // CHECK_ERRORS

  return EXIT_SUCCESS;
}
