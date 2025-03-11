#ifndef _DSIP_FFT_H_
#define _DSIP_FFT_H_

#include <stdlib.h>
#include <stdint.h>

#include "fxp.h"

// void compute_fft(fxp *fft_real_in, fxp *fft_im_in, fxp *fft_real_out, fxp *fft_im_out);

uint16_t ReverseBits (uint16_t index, uint16_t nbits);
uint16_t NumberOfBitsNeeded (uint16_t PowerOfTwo);
void fft_cplx_radix2_nbrev(fxp *Real_Out, fxp *Imag_Out, fxp *f_real, fxp *f_imag, int fft_s, int nbits);


#endif // _DSIP_FFT_H_
