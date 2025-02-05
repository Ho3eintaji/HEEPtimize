
#include "kernels/conv-im2c-ic/conv.h"


void __attribute__((optimize("Os"))) im2col_conv(int32_t *input_to_CGRA, int out_row, int out_col, int output_channel);
void loading_buffer(int32_t **cgra_output);


static kcom_kernel_t *kernels[] = {
    &conv_kernel,
};

static kcom_perf_t  kperf;

void main()
{
    uint8_t kernels_n = sizeof( kernels ) / sizeof( kcom_kernel_t * );
    kcom_kernel_t* kernel;

    kcom_init();

    for( uint8_t ker_idx = 0; ker_idx < kernels_n; ker_idx++ )
    {
        kernel = kernels[ ker_idx ];
        /* Set the kernel ID */
        uint8_t kernel_id = ( ker_idx % (CGRA_KMEM_SIZE - 1) ) + 1; // Must be between 1 and (KMEM_SIZE - 1).
        kernel->kmem[ kernel_id ] = kernel->kmem[1]; // By default the kernels come located with id = 1.
        // The kernel = 1 is kept, so we can always take it from there.

        PRINTF(" %s\n",  kernel->names );

        kcom_load( kernel );

        for( uint16_t it_idx = 0; it_idx < 1; it_idx++ )
        {
            /* Load (of inputs). */
            if( it_idx < 2 )
            {
                kcom_newVCDfile();
                kcom_resetRand();
            }

            kcom_perfRecordIntrSet( &(kperf.time.sw) );
            kcom_perfRecordStart(   &(kperf.time.sw) );
            im2col_conv(input_to_CGRA, 0, 0, 0);
            kcom_perfRecordStop( &(kperf.time.sw));

            kernel->config();

            kcom_perfRecordIntrSet( &(kperf.time.cgra) );
            kcom_perfRecordStart(   &(kperf.time.cgra) );
            kcom_launchKernel( kernel_id );
            kcom_waitingForIntr();


            kcom_perfRecordStart(   &(kperf.time.dead) );
            loading_buffer(kernel->output);
            kcom_perfRecordStop(    &(kperf.time.dead) );
        }
    }
    return 0;
}



void __attribute__((optimize("Os"))) im2col_conv(int32_t *input_to_CGRA, int out_row, int out_col, int output_channel)
{

    int i, j, k, l, c, m, n, o, p, q, r, s, t, u, v, w;

    for (i = 0; i < row_filter; i++)
    {

        input_to_CGRA[0 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][0];
        input_to_CGRA[1 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][1];
        input_to_CGRA[2 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][2];
        input_to_CGRA[3 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][3];
        input_to_CGRA[4 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][4];
        input_to_CGRA[5 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][5];
        input_to_CGRA[6 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][6];
        input_to_CGRA[7 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][7];
        input_to_CGRA[8 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][8];
        input_to_CGRA[9 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][9];
        input_to_CGRA[10 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][10];
        input_to_CGRA[11 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][11];
        input_to_CGRA[12 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][12];
        input_to_CGRA[13 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][13];
        input_to_CGRA[14 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][14];
        input_to_CGRA[15 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][15];
        input_to_CGRA[0 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][0];
        input_to_CGRA[1 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][1];
        input_to_CGRA[2 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][2];
        input_to_CGRA[3 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][3];
        input_to_CGRA[4 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][4];
        input_to_CGRA[5 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][5];
        input_to_CGRA[6 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][6];
        input_to_CGRA[7 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][7];
        input_to_CGRA[8 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][8];
        input_to_CGRA[9 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][9];
        input_to_CGRA[10 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][10];
        input_to_CGRA[11 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][11];
        input_to_CGRA[12 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][12];
        input_to_CGRA[13 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][13];
        input_to_CGRA[14 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][14];
        input_to_CGRA[15 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][15];
        input_to_CGRA[0 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][0];
        input_to_CGRA[1 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][1];
        input_to_CGRA[2 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][2];
        input_to_CGRA[3 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][3];
        input_to_CGRA[4 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][4];
        input_to_CGRA[5 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][5];
        input_to_CGRA[6 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][6];
        input_to_CGRA[7 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][7];
        input_to_CGRA[8 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][8];
        input_to_CGRA[9 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][9];
        input_to_CGRA[10 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][10];
        input_to_CGRA[11 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][11];
        input_to_CGRA[12 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][12];
        input_to_CGRA[13 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][13];
        input_to_CGRA[14 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][14];
        input_to_CGRA[15 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][15];
    }

}

void loading_buffer(int32_t **cgra_output){
    output_from_CGRA[0][0][0]=cgra_output[3][0];
}