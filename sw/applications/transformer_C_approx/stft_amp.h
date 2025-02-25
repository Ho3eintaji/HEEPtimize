#ifndef STFT_AMP_H
#define STFT_AMP_H

#include <stdio.h>
#include "../param.h"

#ifdef __cplusplus
extern "C" {
#endif

quant_bit_width compute_log_amp(int32_t real, int32_t imag);

quant_bit_width compute_log_amp_fp(int32_t real, int32_t imag);
quant_bit_width compute_log_amp_fxp_lut(int32_t real, int32_t imag);
quant_bit_width compute_log_amp_fxp_approx(int32_t real, int32_t imag);

void init_log2_lut(void);
quant_bit_width compute_log_amp_fxp(int32_t real, int32_t imag);

#ifdef __cplusplus
}
#endif

#endif // STFT_AMP_H