#!/bin/bash


# Log file
LOG_FILE="simulation.log"
echo "Simulation started at $(date)" > "$LOG_FILE"

# # Step 1: Clean, generate, and build once
# echo "Running make clean..." | tee -a "$LOG_FILE"
# make clean >> "$LOG_FILE" 2>&1
# if [ $? -ne 0 ]; then
#     echo "Error: Failed to clean. Check the log for details." | tee -a "$LOG_FILE"
#     exit 1
# fi

# echo "Running make heepatia-gen-force..." | tee -a "$LOG_FILE"
# make heepatia-gen-force >> "$LOG_FILE" 2>&1
# if [ $? -ne 0 ]; then
#     echo "Error: Failed to generate force files. Check the log for details." | tee -a "$LOG_FILE"
#     exit 1
# fi

echo "Running make questasim-postsynth-build..." | tee -a "$LOG_FILE"
make questasim-postsynth-build BOOT_MODE=flash VCD_MODE=2 LINKER=flash_load TB_SYSCLK=10ns >> "$LOG_FILE" 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Failed to build the project for QuestaSim. Check the log for details." | tee -a "$LOG_FILE"
    exit 1 
fi

# Step 2: Loop through the different KERNEL_PARAMS values
for col_b in 8 16 32 64 128 256; do
    echo "Starting simulation with --col_b=$col_b..." | tee -a "$LOG_FILE"

    # Step 2a: Build the application with the current KERNEL_PARAMS
    KERNEL_PARAMS="int32 --row_a 8 --col_a 8 --col_b $col_b"
    echo "Running make app with KERNEL_PARAMS=$KERNEL_PARAMS..." | tee -a "$LOG_FILE"
    make app PROJECT=accels-matmul LINKER=flash_load BOOT_MODE=flash VCD_MODE=2 KERNEL_PARAMS="$KERNEL_PARAMS" >> "$LOG_FILE" 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build the application with KERNEL_PARAMS=$KERNEL_PARAMS. Check the log for details." | tee -a "$LOG_FILE"
        exit 1
    fi

    # Step 2b: Run the simulation
    echo "Running make questasim-postsynth-run..." | tee -a "$LOG_FILE"
    make questasim-postsynth-run BOOT_MODE=flash VCD_MODE=2 LINKER=flash_load >> "$LOG_FILE" 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Simulation failed for KERNEL_PARAMS=$KERNEL_PARAMS. Check the log for details." | tee -a "$LOG_FILE"
        exit 1
    fi

    # Step 2c: Store simulation results
    SIM_RESULTS_DIR="build/epfl_heepatia_heepatia_0.3.0/sim_postsynthesis-modelsim/logs"
    DEST_DIR="private/matmul_postsynth_sims/col_b_$col_b"
    
    echo "Copying simulation results from $SIM_RESULTS_DIR to $DEST_DIR..." | tee -a "$LOG_FILE"
    mkdir -p "$DEST_DIR" >> "$LOG_FILE" 2>&1
    cp -r "$SIM_RESULTS_DIR"/* "$DEST_DIR/" >> "$LOG_FILE" 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy simulation results for col_b=$col_b. Check the log for details." | tee -a "$LOG_FILE"
        exit 1
    fi
    
    echo "Simulation with --col_b=$col_b completed successfully." | tee -a "$LOG_FILE"
done

echo "All simulations completed successfully at $(date)." | tee -a "$LOG_FILE"
