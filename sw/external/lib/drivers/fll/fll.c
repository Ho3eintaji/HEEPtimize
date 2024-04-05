// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "x-heep.h"
#include "fll.h"

#define __FL1(x) (31 - __builtin_clz((x)))
#define __MAX(x,y) ((x)>(y)?(x):(y))

static uint32_t fll_get_freq_from_mult_div(uint32_t mult_factor, uint32_t clk_div)
{
  // mult_factor is 16 bits
  // clk_div is 4 bits
  uint32_t fll_freq = (REFERENCE_CLOCK_Hz*mult_factor)>>(clk_div-1);
  return fll_freq;
}

static uint32_t fll_get_mult_div_from_freq(uint32_t freq, uint32_t *mult, uint32_t *clk_div)
{
    uint32_t fref = REFERENCE_CLOCK_Hz;
    uint32_t Log2M = __FL1(freq) - __FL1(fref);
    uint32_t D = __MAX(1, (FLL_LOG2_MAXM - Log2M)>>1);
    uint32_t M = (freq<<D)/fref;
    uint32_t fres;

    fres = (fref*M + (1<<(D-1)))>>D;   /* Rounding */

    *mult = M; *clk_div = D+1;

    return fres;
}

uint32_t fll_init(const fll_t *fll)
{
  fll_conf1_reg_t fll_conf1 = fll_conf1_get(fll);

  // Only lock the fll if it is not already done by the boot code
  if (fll_conf1.op_mode == 0)
  {
    /* Set Clock Ref lock assert count */
    fll_conf2_reg_t fll_conf2 = fll_conf2_get(fll);
    fll_conf2.assert_cycles = 6;
    fll_conf2.lock_tolerance = 0x50;
    fll_conf2_set(fll, fll_conf2.raw);

    /* Lock Fll */
    fll_conf1.lock_enable = 1;
    fll_conf1.op_mode = 1;
    // Set FLL frequency to ~50 MHz (default value)
    fll_conf1.dco_input = 0x158; 
    fll_conf1_set(fll, fll_conf1.raw);
  }

  // Get the real frequency and don't forget to set the FLL frequency in SoC Controller
  uint32_t fll_freq = fll_get_freq_from_mult_div(fll_conf1.mult_factor, fll_conf1.clk_div);

  return fll_freq;
}

uint32_t fll_set_freq(const fll_t *fll, uint32_t freq)
{
  uint32_t real_freq, mult, clk_div;
  fll_conf1_reg_t fll_conf1;

  real_freq = fll_get_mult_div_from_freq(freq, &mult, &clk_div);

  fll_conf1 = fll_conf1_get(fll);
  // Init FLL if not already done
  if (fll_conf1.op_mode == 0) {
    fll_init(fll);
  }

  // Set FLL frequency
  fll_conf1 = fll_conf1_get(fll);
  fll_conf1.mult_factor = mult;
  fll_conf1.clk_div = clk_div;
  fll_conf1_set(fll, fll_conf1.raw);

  return real_freq;
}

uint32_t fll_get_freq(const fll_t *fll)
{
  uint32_t fll_status;
  fll_conf1_reg_t fll_conf1;

  fll_status = fll_status_get(fll);
  fll_conf1 = fll_conf1_get(fll);
  
  return ((fll_status*REFERENCE_CLOCK_Hz)/fll_conf1.clk_div);
}
