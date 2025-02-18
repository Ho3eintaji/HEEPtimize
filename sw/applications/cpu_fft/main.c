#include <stdio.h>
#include <stdlib.h>
#include "defines.h"
#include "SYLT-FFT/fft.h"
#include "timer_sdk.h"

int main(void) {
    // Example input of size FFT_SIZE (1024 = 2^10)
    static fft_complex_t data[FFT_SIZE];

    // Fill input data (for this example, just zero it)
    for (int i = 0; i < FFT_SIZE; i++) {
        data[i].r = i*i;
        data[i].i = i+1;
    }
    // Put some sample values
    data[0].r = 1024; // example non-zero input

    // Perform the FFT using the library function (assuming it’s named fft_fft)
    timer_cycles_init();
        timer_start();


    fft_fft(data, NUM_BITS);

    uint32_t time_cpu = timer_stop();

        printf("CPU FFT cycles: %d\n", time_cpu);

    return 0;
}