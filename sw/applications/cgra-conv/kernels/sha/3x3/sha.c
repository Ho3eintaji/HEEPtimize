/*
                              *******************
******************************* C SOURCE FILE *******************************
**                            *******************                          **
**                                                                         **
** project  : CGRA-X-HEEP                                                  **
** filename : sha.c                                                 **
** version  : 1                                                            **
** date     : 2023-07-11                                                       **
**                                                                         **
*****************************************************************************
**                                                                         **
** Copyright (c) EPFL                                                      **
** All rights reserved.                                                    **
**                                                                         **
*****************************************************************************
*/

/***************************************************************************/
/***************************************************************************/

/**
* @file   sha.c
* @date   2023-07-11
* @brief  A description of the kernel...
*
*/

#define _SHA_C

/****************************************************************************/
/**                                                                        **/
/*                             MODULES USED                                 */
/**                                                                        **/
/****************************************************************************/
#include <stdint.h>

#include "sha.h"
#include "../function.h"

/****************************************************************************/
/**                                                                        **/
/*                        DEFINITIONS AND MACROS                            */
/**                                                                        **/
/****************************************************************************/

#define CGRA_COLS       3
#define IN_VAR_DEPTH    3
#define OUT_VAR_DEPTH   3

/****************************************************************************/
/**                                                                        **/
/*                      PROTOTYPES OF LOCAL FUNCTIONS                       */
/**                                                                        **/
/****************************************************************************/

static void        config  (void);
static void        software(void);
static uint32_t    check   (void);

/****************************************************************************/
/**                                                                        **/
/*                            GLOBAL VARIABLES                              */
/**                                                                        **/
/****************************************************************************/

const uint32_t  cgra_imem_sha[CGRA_IMEM_SIZE] = {  0x0, 0x0, 0x2a081ff8, 0x2a180004, 0x0, 0x0, 0x2a081ff8, 0x2a180004, 0x0, 0x43c00000, 0x2a081ff8, 0x2a180004, 0x0, 0x43c00000, 0x0, 0x0, 0x0, 0x43c00000, 0xc80000, 0xa90000, 0x0, 0x0, 0x0, 0x0, 0x62080000, 0x0, 0x0, 0x0, 0x62080000, 0x0, 0x0, 0x0, 0x62080000, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa080010, 0x10080000, 0x0, 0x1a080001, 0x0, 0x10080000, 0x0, 0x1a080001, 0x0, 0x10080000, 0x0, 0x1a080001, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa90000, 0x0, 0x0, 0x4a180004, 0x61080000, 0x1b80000, 0x15500000, 0x4a180004, 0x61080000, 0x1b80000, 0x15500000, 0x4a180004, 0x61080000, 0x1b80000, 0x15500000, 0x0, 0x0, 0x0, 0x0, 0xa90000, 0x0, 0x0, 0x3a180004, 0x3a180004, 0x0, 0x61080000, 0x3a180004, 0x3a180004, 0x0, 0x61080000, 0x3a180004, 0x3a180004, 0x0, 0x61080000, 0x0, 0x0, 0x0, 0x0, 0xa90000, 0x0, 0x4a081ffd, 0x4a081ff0, 0x4680000d, 0x0, 0x4a081ffd, 0x4a081ff0, 0x4680000d, 0x0, 0x4a081ffd, 0x4a081ff0, 0x46880009, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3b80000, 0x2b80000, 0x41500000, 0x13500000, 0x3b80000, 0x2b80000, 0x41500000, 0x13500000, 0x3b80000, 0x2b80000, 0x41500000, 0x13500000, 0x0, 0x0, 0xa90000, 0x0, 0x0, 0x0, 0x64080000, 0x0, 0x0, 0x4b80000, 0x64080000, 0x0, 0x0, 0x4b80000, 0x64080000, 0x0, 0x0, 0x4b80000, 0x0, 0x0, 0x0, 0xa90000, 0x0, 0x5a081ff2, 0x1a180004, 0x0, 0x61080000, 0x5a081ff2, 0x1a180004, 0x0, 0x61080000, 0x5a081ff2, 0x1a180004, 0x0, 0x61080000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  };
static uint32_t cgra_kmem_sha[CGRA_KMEM_SIZE] = {  0x0, 0x7012, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
 };

static int32_t cgra_input[CGRA_COLS][IN_VAR_DEPTH]     __attribute__ ((aligned (4)));
static int32_t cgra_output[CGRA_COLS][OUT_VAR_DEPTH]   __attribute__ ((aligned (4)));

static uint32_t	i_w_soft[80];
static uint32_t	i_w_cgra[80];

static uint32_t	o_ret_soft;
static uint32_t	o_ret_cgra;


/****************************************************************************/
/**                                                                        **/
/*                           EXPORTED VARIABLES                             */
/**                                                                        **/
/****************************************************************************/

extern kcom_kernel_t sha_kernel = {
    .kmem   = cgra_kmem_sha,
    .imem   = cgra_imem_sha,
    .col_n  = CGRA_COLS,
    .in_n   = IN_VAR_DEPTH,
    .out_n  = OUT_VAR_DEPTH,
    .input  = cgra_input,
    .output = cgra_output,
    .config = config,
    .func   = software,
    .check  = check,
    .name   = "Sha",
};

/****************************************************************************/
/**                                                                        **/
/*                            LOCAL FUNCTIONS                               */
/**                                                                        **/
/****************************************************************************/

void config()
{
	i_w_soft = kcom_getRand() % (UINT_MAX - 1 - 0 + 1) + 0;
	i_w_cgra = i_w_soft;
	cgra_input[0][0] = i_w_cgra;
	cgra_input[1][0] = i_w_cgra;
	cgra_input[1][1] = i_w_cgra;
	cgra_input[1][2] = i_w_cgra;
	cgra_input[2][0] = 80;
	cgra_input[2][1] = i_w_cgra;

}

void software(void) 
{
    o_ret_soft = sha( i_w_soft );
}

uint32_t check(void) 
{
    uint32_t errors = 0;
    
	o_ret_cgra = cgra_output[0][0];
	o_ret_cgra = cgra_output[0][0];
	o_ret_cgra = cgra_output[0][0];


#if PRINT_CGRA_RESULTS
    PRINTF("------------------------------\n");
    for( uint8_t c = 0; c < CGRA_COLS; c ++)
    {
        for( uint8_t r = 0; r < OUT_VAR_DEPTH; r++ )
        {
            PRINTF("[%d][%d]:%08x\t\t",c,r,cgra_output[c][r]);
        }
        PRINTF("\n");
    }
#endif //PRINT_CGRA_RESULTS


#if PRINT_RESULTS
        PRINTF("\nCGRA\t\tSoft\n");
#endif

    for( int i = 0; i < 1; i++ )
    {
#if PRINT_RESULTS
        PRINTF("%08x\t%08x\t%s\n",
        o_ret_cgra,
        o_ret_soft,
        (o_ret_cgra != o_ret_soft) ? "Wrong!" : ""
        );
#endif

        if (o_ret_cgra != o_ret_soft) {
            errors++;
        }
    }
    return errors;
}

/****************************************************************************/
/**                                                                        **/
/**                                EOF                                     **/
/**                                                                        **/
/****************************************************************************/