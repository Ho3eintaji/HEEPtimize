
set STD_CELLS_DIR ../../../hw/asic/std-cells
set MEMORIES_DIR ../../../hw/asic/symlinks/ARM_Memories/sram8192x32m8/db
set PADS_DIR     ../../../hw/asic/pads
# set FLL_DIR     ../../../hw/asic/fll/db
# set MEM_PWR_SW_DIR ../../../hw/asic/mem-power-switches/db

set NM_CAESAR_DIR ../../../implementation/synthesis/lc_shell/nm-caesar/db
set NM_CARUS_DIR ../../../implementation/synthesis/lc_shell/nm-carus/db

### LVT WC 1.08V
set DB_STDCELLS [glob -directory $STD_CELLS_DIR -- "*.db"]
### WC 1.08V
set DB_MEM [glob -directory $MEMORIES_DIR -- "*ss*125*.db"]
### WC 1.08V (IO 1.62V) - type of pads are random, please change it!!!
set DB_PAD [glob -directory $PADS_DIR -- "*.db"]

# set DB_FLL {}
# lappend DB_FLL "$FLL_DIR/tsmc65_FLL_ss_typical_max_1p08v_125c.db"

# set DB_MEM_PW_SW {}
# lappend DB_MEM_PW_SW "$MEM_PWR_SW_DIR/mem_power_switches.db"

set NM_CARUS {}
lappend NM_CARUS "$NM_CARUS_DIR/NMCarus8192x32m8_ss_1p08v_1p08v_125c.db"

set NM_CAESAR {}
lappend NM_CAESAR "$NM_CAESAR_DIR/NMCaesar8192x32m8_ss_1p08v_1p08v_125c.db"

# target library
set target_library      {}
# set target_library  "$DB_STDCELLS $DB_MEM $DB_PAD $DB_FLL $DB_MEM_PW_SW $NM_CAESAR $NM_CARUS"
set target_library  "$DB_STDCELLS $DB_MEM $DB_PAD"

# link library
set link_library "* $target_library"

#debug output info
puts "------------------------------------------------------------------"
puts "USED LIBRARIES"
puts $link_library
puts "------------------------------------------------------------------"

set link_library " * $link_library"
