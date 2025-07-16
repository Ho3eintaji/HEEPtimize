# HEEPtimize 🚀

HEEPtimize is an advanced research platform based on **HEEPatia** and the underlying **X-HEEP** project, tailored for research into energy-efficient computing. While leveraging the core architecture of HEEPatia and X-HEEP, this version introduces several fundamental modifications:
* **Technology Porting:** The entire ASIC physical design flow is adapted for the **GlobalFoundries 22nm (GF22)** technology node.
* **Accelerator Configuration:** The hardware accelerators has been altered. IPs like `NM-Carus` and `NM-Caesar` have been decoupled for licensing reasons, while others like OpenEdgeCGRA remain integrated.

* **Tailored Software:** This version introduces new software applications and drivers developed specifically for the `heeptimize` hardware configuration


---

## 1. Prerequisites 🛠️

This project has the same core prerequisites as X-HEEP.

#### Environment Setup

An extended version of the `core-v-mini-mcu` Conda environment is required.

##### 1.  **Create the Conda Environment:**
```bash
make conda
```

##### 2.  **Activate the Environment:**
```bash
conda activate heepatia
```

#### Required Tools

Ensure the following tools are installed and available in your system's `PATH`:
* RISC-V GCC Toolchain (supporting at least `rv32imc`)
* Verible (for code formatting)
* GTKWave (for viewing waveforms)
* Siemens QuestaSim (version 2020.4 or newer is recommended)
* Synopsys Design Compiler (version 2020.09 or newer is recommended)
* Cadence Innovus (for Place & Route)

If you are on the EPFL ESL servers, you can initialize most of these tools using the provided environment scripts: `scripts/env.sh` (for BASH) or `scripts/env.csh` (for TCSH).

---

## 2. Project Setup ⚙️

Before you can build or simulate the project, you must set up the required dependencies and proprietary files, which have been removed from this public repository.

#### Core IP and Platform Vendoring

Key components, including the base platform and accelerators, must be "vendorized" (i.e., brought into the project locally). You can do this using the provided utility script.

1.  Obtain the required IP repositories (e.g., `x-heep`, `nm-carus`, etc.).
2.  Create a corresponding `<NAME>.vendor.hjson` file in the `hw/vendor/` directory.
3.  Run the vendor script. This command will clone the repository into the correct location:
    ```bash
    python3 util/vendor.py -vU hw/vendor/<NAME>.vendor.hjson
    ```

    You can update all vendored IPs at once by running `make vendor-update`.

The following components **must be vendorized** for the project to function:
* `x-heep`: The fundamental MCU platform required for the entire build system.
* `pulp_platform_opb_fll_if`: The FLL (Frequency-Locked Loop) IP required for clock generation.
* `nm-carus` and `nm-caesar`: Optional hardware accelerators.

#### Proprietary Technology Files

Due to licensing restrictions, **all proprietary foundry and technology files have been removed**. To run the full ASIC flow, **you must provide your own files**.

The following directories have been cleared and must be populated with your licensed GF22nm PDK files and scripts:

* `hw/asic/bondpads/`
* `hw/asic/mem-power-switches/`
* `hw/asic/pads/`
* `hw/asic/sealring/`
* `hw/asic/std-cells/`
* `hw/asic/std-cells-memories/`
* `implementation/pnr/` (This should contain your Place & Route scripts)

---

## 3.  **Usage and Build Flow 🏗️**

The `makefile` automates all major project flows. Here is the recommended sequence of operations.

### Step 1: Generate Hardware

First, generate all necessary hardware source files, including the top-level wrappers and software headers.
```bash
make heepatia-gen
```

You can specify the hardware target using the `TARGET` parameter:
* `asic` (default): For a full ASIC implementation.
* `pynq-z2`: For the PYNQ-Z2 FPGA board.
* `zcu104`: For the ZCU104 FPGA board.

Example for FPGA:
```bash
make heepatia-gen TARGET=pynq-z2
```

To force a full regeneration, use `make heepatia-gen-force`.

### Step 2: Compile Software

Compile software applications using the `app` target. The build system will automatically compile required libraries (e.g., for Carus) first. You must specify the application with the `PROJECT` variable.

```bash
make app PROJECT=your_app_name
```

The `makefile` supports different compilers via the `TOOLCHAIN` parameter:
* `OHW` (default): The CORE-V compiler from Embecosm.
* `GCC`: The standard RISC-V GCC compiler for `rv32imfc`.
* `POS`: A Clang+GCC toolchain with support for Posit arithmetic.

Example using standard GCC:
```bash
make app PROJECT=hello_world TOOLCHAIN=GCC
```

### Step 3: RTL Simulation

You can simulate the design using either Verilator or QuestaSim. Ensure firmware is compiled first.

**Verilator:**
```bash
# Build the simulation model
make verilator-build

# Run the simulation (with waveform dump)
make verilator-run FIRMWARE=build/sw/app/main.hex

# View the waveform
make verilator-waves
```

**QuestaSim:**
```bash
# Build the simulation model
make questasim-build

# Run the simulation
make questasim-run FIRMWARE=build/sw/app/main.hex

# Run with a GUI
make questasim-gui

# View the waveform
make questasim-waves
```
Key simulation parameters can be customized, including `FIRMWARE`, `BOOT_MODE`, `MAX_CYCLES`, and `BYPASS_FLL`.

### Step 4: ASIC Flow (Synthesis & PnR)

The makefile provides targets for running a full ASIC implementation flow.

##### 1.  Synthesize the Design:

This target uses Synopsys DC. The output will be in `implementation/synthesis/`.

```bash
make synthesis
```

##### 2.  **Place and Route (PnR):**

This requires you to provide your own PnR scripts in the `implementation/pnr/` directory.

```bash
    make pnr
```
##### 3.  **Post-Synthesis & Post-Layout Simulation:**

Run simulations on the generated netlists to verify timing and functionality.

```bash
# Post-synthesis simulation
make questasim-postsynth-run

# Post-layout simulation
make questasim-postlayout-run
```
##### 4.  **Power Analysis:**
Run power analysis using PrimePower on a VCD generated from a post-synthesis simulation.
```bash
make power-analysis
```

### Step 5: FPGA Implementation

To program an FPGA board, follow these steps:

##### 1.  **Generate Hardware for FPGA:**
```bash
make heepatia-gen TARGET=pynq-z2    
```

##### 2.  **Run Vivado Synthesis:**
```bash
make vivado-fpga-synth TARGET=pynq-z2  
```

##### 3.  **RProgram the FPGA:**
```bash
make vivado-fpga-pgm TARGET=pynq-z2
```
---

## 4.  **Cleaning 🧹**

    To remove all generated files and build artifacts, run:
    
    ```bash
    make clean
    ```

---

## License
This project is licensed under the [Solderpad Hardware License, Version 2.1](https://solderpad.org/licenses/SHL-2.1/).
- Original Copyright: Copyright 2025 EPFL 
