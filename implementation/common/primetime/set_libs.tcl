# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

# Allowed ANALYSIS_MODE values
# ANALYSIS_MODE can be one of the following:
# tt_0p50_25  -> Typical at 0.50V, 25C
# tt_0p65_25  -> Typical at 0.65V, 25C
# tt_0p80_25  -> Typical at 0.80V, 25C
# tt_0p90_25  -> Typical at 0.90V, 25C


# Standard cell libraries
set lib_std(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db/GF22FDX_SC8T_104CPP_BASE_CSC28R_TT_0P50V_0P00V_0P00V_0P00V_25C.db
set lib_std(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db/GF22FDX_SC8T_104CPP_BASE_CSC28R_TT_0P65V_0P00V_0P00V_0P00V_25C.db
set lib_std(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db/GF22FDX_SC8T_104CPP_BASE_CSC28R_TT_0P80V_0P00V_0P00V_0P00V_25C.db
set lib_std(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db/GF22FDX_SC8T_104CPP_BASE_CSC28R_TT_0P90V_0P00V_0P00V_0P00V_25C.db
set lib_std(wc)         $design(FLOW_ROOT)/hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db/GF22FDX_SC8T_104CPP_BASE_CSC28R_SSG_0P81V_0P00V_0P00V_0P00V_125C.db

# FLL libraries
# set lib_fll $design(FLOW_ROOT)/hw/asic/fll/db/tsmc65_FLL_ss_typical_max_1p08v_125c.db

# Memory libraries
set lib_mem(tt_0p50_25) ""
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_128x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram128x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_256x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram256x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_512x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram512x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_1024x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram1024x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_2048x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram2048x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_4096x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram4096x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_8192x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram8192x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p50_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_16384x32_LPP_M8_TT_0_650_0_800_25/model/timing/db/sram16384x32m8_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db


set lib_mem(tt_0p65_25) ""
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_128x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram128x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_256x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram256x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_512x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram512x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_1024x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram1024x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_2048x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram2048x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_4096x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram4096x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_8192x32_LPP_M4_TT_0_650_0_800_25/model/timing/db/sram8192x32m4_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p65_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_16384x32_LPP_M8_TT_0_650_0_800_25/model/timing/db/sram16384x32m8_116cpp_TT_0P650V_0P800V_0P000V_0P000V_025C.db

set lib_mem(tt_0p80_25) ""
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_128x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram128x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_256x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram256x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_512x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram512x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_1024x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram1024x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_2048x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram2048x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_4096x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram4096x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_8192x32_LPP_M4_TT_0_800_0_800_25/model/timing/db/sram8192x32m4_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p80_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_16384x32_LPP_M8_TT_0_800_0_800_25/model/timing/db/sram16384x32m8_116cpp_TT_0P800V_0P800V_0P000V_0P000V_025C.db

set lib_mem(tt_0p90_25) ""
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_128x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram128x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_256x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram256x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_512x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram512x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_1024x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram1024x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_2048x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram2048x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_4096x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram4096x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_8192x32_LPP_M4_TT_0_900_0_900_25/model/timing/db/sram8192x32m4_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db
lappend lib_mem(tt_0p90_25) $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_16384x32_LPP_M8_TT_0_900_0_900_25/model/timing/db/sram16384x32m8_116cpp_TT_0P900V_0P900V_0P000V_0P000V_025C.db

set lib_mem(wc) ""
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_128x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram128x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_256x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram256x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_512x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram512x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_1024x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram1024x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_2048x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram2048x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_4096x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram4096x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_8192x32_LPP_M4_SSG_0_810_0_810_125/model/timing/db/sram8192x32m4_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db
lappend lib_mem(wc)         $design(FLOW_ROOT)/hw/asic/std-cells-memories/memories/MemViews_6T_16384x32_LPP_M8_SSG_0_810_0_810_125/model/timing/db/sram16384x32m8_116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db

# Combine memory libraries
set lib_mem($ANALYSIS_MODE) [join $lib_mem($ANALYSIS_MODE) " "]

# Target library
# set target_library "$lib_std($ANALYSIS_MODE) $lib_fll $lib_mem($ANALYSIS_MODE)"
set target_library "$lib_std($ANALYSIS_MODE) $lib_mem($ANALYSIS_MODE)"

# Link library
set link_library ""
set link_library "* $target_library"

# Debug output info
puts "------------------------------------------------------------------"
puts "USED LIBRARIES"
puts $link_library
puts "------------------------------------------------------------------"

set link_library "* $link_library"
