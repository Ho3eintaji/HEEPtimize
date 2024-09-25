# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

set PERFORM_STA 0
set PERFORM_POWER 1

# initial setup
set power_enable_timing_analysis true
set power_enable_analysis true
# averaged (default) or time_based
set power_analysis_mode time_based 

set design(FLOW_ROOT) $::env(FLOW_ROOT)

# should be passed from the Makefile
echo "NETLIST FILE: " $NETLIST
echo "TOP_MODULE: " $TOP_MODULE
echo "VCD FILE: " $VCD_FILE
echo "SDF FILE: " $SDF_FILE
echo "PWR ANALYSIS MODE: " $PWR_ANALYSIS_MODE
set ANALYSIS_MODE $PWR_ANALYSIS_MODE

if {$TOP_MODULE == "heepatia_top"} {
    set STRIP_PATH tb_top/u_tb_system/u_heepatia_top/
} elseif {$TOP_MODULE == "carus_top"} {
    set STRIP_PATH tb_top/u_tb_system/u_heepatia_top/u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/
} elseif {$TOP_MODULE == "caesar_top"} {
    set STRIP_PATH tb_top/u_tb_system/u_heepatia_top/u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/
}

# source init
source ../common/primetime/init.tcl

set CONSTRAINTS $design(FLOW_ROOT)/implementation/synthesis/dc_shell/set_constraints.tcl
# set CONSTRAINTS  $design(FLOW_ROOT)/implementation/synthesis/last_output/netlist.sdc
source ${CONSTRAINTS}


# # clock tree synthesis
# set_propagated_clock [all_clocks]

# # read timings
# read_sdf -load_delay cell $SDF_FILE
# report_annotated_delay > $REPORTS_PATH/annotated_delay.rpt

# Timing analysis
if { $PERFORM_STA == 1 } {
    puts "Performing STA\n"

    # update timing
    update_timing -full

    # ensure design is properly constrained
    check_timing -verbose > $REPORTS_PATH/check_timing.rpt

    # report_timing section
    report_timing -slack_lesser_than 0.0 -delay min -nosplit -input -net -sign 4 -max_paths 50 > $REPORTS_PATH/timing_min.rpt
    report_timing -slack_lesser_than 0.0 -delay max -nosplit -input -net -sign 4 -max_paths 50 > $REPORTS_PATH/timing_max.rpt
    report_timing > $REPORTS_PATH/timing_cmplt.rpt
    report_clock -skew -attribute > $REPORTS_PATH/clock.rpt
    report_analysis_coverage -status_details {untested} > $REPORTS_PATH/analysis_coverage.rpt
    # report_disable_timing > $REPORTS_PATH/disable_timing.rpt
}

# Power analysis
if { $PERFORM_POWER == 1 } {
    puts "Performing Power Analysis\n"

    # read sw activity
    read_vcd ${VCD_FILE} -strip_path $STRIP_PATH

    # run power analysis
    update_power

    # report power
    report_power -nosplit -verbose > ${REPORTS_PATH}/${TOP_MODULE}_power.rpt
    report_power -nosplit -hier -verbose > ${REPORTS_PATH}/${TOP_MODULE}_hier.rpt
    report_power -nosplit -cell_power -leaf -verbose > ${REPORTS_PATH}/${TOP_MODULE}_leaf.rpt


   # Generate CSV power report
    # -------------------------
    source "scripts/gen_pwr_csv.tcl"

    # Initialize report file
    set REPORT_FP [open ${REPORTS_PATH}/power.csv "w"]
    puts $REPORT_FP "CELL,INTERNAL_POWER,SWITCHING_POWER,LEAKAGE_POWER,TOTAL_POWER,RELATIVE_POWER"

    # Set scopes to analyse (full hierarchy report already generated with report_power above)
    # set SCOPES "/ /u_core_v_mini_mcu /u_core_v_mini_mcu/ao_peripheral_subsystem_i /u_core_v_mini_mcu/peripheral_subsystem_i /u_core_v_mini_mcu/memory_subsystem_i /u_heepatia_bus /u_heepatia_peripherals /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_carus_ctl /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vector_pipeline /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf"
    # set SCOPE_NAMES "top core_v_mini_mcu ao_peripheral_subsystem peripheral_subsystem memory_subsystem heepatia_bus nmc_peripherals carus_ctl carus_vector carus_vrf"

    # set SCOPES "/ /u_core_v_mini_mcu /u_core_v_mini_mcu/ao_peripheral_subsystem_i /u_core_v_mini_mcu/peripheral_subsystem_i /u_core_v_mini_mcu/memory_subsystem_i /u_heepatia_bus /u_heepatia_peripherals /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_carus_ctl /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vector_pipeline /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf /u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/IMCaesar_ctrl_Xi /u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/alu_Xi /u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/MEM0 /u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/MEM1"
    # set SCOPE_NAMES "top core_v_mini_mcu ao_peripheral_subsystem peripheral_subsystem memory_subsystem heepatia_bus nmc_peripherals carus_ctl carus_vector carus_vrf caesar_ctl caesar_alu caesar_mem0 caesar_mem1"
    # tod: two lines above i am not sure 

    set SCOPES "/ /u_core_v_mini_mcu /u_core_v_mini_mcu/ao_peripheral_subsystem_i /u_core_v_mini_mcu/peripheral_subsystem_i /u_core_v_mini_mcu/memory_subsystem_i /u_heepatia_bus /u_heepatia_peripherals /u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top /u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top /u_heepatia_peripherals/u_cgra_top_wrapper"
    set SCOPE_NAMES "top core_v_mini_mcu ao_peripheral_subsystem peripheral_subsystem memory_subsystem heepatia_bus accel_peripherals carus_top caesar_top cgra_top"

    # Dump report for each scope
    foreach scope $SCOPES name $SCOPE_NAMES {
        set_scope $scope
        puts "Generating power report $name for scope $scope"
        gen_pwr_csv $REPORT_FP $name
    }

    # Close report file
    close ${REPORT_FP}
}


exit
