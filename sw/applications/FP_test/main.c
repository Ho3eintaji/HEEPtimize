#include <stdio.h>   // For printf
#include <stdint.h>  // For fixed-size integer types (optional, but good practice)
#include "timer_sdk.h"  // For timer functions

// Function to perform a simple floating-point calculation
float floating_point_calculation(float a, float b) {
    float result = (a * b);
    for (int i = 0; i < 1000; i++) {
        result = result + (a * b);
    }
    return result;
}

int main() {
    // 1. Initialize floating-point variables
    float num1;  // 'f' suffix denotes a float literal
    float num2 = 2.71828f;
    float result;

    scanf("%f", &num1); // Read a float from the user

    timer_cycles_init();
    timer_start();
    uint32_t t1, t2;

    // 2. Perform the calculation
    t1 = timer_get_cycles();
    result = floating_point_calculation(num1, num2);
    t2 = timer_get_cycles();

    // 3. Output the result using printf
    printf("Result: %f\n", result); // The %f format specifier is used for floats

    printf("CPU cycles: %u\n", t2 - t1);

    // // 4. (Optional) Format the output to a specific number of decimal places
    // printf("Result (2 decimal places): %.2f\n", result); // .2 limits to 2 places
    // printf("Result (6 decimal places): %.6f\n", result); // .6 limits to 6 places

    // 5. (Optional) Infinite loop (for embedded systems)
    // while (1) {}

    return 0;
}