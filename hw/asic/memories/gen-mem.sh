#!/bin/bash

######################################
# Generate the memories for HEEPatia #
######################################

# You can set where you want to generate the memories by running this
# script with the desired path as argument. In HEEPatia, this should be
#
# ./gen-mem.sh ../symlinks/ARM_Memories

# Absolute path
MEM_DIR=$(realpath "$(dirname $0)")

LOG_DIR=../../../build/memories/logs/
mkdir -p $LOG_DIR

# Desired memory sizes
NUM_ROWS="64 128 256 2048 4096 8192"
RF_MUX=2
SRAM_MUX=8

# Set make parameters
i=0
for n in $NUM_ROWS; do
    if [ $n -lt 2048 ]; then
        MUX=$RF_MUX
        TARGET=rf
    else
        MUX=$SRAM_MUX
        TARGET=sram
    fi

    # Run make with the appropriate parameters
    printf -- "Compiling ${TARGET} with ${n} rows... Check the log in ${LOG_DIR}\n"
    make -C $MEM_DIR -j2 NUM_ROWS=$n MUX=$MUX $TARGET > "${LOG_DIR}/${TARGET}${n}x32m${MUX}.log" 2>&1 &

    PIDS+=($!)
    i=$((i+1))
done

# Wait for all processes to finish and check for errors
ERR=0
for pid in "${PIDS[@]}"; do
    wait $pid
    [ $? -ne 0 ] && ERR=1
done

if [ $ERR -ne 0 ]; then
    printf -- "ERROR occurred! You can check the logs in ${LOG_DIR}\n" >&2
    exit 1
else
    printf -- "DONE! All memories compiled successfully\n"
    exit 0
fi
