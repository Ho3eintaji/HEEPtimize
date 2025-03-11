#include <stdio.h>
#include <stdlib.h>

#include "heepatia.h"
#include "fft.h"
#include "defines.h"
#include "fxp.h"
#include "data.h"
#include "timer_sdk.h"

#define DATA_SIZE 30*512 //16*1024

uint32_t t1, t2;

fxp data_re_out[DATA_SIZE] = {0};
fxp data_im_out[DATA_SIZE] = {0};

int main(void) {
  uint32_t timer_cycles;

  timer_cycles_init();
  timer_start();

  // FFT execution
  uint16_t NumBits = NumberOfBitsNeeded ( FFT_SIZE );
  fxp tmp;


  t1 = timer_get_cycles();
  // I want to run it for whole data in a loop
  for (int i = 0; i < DATA_SIZE; i+=FFT_SIZE) {

    fft_cplx_radix2_nbrev(&data_re_out[i], &data_im_out[i], &dsip_data_fft[0], &dsip_data_fft[FFT_SIZE/2], FFT_SIZE, NumBits);

    // Require a bit reverse for output
    for (int32_t j=0; j<FFT_SIZE; j++ ) {
      uint16_t revBits = ReverseBits ( j, NumBits );
      if (revBits < j) {
        continue;
      }
      tmp = data_re_out[i+j];
      data_re_out[i+j] = data_re_out[i+revBits];
      data_re_out[i+revBits] = tmp;

      tmp = data_im_out[i+j];
      data_im_out[i+j] = data_im_out[i+revBits];
      data_im_out[i+revBits] = tmp;
    }
  }

  t2 = timer_get_cycles() - t1;
  PRINTF("compute_fft_complex for %d data with size %d took %d cycles\n", DATA_SIZE, FFT_SIZE, t2);


  return EXIT_SUCCESS;
}
