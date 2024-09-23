#!/bin/bash

# prepare a working directory
export TIME_STAMP=$(date +"%Y%m%d_%H%M")
export FLOW_ROOT=$(realpath ../..)
export PWR_DIR=$FLOW_ROOT/implementation/power_analysis/

# Set the number of CPUs to use
MAX_CPUS=64
export CPUS=$(nproc)
if [ $CPUS -gt $MAX_CPUS ]; then
    export CPUS=$MAX_CPUS
fi

# enter the build dir
pushd $PWR_DIR
pwr_shell -x "set FLOW_ROOT $FLOW_ROOT; set VCD_FILE $FLOW_ROOT/$1; set NETLIST $2; set SDF_FILE $3; set TOP_MODULE $4; set PWR_ANALYSIS_MODE $5" -file scripts/pwr_script.tcl -output_log_file pwr_shell_$4.log
popd
