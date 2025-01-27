#!/bin/bash

# Log file
LOG_FILE="simulation.log"
echo "Simulation started at $(date)" > "$LOG_FILE"

# Create 'matmul' directory if it doesn't exist
MATMUL_DIR="sims"
mkdir -p "$MATMUL_DIR"

# Mapping of VCD indices to custom labels
# declare -A VCD_LABELS=( [0]="carus" [1]="caesar" [2]="cgra" [3]="cpu" ) # fix it
declare -A VCD_LABELS=( [0]="cpu")

# Function to display usage information
usage() {
    echo "Usage: $0 --row_a 'values' --col_a 'values' --col_b 'values'"
    echo "Example: $0 --row_a '8 16' --col_a '8' --col_b '256 128'"
    exit 1
}

            echo "Starting simulation with --row_a=$row_a, --col_a=$col_a, --col_b=$col_b..." | tee -a "$LOG_FILE"

            # Build the application with the current KERNEL_PARAMS
            KERNEL_PARAMS="int32 --row_a $row_a --col_a $col_a --col_b $col_b"
            echo "Running make app with KERNEL_PARAMS=$KERNEL_PARAMS..." | tee -a "$LOG_FILE"
            make app PROJECT=accels-matmul LINKER=flash_load BOOT_MODE=flash VCD_MODE=2 KERNEL_PARAMS="$KERNEL_PARAMS" >> "$LOG_FILE" 2>&1
            if [ $? -ne 0 ]; then
                echo "Error: Failed to build the application with KERNEL_PARAMS=$KERNEL_PARAMS. Check the log for details." | tee -a "$LOG_FILE"
                exit 1
            fi

            # Run the simulation
            echo "Running make questasim-postsynth-run..." | tee -a "$LOG_FILE"
            make questasim-postsynth-run BOOT_MODE=flash VCD_MODE=2 LINKER=flash_load >> "$LOG_FILE" 2>&1
            if [ $? -ne 0 ]; then
                echo "Error: Simulation failed for KERNEL_PARAMS=$KERNEL_PARAMS. Check the log for details." | tee -a "$LOG_FILE"
                exit 1
            fi

            # Collect VCD files for power analysis
            SIM_RESULTS_DIR="build/epfl_heepatia_heepatia_0.3.0/sim_postsynthesis-modelsim/logs"
            VCD_FILES=("$SIM_RESULTS_DIR"/waves-*.vcd)
            CLK_FILE="$SIM_RESULTS_DIR/clk_ns.txt"

            # Loop through all VCD files
            for VCD_FILE in "${VCD_FILES[@]}"; do
                VCD_INDEX=$(basename "$VCD_FILE" | sed -E 's/waves-([0-9]+)\.vcd/\1/')
                LABEL=${VCD_LABELS[$VCD_INDEX]}  # Fetch the label for this VCD index

                TIME_FILE="$SIM_RESULTS_DIR/time-$VCD_INDEX.txt"

                if [ ! -f "$TIME_FILE" ]; then
                    echo "Warning: Time file $TIME_FILE not found for $VCD_FILE. Skipping." | tee -a "$LOG_FILE"
                    continue
                fi

                # Run power analysis for different voltage settings
                for PWR_MODE in tt_0p50_25 tt_0p65_25 tt_0p80_25 tt_0p90_25; do
                    echo "Running power analysis for $VCD_FILE with $PWR_MODE..." | tee -a "$LOG_FILE"
                    make power-analysis PWR_ANALYSIS_MODE="$PWR_MODE" PWR_VCD="$VCD_FILE" >> "$LOG_FILE" 2>&1
                    if [ $? -ne 0 ]; then
                        echo "Error: Power analysis failed for $VCD_FILE with $PWR_MODE. Check the log for details." | tee -a "$LOG_FILE"
                        exit 1
                    fi

                    # Store the power analysis results
                    REPORT_DIR= A GIVEN PATH BASED ON PROJECT AND OTHER PARAMETERS I AM GIVING!!!
                    mkdir -p "$REPORT_DIR" >> "$LOG_FILE" 2>&1
                    cp -r implementation/power_analysis/reports/* "$REPORT_DIR/" >> "$LOG_FILE" 2>&1
                    cp "$CLK_FILE" "$REPORT_DIR/" >> "$LOG_FILE" 2>&1
                    cp "$TIME_FILE" "$REPORT_DIR/" >> "$LOG_FILE" 2>&1

                    echo "Power analysis results stored in $REPORT_DIR" | tee -a "$LOG_FILE"
                done
            done

            echo "Simulation and power analysis for --row_a=$row_a, --col_a=$col_a, --col_b=$col_b completed successfully." | tee -a "$LOG_FILE"

echo "All simulations and power analyses completed successfully at $(date)." | tee -a "$LOG_FILE"