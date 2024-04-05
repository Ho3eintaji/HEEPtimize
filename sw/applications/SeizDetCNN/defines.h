#include <stdlib.h>
#include <stdint.h>

#ifndef EPILEPSYGAN_GAN_H
#define EPILEPSYGAN_GAN_H

#define MAX_INT_16 (1<<15)

#define NUM_FRACTION_DATA 10
#define NUM_FRACTION_CNV_FC 8
#define NUM_FRACTION_BN 5
#define NEG_INF (-(1<<14))
#define INPUT_LEN (23 * 1024)

#define MUL_CONV(x, y, num) (int32_t)((int)(x)*(int)(y))>>(num)
#define MUL(x, y, num) (short)(((int)(x)*(int)(y))>>(num))

#define mem2d(data,data_len,j,i)   data[((j)*(data_len))+(i)]
#define mem3d(filter,filter_len,filter_depth,n,k,i)   filter[((n)*(filter_depth)+(k))*(filter_len)+(i)]

extern int16_t input_array[INPUT_LEN];

extern int8_t* conv1d_w[3];
extern int8_t* conv1d_b[3];
extern int8_t* dense_w[2];
extern int8_t* dense_b[2];
extern int8_t* bn[12];


// Optimization flags
// #define CONV_MAX_OPTIMIZED
// #define CONV_OPTIMIZED
// #define BATCH_OPTIMIZED


// Debug flags
#define DEBUG
#define PRINT_OVERFLOW
#define BATCH_OPTIMIZED
#define CNT_CYCLES
#define RUN

#endif //EPILEPSYGAN_GAN_H
