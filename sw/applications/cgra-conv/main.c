#include "kernels_common/kernels_common.h"
#include "data.h"
#include "timer_sdk.h"
#include "conv_wc.h"
#include "conv_oc.h"
#include "conv_im2c_oc.h"
#include "conv_im2c_ic.h"

// void __attribute__((optimize("Ofast"))) conv2D();
void conv2D();

static kcom_kernel_t *kernels[] = {
      &conv_wc_kernel,
      &conv_oc_kernel,
      &conv_im2c_oc_kernel,
      // &conv_im2c_ic_kernel, //this one is not working
};

static kcom_perf_t kperf;

void main()
{
      uint32_t time = 0;
	uint8_t kernels_n = sizeof(kernels) / sizeof(kcom_kernel_t *);
	kcom_kernel_t *kernel;

	kcom_init();

	for (uint8_t ker_idx = 0; ker_idx < kernels_n; ker_idx++){
            kernel = kernels[ker_idx];
            uint8_t kernel_id = (ker_idx % (CGRA_KMEM_SIZE - 1)) + 1; // Must be between 1 and (KMEM_SIZE - 1).
            kernel->kmem[kernel_id] = kernel->kmem[1];                // By default the kernels come located with id = 1.
            
            kcom_load(kernel);
            /* Load (of inputs). */
            kernel->config(0);
            /* CGRA Execution */
            kcom_perfRecordIntrSet(&(kperf.time.cgra));
            timer_cycles_init();
            timer_start();
            kcom_launchKernel(kernel_id);
            kcom_waitingForIntr();
            time = timer_stop();
            printf("cgra-%s: %d\n", kernel->name, time);

      }

      // include sw time
      timer_cycles_init();
      timer_start();
      conv2D();
      time = timer_stop();
      printf("cpu: %d\n", time);


	return 0;
}

void conv2D()
{
    int32_t l, r, c, k, i, j, w, t;
    int32_t S;
    int32_t coeff;
    int32_t data;

    for (l = 0; l < N_output; l++)//N_output = 1
    {
        for (k = 0; k < N_filter; k++) // N_filter = 16
        {
            for (r = 0; r < row_output; r++) //row_output = 14
            {
                for (c = 0; c < col_output; c++) //col_output = 14
                {
                    S = 0;
                    for (i = 0; i < row_filter; i++)
                    {
                        for (j = 0; j < col_filter; j++)
                        {
                            for (w = 0; w < C_filter; w++)
                            {
                                data    = input[l][r+i][c+j][w];
                                coeff   = filter[k][i][j][w];
                                S        += coeff*data;
                            }
                        }
                    }
                    CPU_output[l][k][r][c] = S;
                }
            }
        }
    }
}

void  im2col_conv(int32_t *in, int out_row, int out_col, int output_channel)
{

    int i, j, k, l, c, m, n, o, p, q, r, s, t, u, v, w;

    for (i = 0; i < row_filter; i++)
    {

        in[0 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][0];
        in[1 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][1];
        in[2 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][2];
        in[3 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][3];
        in[4 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][4];
        in[5 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][5];
        in[6 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][6];
        in[7 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][7];
        in[8 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][8];
        in[9 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][9];
        in[10 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][10];
        in[11 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][11];
        in[12 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][12];
        in[13 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][13];
        in[14 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][14];
        in[15 + C_filter * 0 + C_filter * col_filter*i] = input[0][i+out_row][0+out_col][15];
        in[0 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][0];
        in[1 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][1];
        in[2 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][2];
        in[3 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][3];
        in[4 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][4];
        in[5 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][5];
        in[6 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][6];
        in[7 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][7];
        in[8 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][8];
        in[9 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][9];
        in[10 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][10];
        in[11 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][11];
        in[12 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][12];
        in[13 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][13];
        in[14 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][14];
        in[15 + C_filter * 1 + C_filter * col_filter*i] = input[0][i+out_row][1+out_col][15];
        in[0 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][0];
        in[1 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][1];
        in[2 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][2];
        in[3 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][3];
        in[4 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][4];
        in[5 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][5];
        in[6 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][6];
        in[7 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][7];
        in[8 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][8];
        in[9 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][9];
        in[10 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][10];
        in[11 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][11];
        in[12 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][12];
        in[13 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][13];
        in[14 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][14];
        in[15 + C_filter * 2 + C_filter * col_filter*i] = input[0][i+out_row][2+out_col][15];
    }

}


// Apparantely it is used for im2c_oc and oc kernels
// loading_buffer(kernel->output);
void loading_buffer_oc(int32_t **cgra_output){
    for(int i = 0; i < 4 ; i++){
        CGRA_output[4*i]  [0][0] = cgra_output[0][i];
        CGRA_output[4*i+1][0][0] = cgra_output[1][i];
        CGRA_output[4*i+2][0][0] = cgra_output[2][i];
        CGRA_output[4*i+3][0][0] = cgra_output[3][i];
    }
}

// Apparantely it is used for im2c_ic
void loading_buffer_im2c_ic(int32_t **cgra_output){
    CGRA_output[0][0][0]=cgra_output[3][0];
}