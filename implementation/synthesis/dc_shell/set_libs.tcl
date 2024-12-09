# Custom procedure to recursively find .db files
proc find_files {dir pattern} {
    set results {}
    foreach file [glob -nocomplain -directory $dir *] {
        if {[file isdirectory $file]} {
            lappend results {*}[find_files $file $pattern]
        } elseif {[string match $pattern [file tail $file]]} {
            lappend results $file
        }
    }
    return $results
}

# set std cells
set STD_CELLS_DIR ../../../hw/asic/std-cells/GF22FDX_SC8T_104CPP_BASE_CSC28R_FDK_RELV05R50/model/timing/db
set DB_STDCELLS [glob -directory $STD_CELLS_DIR -- "*SSG_0P81V_0P00V_0P00V_0P00V_125C.db"] 
puts "------------------------------------------------------------------"
puts "USED STDCELLS"
puts $DB_STDCELLS
puts "------------------------------------------------------------------"


set MEMORIES_DIR ../../../hw/asic/std-cells-memories/compiled_memories
# Find all .db files recursively
set DB_MEM [find_files $MEMORIES_DIR "*116cpp_SSG_0P810V_0P810V_0P000V_0P000V_125C.db"] 

# Check if any files were found
if {[llength $DB_MEM] == 0} {
    echo "No .db files found in the specified directory."
} else {
    echo "Found the following .db files:"
    foreach file $DB_MEM {
        echo $file
    }
}

# set PADS_DIR     ../../../hw/asic/pads
# set DB_PAD [glob -directory $PADS_DIR -- "*.db"]

# set FLL_DIR     ../../../hw/asic/fll/db
# set DB_FLL {}
# lappend DB_FLL "$FLL_DIR/tsmc65_FLL_ss_typical_max_1p08v_125c.db"

# set MEM_PWR_SW_DIR ../../../hw/asic/mem-power-switches/db
# set DB_MEM_PW_SW {}
# lappend DB_MEM_PW_SW "$MEM_PWR_SW_DIR/mem_power_switches.db"

# target library
set target_library      {}
# set target_library  "$DB_STDCELLS $DB_MEM $DB_PAD $DB_FLL $DB_MEM_PW_SW $NM_CAESAR $NM_CARUS"
# set target_library  "$DB_STDCELLS $DB_MEM $DB_FLL"
set target_library  "$DB_STDCELLS $DB_MEM"

# link library
set link_library "* $target_library"

#debug output info
puts "------------------------------------------------------------------"
puts "USED LIBRARIES"
puts $link_library
puts "------------------------------------------------------------------"

set link_library " * $link_library"
