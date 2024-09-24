#!/bin/bash

# Check that all required arguments are provided
if [ "$#" -ne 6 ]; then
    echo "Usage: $0 <MAKE_DIR> <SIM_NAME> <PROJECT> <VCD_MODE> <TB_SYSCLK> <PWR_ANALYSIS_MODE>"
    exit 1
fi

# Accessing arguments passed to the script
MAKE_DIR=$1
SIM_NAME=$2
PROJECT=$3
VCD_MODE=$4
TB_SYSCLK=$5
PWR_ANALYSIS_MODE=$6

# Display the received arguments (for verification)
echo "Received MAKE_DIR: $MAKE_DIR"
echo "Received SIM_NAME: $SIM_NAME"
echo "Received PROJECT: $PROJECT"
echo "Received VCD_MODE: $VCD_MODE"
echo "Received TB_SYSCLK: $TB_SYSCLK"
echo "Received PWR_ANALYSIS_MODE: $PWR_ANALYSIS_MODE"


# Check if the MAKE_DIR is set to "yes"
if [ "$MAKE_DIR" == "yes" ]; then
    echo "MAKE_DIR is set. Proceeding with directory creation and copying."

    # Get the path to the main project directory (assuming the script is run from within the project root originally)
    MAIN_PROJECT_DIR=$(pwd)
    echo "Main project directory path (parent): $MAIN_PROJECT_DIR"

    # Move one directory up to create the 'sims' directory there
    cd ..

    # Check if the "sims" directory exists, if not, create it
    if [ ! -d "sims" ]; then
        echo "The 'sims' directory does not exist in the parent directory. Creating it now..."
        mkdir -p sims
    else
        echo "The 'sims' directory already exists in the parent directory."
    fi

    # Create a subdirectory with the name of the simulation inside the "sims" directory
    SIM_DIR="sims/$SIM_NAME"
    if [ -d "$SIM_DIR" ]; then
        echo "Error: The simulation directory '$SIM_DIR' already exists. Terminating the script."
        exit 1
    else
        echo "Creating simulation directory: $SIM_DIR"
        mkdir -p "$SIM_DIR"
    fi

    # Create the "heepatia" directory under the simulation directory
    HEEPATIA_DIR="$SIM_DIR/heepatia"
    echo "Creating directory: $HEEPATIA_DIR"
    mkdir -p "$HEEPATIA_DIR"

    # Copy all directories and files except the excluded ones to the heepatia directory
    echo "Copying files to $HEEPATIA_DIR (excluding 'build/', 'private/', '.vscode/', '.Xil/', 'sims/', '.git/')..."
    rsync -av --exclude 'build/' --exclude 'private/' "$MAIN_PROJECT_DIR/" "$HEEPATIA_DIR"
else
    echo "MAKE_DIR is not set to 'yes'. Skipping directory creation and file copying."
    
    # Just set the HEEPATIA_DIR path for further steps
    HEEPATIA_DIR="../sims/$SIM_NAME/heepatia"
    if [ ! -d "$HEEPATIA_DIR" ]; then
        echo "Error: The heepatia directory '$HEEPATIA_DIR' does not exist. Please set MAKE_DIR to 'yes' to create it."
        exit 1
    fi
fi


# Change to the heepatia directory
cd "$HEEPATIA_DIR"
echo "Current directory after moving to heepatia: $(pwd)"

# Run the make app command with the provided PROJECT
echo "Running 'make app PROJECT=$PROJECT LINKER=flash_load BOOT_MODE=flash VCD_MODE=$VCD_MODE' inside $HEEPATIA_DIR..."
make clean
make heepatia-gen-force
make set-tb-sysclk TB_SYSCLK=$TB_SYSCLK
make questasim-build BOOT_MODE=flash VCD_MODE=$VCD_MODE LINKER=flash_load
make app PROJECT="$PROJECT" LINKER=flash_load BOOT_MODE=flash VCD_MODE=$VCD_MODE
make questasim-run BOOT_MODE=flash VCD_MODE=$VCD_MODE LINKER=flash_load

# extract vcd timing

# run power analysis
exit 0

# example on how to run it: ./scripts/sim/create-sim-dir.sh yes able_to_run_rtl_copyWaves_test hello_world 0 10ns tt_0p80_25 