//
// Created by alireza on 10/6/23.
// NMC version by Francesco Poluzzi
//

#include <stdio.h>
// #include <math.h>
#include "transformer.h"
#include "data_cpp/signal.h"
#include "data_cpp/signal_fft.h"
#include "SYLT-FFT/fft.h"
#include "weightsAndBiasesC.h"
#include "transformerBlockC.h"
#include "x-heep.h"
#include "timer_sdk.h"
#include "w25q128jw.h"
#include "dma_sdk.h"
#include "defines.h"
#include "stft_amp.h"

#define FS_INITIAL 0x01

// initalize global timing variables
uint32_t t_tmp = 0;
uint32_t t_logamp = 0;
uint32_t t_gelu = 0;
uint32_t t_softmax = 0;
uint32_t t_hanning = 0;
uint32_t t_fft = 0;
uint32_t t_norm = 0;
uint32_t t_matmul_add = 0;
uint32_t t_matmul = 0;
uint32_t t_add = 0;
uint32_t t_clsconcat = 0;
uint32_t t_transpose = 0;
uint32_t t_mh_transpose = 0;
uint32_t t_mm_scale = 0;
uint32_t t_euc_dist = 0;

void print_kernels_cycles(void);

float error_check(const quant_bit_width *groundTruth, const quant_bit_width *output, size_t length)
{
    long error = 0;
    for (int i = 0; i < length; i++)
    {
        error += MUL_HQ(groundTruth[i] - output[i], groundTruth[i] - output[i]);
    }
    error = (error >> NUM_FRACTION_BITS);
    return (float)error / (float)length;
}

// calculates the Euclidean distances between the output of the Transformer network and a set of prototype vectors
void prototype_distances(quant_bit_width *prototypeVec, const quant_bit_width *modelOutput, int32_t *distVec, size_t prototypeLength, int prototypeNums)
{
    for (int p = 0; p < prototypeNums; p++)
    {
        long dist = 0;
        quant_bit_width *prototypePtr = prototypeVec + (p * prototypeLength);
        for (int i = 0; i < prototypeLength; i++)
        {
            dist += MUL_HQ(prototypePtr[i] - modelOutput[i], prototypePtr[i] - modelOutput[i]);
        }
        dist = (dist >> NUM_FRACTION_BITS);
        distVec[p] = (int32_t)dist;
    }
}

void transformerInference(quant_bit_width *transformerInput, quant_bit_width *transformerOutput, quant_bit_width *input_normalized, quant_bit_width *qkv, quant_bit_width *intermediate)
{
    quant_bit_width *weightVec[NUM_LAYERS * (3 * NUM_HEAD + 5) + 5];
    quant_bit_width *biasVec[NUM_LAYERS * (3 * NUM_HEAD + 5) + 5];
    getWeights(weightVec); // obtain an ordered list of weights
    getBiases(biasVec);    // obtain an ordered list of biases
    // the CLS token is a special token that is prepended to the input sequence before the transformer processes the data.
    quant_bit_width *clsTokenVector = getClassToken();
    // the position embedding is a matrix that contains the position of each token in the input sequence.
    quant_bit_width *posMatrix = getPosEmbedding();
    // create the transformer struct containing all the necessary information for inference
    TransformerBlock *selfatten = createTransformerBlock(D_SEQ, D_MODEL, D_Q, NUM_HEAD, D_FF, weightVec, biasVec, clsTokenVector, posMatrix);
    computeFixedPoint(selfatten, D_SEQ, transformerInput, input_normalized, transformerOutput, intermediate, qkv, ram_buffer);
}

void initialize_stft(fft_complex_t *data, const quant_bit_width *raw_input_signal)
{   
    // Initialize each element of the data array
    for (int i = 0; i < 256; i++)
    {
        // hanning values are multiiplied to the input signal of FFT to reduce the spectral leakage
        data[i].r = (MUL_HQ(raw_input_signal[i], hanning[i]));
        data[i].i = 0;
    }
    for (int i = 256; i < 512; i++) // last 256 elements are set to zero
    {
        data[i].r = 0;
        data[i].i = 0;
    }
}

