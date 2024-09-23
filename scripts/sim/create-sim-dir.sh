#!/bin/bash

# Accessing arguments
MAKE_DIR=$1

if [ -z "$SIM_NAME" ]; then
    echo "Error: SIM_NAME is not provided. Please provide SIM_NAME when running this script."
    exit 1
fi
if [ -z "$PROJECT" ]; then
    echo "Error: PROJECT is not provided. Please provide PROJECT when running this script."
    exit 1
fi
if [ -z "$VCD_MODE" ]; then
    echo "Error: VCD_MODE is not provided. Please provide VCD_MODE when running this script."
    exit 1
fi
if [ -z "$TB_SYSCLK_ns" ]; then
    echo "Error: TB_SYSCLK_ns is not provided. Please provide TB_SYSCLK_ns when running this script."
    exit 1
fi
if [ -z "$PWR_ANALYSIS_MODE" ]; then
    echo "Error: PWR_ANALYSIS_MODE is not provided. Please provide PWR_ANALYSIS_MODE when running this script."
    exit 1
fi

# Display the received arguments (for verification)
echo "Received MAKE_DIR: $MAKE_DIR"
echo "Received SIM_NAME: $SIM_NAME"
echo "Received PROJECT: $PROJECT"
echo "Received VCD_MODE: $VCD_MODE"
echo "Received TB_SYSCLK_ns: $TB_SYSCLK_ns"
echo "Received PWR_ANALYSIS_MODE: $PWR_ANALYSIS_MODE"


# Check if the MAKE_DIR is set to "yes"
if [ "$MAKE_DIR" == "yes" ]; then
    echo "MAKE_DIR is set. Proceeding with directory creation and copying."

    # Check if the "sims" directory exists, if not, create it
    if [ ! -d "sims" ]; then
        echo "The 'sims' directory does not exist. Creating it now..."
        mkdir -p sims
    else
        echo "The 'sims' directory already exists."
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

    # Get the path to the main project directory (assuming the script is run from within the project root)
    MAIN_PROJECT_DIR=$(pwd)

    # Copy all directories and files except 'build/' and 'private/' to the heepatia directory
    echo "Copying files to $HEEPATIA_DIR (excluding 'build/' and 'private/')..."
    rsync -av --exclude 'build/' --exclude 'private/' --exclude '.vscode/' --exclude '.Xil/' --exclude 'sims/' --exclude '.git/' "$MAIN_PROJECT_DIR/" "$HEEPATIA_DIR"
else
    echo "MAKE_DIR is not set to 'yes'. Skipping directory creation and file copying."
    
    # Just set the HEEPATIA_DIR path for further steps
    HEEPATIA_DIR="sims/$SIM_NAME/heepatia"
    if [ ! -d "$HEEPATIA_DIR" ]; then
        echo "Error: The heepatia directory '$HEEPATIA_DIR' does not exist. Please set CREATE_FLAG to 'yes' to create it."
        exit 1
    fi
fi

# Change to the heepatia directory
cd "$HEEPATIA_DIR"

# Run the make app command with the provided PROJECT
echo "Running 'make app PROJECT=$PROJECT LINKER=flash_load BOOT_MODE=flash VCD_MODE=1' inside $HEEPATIA_DIR..."
make clean
make heepatia-gen-force
make set-tb-sysclk TB_SYSCLK_ns=$TB_SYSCLK_ns
make questasim-postsynth-build BOOT_MODE=flash VCD_MODE=$VCD_MODE LINKER=flash_load
make app PROJECT="$PROJECT" LINKER=flash_load BOOT_MODE=flash VCD_MODE=$VCD_MODE
make questasim-postsynth-run BOOT_MODE=flash VCD_MODE=$VCD_MODE LINKER=flash_load

# extract vcd timing

# run power analysis

# 