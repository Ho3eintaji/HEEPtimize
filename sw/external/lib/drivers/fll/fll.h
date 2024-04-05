// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef _DRIVERS_FLL_H_
#define _DRIVERS_FLL_H_

#include <stddef.h>
#include <stdint.h>

#include "mmio.h"
#include "fll_regs.h"

/* Maximum Log2(DCO Frequency) */
#define FLL_LOG2_MAXDCO     29
/* Maximum Log2(Clok Divider) */
#define FLL_LOG2_MAXDIV     15
/* Maximum Log2(Multiplier) */
#define FLL_LOG2_MAXM       (FLL_LOG2_MAXDCO - 26)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fll {
  /**
   * The base address for the FLL hardware registers.
   */
  mmio_region_t base_addr;
} fll_t;

// FLL registers structure

typedef struct fll_status_reg {
    uint32_t mult_factor : 16; /* Ccurrent multiplication factor (i.e., current frequency) */
} fll_status_reg_t;

typedef union fll_conf_1_reg {
  struct {
    uint32_t mult_factor : 16; /* Target clock multiplication factor for normal mode, reset=0x0C35 */
    uint32_t dco_input   : 10; /* DCO input code for stand-alone mode, reset=0x158 */
    uint32_t clk_div     : 4;  /* FLL output clock divider setting, reset=0x2 (e.i. div 4) */
    uint32_t lock_enable : 1;  /* FLL output gated by LOCK signal (active high), reset=1 */
    uint32_t op_mode     : 1;  /* Operation mode select (0=stand-alone, 1=normal), reset=0 */
  };
  uint32_t raw;
} fll_conf1_reg_t;

typedef union fll_conf_2_reg {
  struct {
    uint32_t loop_gain        : 4;  /* FLL loop gain setting (default: 2(−8) = 1/256), reset=0x8 */
    uint32_t deassert_cycles  : 6;  /* In normal mode: no. of unstable REFCLK cycles until LOCK deassert. In stand-alone mode: lower 6-bit of LOCK assert counter target, reset=0x10 */
    uint32_t assert_cycles    : 6;  /* In normal mode: no. of stable REFCLK cycles until LOCK assert. In stand-alone mode: upper 6-bit of LOCK assert counter target, reset=0x10 */
    uint32_t lock_tolerance   : 12; /* Lock tolerance: margin around the target multiplication factor within which the output clock is considered stable, reset=0x200
                                         With Fmax=250MHz (Div=2^4), Fmin=32K (Div=2^15)
                                         Tolerance: 32K*(512/16)=1.048MHz .. 512 Hz */
    uint32_t unused           : 1;  /* Unused bit 28 */
    uint32_t clk_sta_mode     : 1;  /* Config clock select in STA mode (0=DCOCLK, 1=REFCLK), reset=0 */
    uint32_t open_loop_enable : 1;  /* Open-loop-when-locked (active high), reset=0 */
    uint32_t dithering_enable : 1;  /* When 1 Dithering is enabled */
  };
  uint32_t raw;
} fll_conf2_reg_t;

typedef union fll_integrator_reg {
  struct {
    uint32_t unused    :  6; /* Unused lower bits */
    uint32_t frac_part : 10; /* Integrator state: fractional part (dither unit input) */
    uint32_t int_part  : 10; /* Integrator state: integer part (DCO input bits) */
  };
  uint32_t raw;
} fll_integrator_reg_t;


// FLL registers access functions

// Lock the FLL if not locked to default frequency of ~50 MHz
// return the actual frequency
uint32_t fll_init(const fll_t *fll);

// Lock the FLL if not locked and set the provide frequency in Hertz
// return the actual frequency
uint32_t fll_set_freq(const fll_t *fll, uint32_t frequency);

// return the actual frequency
uint32_t fll_get_freq(const fll_t *fll);

// FLL inline functions

static inline __attribute__((always_inline)) uint32_t fll_status_get(const fll_t *fll) {
  uint32_t status_reg = mmio_region_read32(fll->base_addr, (ptrdiff_t)(FLL_STATUS_REG_OFFSET));
  return bitfield_field32_read(status_reg, FLL_STATUS_MULT_FACTOR_FIELD);
}

static inline __attribute__((always_inline)) __attribute__((const)) uint32_t fll_create_config_1(const fll_conf1_reg_t configopts) {
    uint32_t conf_reg = 0;
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_1_CLK_MULT_FIELD, configopts.mult_factor);
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_1_DCO_CODE_STA_FIELD, configopts.dco_input);
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_1_CLK_DIV_FIELD, configopts.clk_div);
    conf_reg = bitfield_bit32_write(conf_reg, FLL_CONFIG_1_LOCK_ENABLE_BIT, configopts.lock_enable);
    conf_reg = bitfield_bit32_write(conf_reg, FLL_CONFIG_1_OP_MODE_BIT, configopts.op_mode);
    return conf_reg;
}

static inline __attribute__((always_inline)) void fll_conf1_set(const fll_t *fll, const uint32_t conf_reg) {
  mmio_region_write32(fll->base_addr, (ptrdiff_t)(FLL_CONFIG_1_REG_OFFSET), conf_reg);
}

