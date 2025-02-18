
#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h> // For log2 and ceil

/* --------------------------------------------------------------------------
 *                     Application defines
 * --------------------------------------------------------------------------*/

#define FFT_SIZE (1024) //512, 1024, 2048, 4096
#define NUM_BITS ((int)ceil(log2(FFT_SIZE)))

#endif // _DEFINES_H_
