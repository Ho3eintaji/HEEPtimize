#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "rv_plic.h"
#include "rv_plic_regs.h"
#include "heepocrates.h"
#include "fll.h"
#include "soc_ctrl.h"
#include "x-heep.h"

// Choose what to test
#define FLL_DEFAULT_VAL_TEST
#define FLL_OPEN_LOOP_TEST
#define FLL_NORMAL_MODE_TEST

uint32_t fll_init(const fll_t *fll);
static uint32_t fll_get_freq_from_mult_div(uint32_t mult_factor, uint32_t clk_div);

int main(void) {

  uint32_t fll_freq, fll_freq_real;

  // FLL peripheral structure to access the registers
  fll_t fll;
  fll.base_addr = mmio_region_from_addr((uintptr_t)FLL_START_ADDRESS);

  soc_ctrl_t soc_ctrl;
  soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);

  uint32_t fll_status = fll_status_get(&fll);
  fll_conf1_reg_t fll_conf1 = fll_conf1_get(&fll);
  fll_conf2_reg_t fll_conf2 = fll_conf2_get(&fll);
  fll_integrator_reg_t fll_integrator = fll_integrator_get(&fll);

  fll_freq = fll_get_freq(&fll);
  printf("fll_status : %08x\n", fll_status);
  printf("fll_freq Hz: %d\n", fll_freq);

#ifdef FLL_DEFAULT_VAL_TEST
  if (fll_conf1.mult_factor      != 6103  || // reset value 0xC35 is changed by external_crt0.S init to 100 MHz
      fll_conf1.dco_input        != 395   || // reset value 0x158 is changed by external_crt0.S init to 100 MHz
      fll_conf1.clk_div          != 0x002 ||
      fll_conf1.lock_enable      != 0x001 ||
      fll_conf1.op_mode          != 0x001 || // reset value 0x000 is changed by external_crt0.S init to 100 MHz
      fll_conf2.loop_gain        != 0x008 ||
      fll_conf2.deassert_cycles  != 0x010 ||
      fll_conf2.assert_cycles    != 0x006 || // reset value 0x010 is changed by external_crt0.S init to 100 MHz
      fll_conf2.lock_tolerance   != 0x050 || // reset value 0x200 is changed by external_crt0.S init to 100 MHz
      fll_conf2.clk_sta_mode     != 0x000 ||
      fll_conf2.open_loop_enable != 0x000 ||
      fll_conf2.dithering_enable != 0x000 ||
      fll_integrator.frac_part   != 0x058 || // reset value 0x000 is changed by external_crt0.S init to 100 MHz
      fll_integrator.int_part    != 0x189) { // reset value 0x158 is changed by external_crt0.S init to 100 MHz
    
    printf("FLL default values incorrect!\n");
      #ifdef PRINT_FLL_DEFAULT_VAL_TEST
        printf("================ FLL DEFAULT/INIT VALUES CHECK ================\n");
        printf("fll_conf1.mult_factor (initval=0x17d7): %08x\n", fll_conf1.mult_factor);
        printf("fll_conf1.dco_input (initval=0x18b): %08x\n", fll_conf1.dco_input);
        printf("fll_conf1.clk_div (resval=0x002): %08x\n", fll_conf1.clk_div);
        printf("fll_conf1.lock_enable (resval=0x001): %08x\n", fll_conf1.lock_enable);
        printf("fll_conf1.op_mode (initval=0x001): %08x\n", fll_conf1.op_mode);
        printf("fll_conf2.loop_gain (resval=0x008): %08x\n", fll_conf2.loop_gain);
        printf("fll_conf2.deassert_cycles (resval=0x010): %08x\n", fll_conf2.deassert_cycles);
        printf("fll_conf2.assert_cycles (initval=0x006): %08x\n", fll_conf2.assert_cycles);
        printf("fll_conf2.lock_tolerance (initval=0x050): %08x\n", fll_conf2.lock_tolerance);
        printf("fll_conf2.clk_sta_mode (resval=0x000): %08x\n", fll_conf2.clk_sta_mode);
        printf("fll_conf2.open_loop_enable (resval=0x000): %08x\n", fll_conf2.open_loop_enable);
        printf("fll_conf2.dithering_enable (resval=0x000): %08x\n", fll_conf2.dithering_enable);
        printf("fll_integrator.frac_part (initval=0x058): %08x\n", fll_integrator.frac_part);
        printf("fll_integrator.int_part (initval=0x189): %08x\n", fll_integrator.int_part);
      #endif
  } else {
    printf("FLL default/init values correct!\n");
  }