static inline __attribute__((always_inline)) fll_conf1_reg_t fll_conf1_get(const fll_t *fll) {
  uint32_t conf_reg = mmio_region_read32(fll->base_addr, (ptrdiff_t)(FLL_CONFIG_1_REG_OFFSET));
  fll_conf1_reg_t configopts = {
    .mult_factor = bitfield_field32_read(conf_reg, FLL_CONFIG_1_CLK_MULT_FIELD),
    .dco_input   = bitfield_field32_read(conf_reg, FLL_CONFIG_1_DCO_CODE_STA_FIELD),
    .clk_div     = bitfield_field32_read(conf_reg, FLL_CONFIG_1_CLK_DIV_FIELD),
    .lock_enable = bitfield_bit32_read(conf_reg, FLL_CONFIG_1_LOCK_ENABLE_BIT),
    .op_mode     = bitfield_bit32_read(conf_reg, FLL_CONFIG_1_OP_MODE_BIT),
  };
  return configopts;
}

static inline __attribute__((always_inline)) __attribute__((const)) uint32_t fll_create_config_2(const fll_conf2_reg_t configopts) {
    uint32_t conf_reg = 0;
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_2_LOOP_GAIN_FIELD, configopts.loop_gain);
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_2_DEASSERT_CYCLES_FIELD, configopts.deassert_cycles);
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_2_ASSERT_CYCLES_FIELD, configopts.assert_cycles);
    conf_reg = bitfield_field32_write(conf_reg, FLL_CONFIG_2_LOCK_TOLERANCE_FIELD, configopts.lock_tolerance);
    conf_reg = bitfield_bit32_write(conf_reg, FLL_CONFIG_2_CLK_STA_MODE_BIT, configopts.clk_sta_mode);
    conf_reg = bitfield_bit32_write(conf_reg, FLL_CONFIG_2_OPEN_LOOP_ENABLE_BIT, configopts.open_loop_enable);
    conf_reg = bitfield_bit32_write(conf_reg, FLL_CONFIG_2_DITHERING_ENABLE_BIT, configopts.dithering_enable);
    return conf_reg;
}

static inline __attribute__((always_inline)) void fll_conf2_set(const fll_t *fll, const uint32_t conf_reg) {
  mmio_region_write32(fll->base_addr, (ptrdiff_t)(FLL_CONFIG_2_REG_OFFSET), conf_reg);
}

static inline __attribute__((always_inline)) fll_conf2_reg_t fll_conf2_get(const fll_t *fll) {
  uint32_t conf_reg = mmio_region_read32(fll->base_addr, (ptrdiff_t)(FLL_CONFIG_2_REG_OFFSET));
  fll_conf2_reg_t configopts = {
    .loop_gain        = bitfield_field32_read(conf_reg, FLL_CONFIG_2_LOOP_GAIN_FIELD),
    .deassert_cycles  = bitfield_field32_read(conf_reg, FLL_CONFIG_2_DEASSERT_CYCLES_FIELD),
    .assert_cycles    = bitfield_field32_read(conf_reg, FLL_CONFIG_2_ASSERT_CYCLES_FIELD),
    .lock_tolerance   = bitfield_field32_read(conf_reg, FLL_CONFIG_2_LOCK_TOLERANCE_FIELD),
    .clk_sta_mode     = bitfield_bit32_read(conf_reg, FLL_CONFIG_2_CLK_STA_MODE_BIT),
    .open_loop_enable = bitfield_bit32_read(conf_reg, FLL_CONFIG_2_OPEN_LOOP_ENABLE_BIT),
    .dithering_enable = bitfield_bit32_read(conf_reg, FLL_CONFIG_2_DITHERING_ENABLE_BIT),
  };
  return configopts;
}

static inline __attribute__((always_inline)) __attribute__((const)) uint32_t fll_create_integrator(const fll_integrator_reg_t configopts) {
    uint32_t conf_reg = 0;
    conf_reg = bitfield_field32_write(conf_reg, FLL_INTEGRATOR_FRAC_FIELD, configopts.frac_part);
    conf_reg = bitfield_field32_write(conf_reg, FLL_INTEGRATOR_INT_FIELD, configopts.int_part);
    return conf_reg;
}

static inline __attribute__((always_inline)) void fll_integrator_set(const fll_t *fll, const uint32_t integrator_reg) {
  mmio_region_write32(fll->base_addr, (ptrdiff_t)(FLL_INTEGRATOR_REG_OFFSET), integrator_reg);
}

static inline __attribute__((always_inline)) fll_integrator_reg_t fll_integrator_get(const fll_t *fll) {
  uint32_t conf_reg = mmio_region_read32(fll->base_addr, (ptrdiff_t)(FLL_INTEGRATOR_REG_OFFSET));
  fll_integrator_reg_t integrator_reg = {
    .unused    = 0,
    .frac_part = bitfield_field32_read(conf_reg, FLL_INTEGRATOR_FRAC_FIELD),
    .int_part  = bitfield_field32_read(conf_reg, FLL_INTEGRATOR_INT_FIELD),
  };
  return integrator_reg;
}

#ifdef __cplusplus
}
#endif

#endif // _DRIVERS_FLL_H_
