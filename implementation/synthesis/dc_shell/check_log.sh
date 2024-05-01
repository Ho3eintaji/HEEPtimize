#!/bin/bash

# Copyright 2024 EPFL and Universidad Complutense de Madrid
#
# Author: David Mallasén
# Date: 25/04/2024
# Description: Check the synthesis log file for errors and warnings
#
# Usage: ./check_log.sh <log_file>
# Example: ./check_log.sh ./last_output/synth.log > errors_warnings.log

# Check if the user has provided a log file
if [ $# -ne 1 ]; then
    printf -- "Usage: ./check_log.sh <log_file>\n"
    exit 1
fi

# Check if the log file exists
if [ ! -f $1 ]; then
    printf -- "ERROR: Log file does not exist\n"
    exit 1
fi

# Print the errors and warnings found in the log file
# Do not print the ones the contain certain patterns

printf -- "Errors:\n"
grep -E "Error: " $1
printf -- "\n"

printf -- "====================================================================================\n"
printf -- "This script is waiving some warnings. Check the original logs to be completely sure!\n"
printf -- "====================================================================================\n"
printf -- "Warnings:\n"
grep -E "Warning: " $1 \
    | grep -v "Warning: .* Tool will ignore the request and use 1 cores. (UIO-231)" \
    | grep -v "Warning: .* The architecture arch has already been analyzed. It is being replaced. (VHD-4)" \
    | grep -v "Warning: .* Parameter keyword used in local parameter declaration. (VER-329)" \
    | grep -v "Warning: .* Invalid escape sequence .* (VER-941)" \
    | grep -v "Warning: .* DEFAULT branch of CASE statement cannot be reached. (ELAB-311)" \
    | grep -v "Warning: .* signed to unsigned conversion occurs. (VER-318)" \
    | grep -v "Warning: .* is connecting multiple ports. (UCN-1)" \