#endif

#ifdef FLL_OPEN_LOOP_TEST
  printf("================ FLL OPEN LOOP / STANDALONE MODE CHECK ================\n");
  // FLL Open loop FLL DCO code transfer function (in typical case)
  // !!! FLL Documentation inverts the formulas for {272:360} and {360:31023} !!!
  // dco_input < 272  : freq = 0.000001
  // dco_input < 360  : freq =  1068.966 - 8.626*dco_input + 0.017612*(dco_input**2.0)
  // dco_input >= 360 : freq = -1586.080 + 5.518*dco_input + -0.001191*(dco_input**2.0)
  // FLL Standalone mode (open loop) DCO input code ramp-down

  uint32_t dco_input_default = 0x158; // dco input:0x158 (344) ~= 50MHz

  // Enable open loop mode
  // fll_freq = fll_set_freq(&fll, 50000000);
  fll_conf1.op_mode = 0;
  fll_conf1_set(&fll, fll_conf1.raw);
  fll_conf1.dco_input = dco_input_default;
  fll_conf1_set(&fll, fll_conf1.raw);
  // Small delay to let the FLL settle
  for (int j = 0; j < 1000; j++) {
    asm volatile("nop");
  }
  fll_freq_real = fll_get_freq(&fll);
  soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);
  printf("OPEN LOOP: fll_freq real Hz = %d\n", fll_freq_real);

  // DCO input code is filtered so the value is incremented by 10 to see a change in frequency
  for (int i = 0; i < (dco_input_default-272-4); i+=10)
  {
    const uint32_t config1 = fll_create_config_1((fll_conf1_reg_t){
      .mult_factor = fll_conf1.mult_factor,
      .dco_input   = dco_input_default-i,
      .clk_div     = fll_conf1.clk_div,
      .lock_enable = fll_conf1.lock_enable,
      .op_mode     = fll_conf1.op_mode
    });
    fll_conf1_set(&fll, config1);
    fll_status = fll_status_get(&fll);
    // Small delay to let the FLL settle
    for (int j = 0; j < 1000; j++) {
      asm volatile("nop");
    }
    // Update frequency in SoC controller otherwise devices using this value (e.g., uart) will not work
    fll_freq = fll_get_freq(&fll);
    soc_ctrl_set_frequency(&soc_ctrl, fll_freq);
    printf("OPEN LOOP: fll DCO input = %d\n", dco_input_default-i);
    printf("OPEN LOOP: fll_freq Hz   = %d\n", fll_freq);
  }
  printf("FLL open loop mode working if you can read this and FLL frequency changed!\n");
#endif // FLL_OPEN_LOOP_TEST


#ifdef FLL_NORMAL_MODE_TEST
  printf("================ FLL NORMAL/LOCK MODE CHECK ================\n");

  fll_freq = fll_init(&fll);

  fll_freq_real = fll_get_freq(&fll);

  soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);

  printf("NORMAL MODE: fll_freq request Hz = %d\n", fll_freq);
  printf("NORMAL MODE: fll_freq real Hz    = %d\n", fll_freq_real);

  // Set FLL to 250 MHz
  fll_freq = fll_set_freq(&fll, 250000000);

  fll_freq_real = fll_get_freq(&fll);

  soc_ctrl_set_frequency(&soc_ctrl, fll_freq_real);

  printf("NORMAL MODE: fll_freq request Hz = %d\n", fll_freq);
  printf("NORMAL MODE: fll_freq real Hz    = %d\n", fll_freq_real);

  printf("FLL normal mode working if you can read this and FLL frequency changed!\n");

#endif // FLL_NORMAL_MODE_TEST

  return EXIT_SUCCESS;
}
