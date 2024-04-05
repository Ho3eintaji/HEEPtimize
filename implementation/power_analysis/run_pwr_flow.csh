#!/bin/tcsh

# prepare a working directory
set TIME_STAMP   = `date +"%Y%m%d_%H%M"`
pushd ../../; setenv FLOW_ROOT `pwd`; popd
#set BUILD_DIR=$FLOW_ROOT/implementation/power_analysis/build_${TIME_STAMP}
set PWR_DIR=$FLOW_ROOT/implementation/power_analysis/

#mkdir -p $BUILD_DIR

set MAX_CPUS = 64
set CPUS = `nproc`

if ( $CPUS > $MAX_CPUS ) then
    set CPUS = $MAX_CPUS
endif


# enter the build dir
pushd $PWR_DIR

pwr_shell -x "set FLOW_ROOT $FLOW_ROOT; set VCD_FILE $1; set NETLIST $2; set SDF_FILE $3; set TOP_MODULE $4" -file scripts/pwr_script.tcl -output_log_file pwr_shell_$4.log
#pwr_shell -x "set FLOW_ROOT $FLOW_ROOT; set VCD_FILE $1; set NETLIST $2; set SDF_FILE $3;" -output_log_file pwr_shell.log

popd
