#!/bin/bash

#######################################################################
# Single-port SRAM memories for the main memories bank of HEEPocrates #
#######################################################################

# Absolute path
MEM_DIR=$(realpath "$(dirname $0)")

# Desired memory sizes
NUM_ROWS="64 128 256 2048 4096"
RF_MUX=2
SRAM_MUX=8

# Set make parameters
i=0
for n in $NUM_ROWS; do
    if [ $n -lt 2048 ]; then
        printf -- "Compiling register file with %u rows...\n" $n
        MUX=$RF_MUX
        TARGET=rf
    else
        printf -- "Compiling SRAM with %u rows...\n" $n
        MUX=$SRAM_MUX
        TARGET=sram
    fi
    make -C $MEM_DIR -j2 NUM_ROWS=$n MUX=$MUX $TARGET &
    PIDS+=($!)
    i=$((i+1))
done

ERR=0
for pid in "${PIDS[@]}"; do
    wait $pid
    [ $? -ne 0 ] && ERR=1
done

if [ $ERR -ne 0 ]; then
    printf -- "ERROR occurred!\n" >&2
    exit 1
fi

printf -- "ALL DONE!\n"
exit 0

