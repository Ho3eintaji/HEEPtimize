# Copyright 2022 OpenHW Group
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

set_host_options -max_cores 16

set SET_LIBS ${SCRIPT_DIR}/set_libs.tcl
set CONSTRAINTS ${SCRIPT_DIR}/set_constraints.tcl

remove_design -all

source ${SET_LIBS}

# todo: not needed because I do not want to skip any cell
# # Make sure no HVT cells are used
# set_attribute [get_lib_cells tcbn65lp*/*HVT] dont_use true
# # Use only power switches from the HVT lib
# set_attribute [get_lib_cells tcbn65lp*/FTR*HVT] dont_use false
# set_attribute [get_lib_cells tcbn65lp*/HDR*HVT] dont_use false

source ${READ_SOURCES}.tcl

elaborate ${TOP_MODULE}
link

write -f ddc -hierarchy -output ${REPORT_DIR}/precompiled.ddc

source ${CONSTRAINTS}

report_clocks > ${REPORT_DIR}/clocks.rpt
report_timing -loop -max_paths 10 > ${REPORT_DIR}/timing_loop.rpt

# Minimum number of bit required for clock gating and do not use Latches
set_clock_gating_style -minimum_bitwidth 3 -positive_edge_logic integrated:SC8T_CKGPRELATNX4_DDC36UH -control_point before

# remove_upf
# load_upf ../../../heepatia.synthesis.upf


# set_voltage 1.08 -object_list { VDD }
# set_voltage 0.00 -object_list { VSS }
# set_voltage 1.08 -object_list { VDD_CAESAR }
# set_voltage 1.08 -object_list { VDD_CARUS }

compile_ultra -no_autoungroup -no_boundary_optimization -timing -gate_clock

write -f ddc -hierarchy -output ${REPORT_DIR}/compiled.ddc

change_names -rules verilog -hier

write -format verilog -hier -o ${REPORT_DIR}/netlist.v

write_sdc -version 1.7 ${REPORT_DIR}/netlist.sdc
write_sdf -version 2.1 ${REPORT_DIR}/netlist.sdf

report_timing -nosplit > ${REPORT_DIR}/timing.rpt
report_area -hier -nosplit > ${REPORT_DIR}/area.rpt
report_resources -hierarchy > ${REPORT_DIR}/resources.rpt
report_constraints > ${REPORT_DIR}/constraints.rpt
report_clock_gating > ${REPORT_DIR}/clock_gating.rpt
report_power > ${REPORT_DIR}/power.rpt
report_timing -max_paths 10 -through u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/*  > ${REPORT_DIR}/timing_carus.rpt
report_timing -max_paths 10 -through u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper/u_caesar_top/* > ${REPORT_DIR}/timing_caesar.rpt
report_timing -max_paths 10 -through u_core_v_mini_mcu/external_subsystem_powergate_switch_ack_i*  >> ${REPORT_DIR}/timing_sw_cells.rpt
report_timing -max_paths 10 -through u_heepatia_peripherals/u_cgra_top_wrapper/*  >> ${REPORT_DIR}/timing_cgra.rpt

### save here also the report
set report_date [sh date +%Y_%m_%d_%k:%M]

file mkdir ${SCRIPT_DIR}/output_$report_date

write -f ddc -hierarchy -output ${SCRIPT_DIR}/output_$report_date/compiled.ddc

sh cp -R ${REPORT_DIR} ${SCRIPT_DIR}/output_$report_date

#this is where the backend tool gets all the files
sh rm -rf ${SCRIPT_DIR}/../../synthesis/last_output
file mkdir ${SCRIPT_DIR}/../../synthesis/last_output

sh cp -R ${REPORT_DIR}/synth.log ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/netlist.sdc ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/netlist.sdf ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/netlist.v ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/precompiled.ddc ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/compiled.ddc ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/timing.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/area.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/resources.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/timing_loop.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/clocks.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/timing_caesar.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/timing_carus.rpt ${SCRIPT_DIR}/../../synthesis/last_output
sh cp -R ${REPORT_DIR}/timing_cgra.rpt ${SCRIPT_DIR}/../../synthesis/last_output