void stft_rearrange(quant_bit_width *rawInputSignal, quant_bit_width *stftVec, size_t patchHeight, size_t patchWidth)
{
    fft_complex_t *data = (fft_complex_t*) &ram_buffer[7216]; // 4096 Bytes
    int overlap = 64; // The overlap between consecutive windows for the STFT
    for (int ch = 0; ch < 20; ch++) // Each channel represents a different segment of the rawInputSignal
    {
        for (int time_step = 0; time_step < 15; time_step++) // 15 time steps (or frames) for each channel. Each frame represents a time window on which the FFT will be performed
        {
            t_tmp = timer_get_cycles();
            // starting position of the raw signal for the current channel and time step:
            quant_bit_width *rawSignalPtr = rawInputSignal + ch * 3072 + (256 - overlap) * time_step;
            initialize_stft(data, rawSignalPtr); // Initialize the data array (with hanning values)
            t_hanning += timer_get_cycles() - t_tmp;
            t_tmp = timer_get_cycles();
            fft_fft(data, 9); // Perform the FFT
            t_fft += timer_get_cycles() - t_tmp;
            t_tmp = timer_get_cycles();
            // starting position in the output stftVec where the results of the current FFT should be stored:
            quant_bit_width *stftVecPtr = stftVec + ch * 15 * 160 + (time_step / patchWidth) * patchWidth * patchHeight + (time_step % patchWidth);
            for (int index = 0; index < patchHeight; index++) //  first half of the frequency bins
            {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);// logarithmic amplitude (amplitude in dB)
                *stftVecPtr = stft_int; // store the result in the stftVec
                stftVecPtr += patchWidth; // move to the next position in the stftVec (increase by stride)
            }
            // skip over the output matrix and do the same for the second half of the sequence
            stftVecPtr += patchHeight * patchWidth * 2;
            for (int index = patchHeight; index < 2 * patchHeight; index++)
            {
                quant_bit_width stft_int = compute_log_amp(data[index].r, data[index].i);
                *stftVecPtr = stft_int;
                stftVecPtr += patchWidth;
            }
            t_logamp += timer_get_cycles() - t_tmp;
        }
    }
}

int main()
{
    // Initialize the DMA
    dma_sdk_init();
    // Pick the correct spi device based on simulation type
    spi_host_t* spi = spi_flash;
    // Init SPI host and SPI<->Flash bridge parameters
    if (w25q128jw_init(spi) != FLASH_OK){
        PRINTF("Error initializing SPI flash\n");
        return 1;
    } 
    quant_bit_width *stftVec = raw_signal;
    quant_bit_width *rawInputSignal = raw_signal + 160 * 15;
    quant_bit_width *out = raw_signal + 160 * 15 * 20;
    quant_bit_width *intermediate = raw_signal + 16 * 1024;
    quant_bit_width *qkv = out + 2048;
    quant_bit_width *input_normalized = out + 4096;
    int32_t distances[2];
    #if defined(PRINT_INTERMEDIATE_CYCLES) || defined(PRINT_TOTAL_CYCLES) || defined(PRINT_RESULTS) || defined(PRINT_MATRICES_SIZES) || defined(DEBUG_PRINTS)
        PRINTF("Start\n");
    #endif
    // Divide input sequence in windows, perform fft on each window and store the results in columns of stftVec 
    #if defined(PRINT_INTERMEDIATE_CYCLES) || defined(PRINT_TOTAL_CYCLES)
        timer_cycles_init();
        timer_start();
    #endif
    stft_rearrange(rawInputSignal, stftVec, 80, 5);
    #ifdef PRINT_INTERMEDIATE_CYCLES
        uint32_t fft_cycles = timer_stop();
        PRINTF("FFT cycles: %u\n", fft_cycles);
    #endif
    transformerInference(stftVec, out, input_normalized, qkv, intermediate);
    // calculate distances for classification
    t_tmp = timer_get_cycles();
    prototype_distances(prototypes, out, distances, D_MODEL, 2);
    t_euc_dist += timer_get_cycles() - t_tmp;
    #ifdef PRINT_TOTAL_CYCLES
       uint32_t total_cycles = timer_stop();
         PRINTF("Total cycles: %u\n", total_cycles);
    #endif

    #ifdef PRINT_KERNELS_CYCLES
        print_kernels_cycles();
    #endif

    #ifdef PRINT_RESULTS
        PRINTF("Distances : \n");
        for (int i = 0; i < 2; i++)
            PRINTF("From the prototype of class %d = %d\n", i, distances[i]);
        return 0;
    #endif
}

void print_kernels_cycles(void)
{
    PRINTF("hanning and zeroPad: %u\n", t_hanning);
    PRINTF("FFT: %u\n", t_fft);
    PRINTF("Log Amp: %u\n", t_logamp);
    PRINTF("norm: %u\n", t_norm);
    PRINTF("matmul_add: %u\n", t_matmul_add);
    PRINTF("matmul: %u\n", t_matmul);
    PRINTF("add: %u\n", t_add);
    PRINTF("clsconcat: %u\n", t_clsconcat);
    PRINTF("transpose: %u\n", t_transpose);
    PRINTF("mm_scale: %u\n", t_mm_scale);
    PRINTF("mh_transpose: %u\n", t_mh_transpose);
    PRINTF("gelu: %u\n", t_gelu);
    PRINTF("softmax: %u\n", t_softmax);
    PRINTF("euc_dist: %u\n", t_euc_dist);
}
