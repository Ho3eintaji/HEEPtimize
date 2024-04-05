# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0


set LIB_PATH $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Back_End/lef/tcbn65lplvt_200a/

set cap_tbl(cbest)   $LIB_PATH/techfiles/captable/cln65lp_1p09m+alrdl_top2_cbest.captable
set cap_tbl(cworst)  $LIB_PATH/techfiles/captable/cln65lp_1p09m+alrdl_top2_cworst.captable
set cap_tbl(rcbest)  $LIB_PATH/techfiles/captable/cln65lp_1p09m+alrdl_top2_rcbest.captable
set cap_tbl(rcworst) $LIB_PATH/techfiles/captable/cln65lp_1p09m+alrdl_top2_rcworst.captable
set cap_tbl(typical) $LIB_PATH/techfiles/captable/cln65lp_1p09m+alrdl_top2_typical.captable

set cap_tables {cbest cworst rcbest rcworst typical}

set lib_std(wc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtwc0d9_ccs.db
set lib_std(bc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtbc_ccs.db
set lib_std(tc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvttc_ccs.db
set lib_std(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtwcl0d9_ccs.db
set lib_std(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtml_ccs.db
set lib_std(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtlt1d1_ccs.db

#For Caesar and Carus
set lib_std_rvt(wc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lpwc_ccs.db
set lib_std_rvt(bc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lpbc_ccs.db
set lib_std_rvt(tc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lptc_ccs.db
set lib_std_rvt(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lpwcl_ccs.db
set lib_std_rvt(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lpml_ccs.db
set lib_std_rvt(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lp_200a/tcbn65lpbc_ccs.db

#For Carus
set lib_std_hvt(wc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvtwc0d9_ccs.db
set lib_std_hvt(bc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvtbc_ccs.db
set lib_std_hvt(tc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvttc_ccs.db
set lib_std_hvt(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvtwcl0d9_ccs.db
set lib_std_hvt(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvtml_ccs.db
set lib_std_hvt(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lphvt_200a/tcbn65lphvtlt1d1_ccs.db

#dual-rail std cells

set lib_std_dualrail(wc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtwc0d90d9_ccs.db
set lib_std_dualrail(bc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvtbc1d321d32_ccs.db
set lib_std_dualrail(tc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lplvt_200a/tcbn65lplvttc1d21d2_ccs.db
# don't have these, but Davide has to check
set lib_std_dualrail(bc_m40) $lib_std_dualrail(bc)
set lib_std_dualrail(wc_m40) $lib_std_dualrail(wc)
set lib_std_dualrail(bc_125) $lib_std_dualrail(bc)


#coarse-grain cells (power switches)
set lib_cgcells(wc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lpcghvt_200a/tcbn65lpcghvtwc0d9_ccs.db
set lib_cgcells(bc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lpcghvt_200a/tcbn65lpcghvtbc_ccs.db
set lib_cgcells(tc) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Front_End/timing_power_noise/CCS/tcbn65lpcghvt_200a/tcbn65lpcghvttc_ccs.db
# don't have these, but Davide has to check
set lib_cgcells(bc_m40) $lib_cgcells(bc)
set lib_cgcells(wc_m40) $lib_cgcells(wc)
set lib_cgcells(bc_125) $lib_cgcells(bc)

# custom power switches
set lib_pswitch(wc) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db
set lib_pswitch(bc) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db
set lib_pswitch(tc) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db
set lib_pswitch(bc_m40) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db
set lib_pswitch(wc_m40) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db
set lib_pswitch(bc_125) $design(FLOW_ROOT)/hw/asic/mem-power-switches/db/mem_power_switches.db

set lib_fll(wc) $design(FLOW_ROOT)/hw/asic/fll/db/tsmc65_FLL_ss_typical_max_1p08v_125c.db
set lib_fll(bc) $design(FLOW_ROOT)/hw/asic/fll/db/tsmc65_FLL_ff_typical_min_1p32v_m40c.db
set lib_fll(tc) $design(FLOW_ROOT)/hw/asic/fll/db/tsmc65_FLL_tt_typical_max_1p20v_25c.db
# don't have these, lets use the bc/wc we have...
set lib_fll(bc_m40) $lib_fll(bc)
set lib_fll(wc_m40) $lib_fll(wc)
set lib_fll(bc_125) $lib_fll(bc)


set lib_io(wc) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Front_End/timing_power_noise/NLDM/tpdn65lpnv2od3_200a/tpdn65lpnv2od3wc.db
set lib_io(bc) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Front_End/timing_power_noise/NLDM/tpdn65lpnv2od3_200a/tpdn65lpnv2od3bc.db
set lib_io(tc) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Front_End/timing_power_noise/NLDM/tpdn65lpnv2od3_200a/tpdn65lpnv2od3tc.db
# don't have these, lets use the bc/wc we have...
set lib_io(bc_m40) $lib_io(bc)
set lib_io(wc_m40) $lib_io(wc)
set lib_io(bc_125) $lib_io(bc)


set lib_mem(wc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_ss_1p08v_1p08v_125c.db
set lib_mem(bc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_ff_1p32v_1p32v_m40c.db
set lib_mem(tc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_tt_1p20v_1p20v_25c.db
set lib_mem(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_ff_1p32v_1p32v_m40c.db
set lib_mem(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_ss_1p08v_1p08v_m40c.db
set lib_mem(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/db/sram8192x32m8_ff_1p32v_1p32v_125c.db

# LIBs specific to CAESAR
set lib_mem_caesar(wc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_ss_1p08v_1p08v_125c.db
set lib_mem_caesar(bc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_ff_1p32v_1p32v_m40c.db
set lib_mem_caesar(tc) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_tt_1p20v_1p20v_25c.db
set lib_mem_caesar(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_ss_1p08v_1p08v_m40c.db
set lib_mem_caesar(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_ff_1p32v_1p32v_125c.db
set lib_mem_caesar(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram4096x32m8/db/sram4096x32m8_ff_1p32v_1p32v_m40c.db


# LIBs specific to Carus
set lib_emem(wc)        $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_ss_1p08v_1p08v_125c.db
set lib_emem(bc)        $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_ff_1p32v_1p32v_m40c.db
set lib_emem(tc)        $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_tt_1p20v_1p20v_25c.db
set lib_emem(bc_m40)    $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_ff_1p32v_1p32v_m40c.db
set lib_emem(wc_m40)    $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_ss_1p08v_1p08v_m40c.db
set lib_emem(bc_125)    $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/db/rf128x32m2_ff_1p32v_1p32v_125c.db

set lib_vrf(wc)     $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_ss_1p08v_1p08v_125c.db
set lib_vrf(bc)     $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_ff_1p32v_1p32v_m40c.db
set lib_vrf(tc)     $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_tt_1p20v_1p20v_25c.db
set lib_vrf(bc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_ff_1p32v_1p32v_m40c.db
set lib_vrf(wc_m40) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_ss_1p08v_1p08v_m40c.db
set lib_vrf(bc_125) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram2048x32m8/db/sram2048x32m8_ff_1p32v_1p32v_125c.db


# target library
set target_library "$lib_std($ANALYSIS_MODE) $lib_std_rvt($ANALYSIS_MODE) $lib_std_hvt($ANALYSIS_MODE) $lib_std_dualrail($ANALYSIS_MODE) $lib_cgcells($ANALYSIS_MODE) $lib_pswitch($ANALYSIS_MODE) $lib_fll($ANALYSIS_MODE) $lib_io($ANALYSIS_MODE) $lib_mem($ANALYSIS_MODE) $lib_mem_caesar($ANALYSIS_MODE) $lib_emem($ANALYSIS_MODE) $lib_vrf($ANALYSIS_MODE)"

# link library
set link_library ""
set link_library "* $target_library"

# debug output info
puts "------------------------------------------------------------------"
puts "USED LIBRARIES"
puts $link_library
puts "------------------------------------------------------------------"

set link_library "* $link_library"
