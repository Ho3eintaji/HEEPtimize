// FFT execution on the DSIP (real or complex input)

#include "fft.h"

void fft_cplx_radix2_nbrev(fxp *Real_Out, fxp *Imag_Out, fxp *f_real, fxp *f_imag, int fft_s, int nbits)
{
  int32_t n_iter = 1;
  int32_t n_2 = fft_s/2;
  int32_t f_idx = n_iter;
  int32_t s_repetition = n_2;
  int32_t base_idx;

  int i;
  for (i=nbits; i>0; i--)  // max=numbits
  {
    int k;
    for (k=0; k<n_iter; k++) // N/2 << 1
    {
      base_idx = k << i;

      int j;
      for (j=0; j<s_repetition; j++)
      {
        fxp even_real = Real_Out[base_idx+j  ];   // even
        fxp even_imag = Imag_Out[base_idx+j  ];   // even

        fxp odd_real  = Real_Out[base_idx+j+n_2];   // odd
        fxp odd_imag  = Imag_Out[base_idx+j+n_2];   // odd

        // use look-up table
        fxp w_r = f_real[j * f_idx]; // (N_SAMPLE/N)
        fxp w_i = f_imag[j * f_idx]; // (N_SAMPLE/N)

        fxp p_sum_r = even_real - odd_real;
        fxp p_sum_i = even_imag - odd_imag;

        Real_Out[base_idx+j  ] = (even_real + odd_real);
        Real_Out[base_idx+j+n_2] = fxp_mult( p_sum_r, w_r ) - fxp_mult( p_sum_i, w_i );

        Imag_Out[base_idx+j  ] = (even_imag + odd_imag);
        Imag_Out[base_idx+j+n_2] = fxp_mult( p_sum_r, w_i ) + fxp_mult( p_sum_i, w_r );
      }
    }
    n_iter <<= 1;
    f_idx = n_iter;
    n_2 >>= 1;
    s_repetition = n_2;
  }
}

uint16_t ReverseBits(uint16_t index, uint16_t nbits) {
  uint16_t i, rev;
  for (i=rev=0; i<nbits; i++) {
    rev = (rev << 1) | (index & 1);
    index >>= 1;
  }
  return rev;
}

uint16_t NumberOfBitsNeeded(uint16_t PowerOfTwo) {
  uint16_t i;
  if (PowerOfTwo < 2) {
    return 0; // should not happen
  }
  for (i=0; ;i++) {
    if (PowerOfTwo & (1 << i))
      return i;
  }
}
