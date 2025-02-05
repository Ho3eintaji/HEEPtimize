
#include "kernels/conv-oc/conv.h"


void __attribute__((optimize("Os"))) im2col_conv(int32_t *in, int out_row, int out_col, int output_channel);
void loading_buffer(int32_t **cgra_output);

int32_t output_from_CGRA[N_filter][row_output][col_output] __attribute__ ((aligned (4)));
int32_t input_to_CGRA[row_filter * col_filter * C_filter];

static kcom_kernel_t *kernels[] = {
    &conv_kernel,
};

static kcom_perf_t  kperf;

void main()
{
    uint8_t kernels_n = sizeof( kernels ) / sizeof( kcom_kernel_t * );
    kcom_kernel_t* kernel;

    kcom_init();

    uint8_t ker_idx = 0;

    kernel = kernels[ ker_idx ];
    uint8_t kernel_id = ( ker_idx % (CGRA_KMEM_SIZE - 1) ) + 1; // Must be between 1 and (KMEM_SIZE - 1).
    kernel->kmem[ kernel_id ] = kernel->kmem[1]; // By default the kernels come located with id = 1.

    kcom_load( kernel );

    for( uint16_t it_idx = 0; it_idx < 2; it_idx++ )
    {
        /* Load (of inputs). */
        kcom_newVCDfile();
        kernel->config(0);

        kcom_perfRecordIntrSet( &(kperf.time.cgra) );
        kcom_perfRecordStart(   &(kperf.time.cgra) );
        kcom_launchKernel( kernel_id );
        kcom_waitingForIntr();

        kcom_perfRecordStart(   &(kperf.time.dead) );
        loading_buffer(kernel->output);
        kcom_perfRecordStop(    &(kperf.time.dead) );


    }
    return 0;
}


void loading_buffer(int32_t **cgra_output){
    for(int i = 0; i < 4 ; i++){
        output_from_CGRA[4*i]  [0][0] = cgra_output[0][i];
        output_from_CGRA[4*i+1][0][0] = cgra_output[1][i];
        output_from_CGRA[4*i+2][0][0] = cgra_output[2][i];
        output_from_CGRA[4*i+3][0][0] = cgra_output[3][i];
    }
}