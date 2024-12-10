//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#ifndef FVLLMONTITRANSFORMER_SOFTMAXC_H
#define FVLLMONTITRANSFORMER_SOFTMAXC_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "../param.h"

void computeSoftmax(int16_t* input, size_t seq_len);
void computeSoftmax_nonsquare(int16_t* input, size_t num_rows, size_t num_cols);

#define FIXED_MAX(a, b) ((a) > (b) ? (a) : (b))

#endif //FVLLMONTITRANSFORMER_SOFTMAXC_H
