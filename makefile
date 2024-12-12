# Copyright 2022 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Hossein Taji
# Date: 29/04/2024
# Description: Top-level makefile for heepatia

#############################
# ----- CONFIGURATION ----- #
#############################

# Global configuration
ROOT_DIR			:= $(realpath .)
BUILD_DIR 			:= build

# FUSESOC and Python values (default)
ifndef CONDA_DEFAULT_ENV
$(info USING VENV)
FUSESOC = ./.venv/bin/fusesoc
PYTHON  = ./.venv/bin/python
else
$(info USING MINICONDA $(CONDA_DEFAULT_ENV))
FUSESOC := $(shell which fusesoc)
PYTHON  := $(shell which python)
endif

FORMAT ?= false

# NMC slaves number
CARUS_NUM			?= 1
CAESAR_NUM			?= 1

# # CARUS configuration

# CARUS_SIZE			?= 0x00008000
# CARUS_NUM_BANKS     ?= 4

# X-HEEP configuration
XHEEP_DIR			:= $(ROOT_DIR)/hw/vendor/x-heep
MCU_CFG_PERIPHERALS ?= $(ROOT_DIR)/config/mcu-gen.hjson
X_HEEP_CFG  		?= $(ROOT_DIR)/config/mcu-gen-system.hjson
X_HEEP_CFG_FPGA    	?= $(ROOT_DIR)/config/mcu-gen-system-fpga.hjson
PAD_CFG				?= $(ROOT_DIR)/config/heep-pads.hjson
PAD_CFG_FPGA	    ?= $(ROOT_DIR)/config/heep-pads-fpga.hjson
EXT_PAD_CFG			?= $(ROOT_DIR)/config/heepatia-pads.hjson
EXTERNAL_DOMAINS	:= 3 # NM-Carus + OECGRA + NM-Caesar //todo: not sure
MCU_GEN_OPTS		:= \
	--memorybanks $(MEMORY_BANKS) \
	--memorybanks_il $(MEMORY_BANKS_IL) \
	--bus $(BUS) \
	--config $(X_HEEP_CFG) \
	--cfg_peripherals $(MCU_CFG_PERIPHERALS) \
	--pads_cfg $(PAD_CFG) \
	--external_domains $(EXTERNAL_DOMAINS)
MCU_GEN_OPTS_FPGA	:= \
	--memorybanks $(MEMORY_BANKS) \
	--memorybanks_il $(MEMORY_BANKS_IL) \
	--bus $(BUS) \
	--config $(X_HEEP_CFG_FPGA) \
	--cfg_peripherals $(MCU_CFG_PERIPHERALS) \
	--pads_cfg $(PAD_CFG_FPGA) \
	--external_domains $(EXTERNAL_DOMAINS)
HEEPATIA_TOP_TPL		:= $(ROOT_DIR)/hw/ip/heepatia_top.sv.tpl
PAD_RING_TPL			:= $(ROOT_DIR)/hw/ip/pad-ring/pad_ring.sv.tpl
# PAD_IO_TPL              := $(ROOT_DIR)/implementation/pnr/inputs/heepatia.io.tpl
MCU_GEN_LOCK			:= $(BUILD_DIR)/.mcu-gen.lock

# heepatia configuration
HEEPATIA_GEN_CFG	:= config/heepatia-cfg.hjson
HEEPATIA_GEN_OPTS	:= \
	--cfg $(HEEPATIA_GEN_CFG)\
	--carus_num $(CARUS_NUM) \
	--caesar_num $(CAESAR_NUM) 
# --carus_num $(CARUS_NUM) \
# --carus_size $(CARUS_SIZE) \
# --carus_num_banks $(CARUS_NUM_BANKS)
HEEPATIA_GEN_TPL  := \
	hw/ip/heepatia-ctrl/data/heepatia_ctrl.hjson.tpl \
	hw/ip/packages/heepatia_pkg.sv.tpl \
	hw/ip/heepatia-ctrl/rtl/heepatia_ctrl_reg.sv.tpl \
	sw/external/lib/runtime/heepatia.h.tpl
HEEPATIA_GEN_LOCK := build/.heepatia-gen.lock

# Implementation specific variables
# TARGET options are 'asic' (default) and 'pynq-z2'
TARGET ?= asic

# Simulation DPI libraries
DPI_LIBS			:= $(BUILD_DIR)/sw/sim/uartdpi.so
DPI_CINC			:= -I$(dir $(shell which verilator))../share/verilator/include/vltstd

# Simulation configuration
LOG_LEVEL			?= LOG_NORMAL
BOOT_MODE			?= force # jtag: wait for JTAG (DPI module), flash: boot from flash, force: load firmware into SRAM
ifeq ($(BOOT_MODE), jtag)
	FIRMWARE		?= $(ROOT_DIR)/build/sw/app/main.hex.srec
else
	FIRMWARE		?= $(ROOT_DIR)/build/sw/app/main.hex
endif
VCD_MODE			?= 2 # QuestaSim-only - 0: no dumo, 1: dump always active, 2: dump triggered by GPIO 0
BYPASS_FLL          ?= 1 # 0: FLL enabled, 1: FLL bypassed (TODO: make FLL work and set this to 0 by default)
MAX_CYCLES			?= 100000000
FUSESOC_FLAGS		?=
FUSESOC_ARGS		?=

# Flash file
FLASHWRITE_FILE		?= $(FIRMWARE)

# QuestaSim
FUSESOC_BUILD_DIR			= $(shell find $(BUILD_DIR) -type d -name 'epfl_heepatia_heepatia_*' 2>/dev/null | sort | head -n 1)
QUESTA_SIM_DIR				= $(FUSESOC_BUILD_DIR)/sim-modelsim
QUESTA_SIM_RTL_GF22_DIR	= $(FUSESOC_BUILD_DIR)/sim_rtl_gf22-modelsim
QUESTA_SIM_POSTSYNTH_DIR 	= $(FUSESOC_BUILD_DIR)/sim_postsynthesis-modelsim
QUESTA_SIM_POSTLAYOUT_DIR 	= $(FUSESOC_BUILD_DIR)/sim_postlayout-modelsim

# Waves
SIM_VCD 			?= $(BUILD_DIR)/sim-common/questa-waves.fst

# Application data generation
# NOTE: the application makefile may accept additional parameters, e.g.:
# 	KERNEL_PARAMS="--row_a 8 --col_a 8 --col_b 256"
# for carus-matmul.
APP_MAKE 			:= $(wildcard sw/applications/$(PROJECT)/*akefile)

TOOLCHAIN ?= OHW #also GCC

# Custom preprocessor definitions
CDEFS				?=

# Software build configuration
SW_DIR		:= sw
LINK_FOLDER := $(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))")/sw/linker

# Testing flags
# Optional TEST_FLAGS options are '--compile-only'
TEST_FLAGS=

# Dummy target to force software rebuild
PARAMS = $(PROJECT)

# Benchmarking configuration
PWR_VCD ?= $(QUESTA_SIM_POSTSYNTH_DIR)/logs/waves-0.vcd
THR_TESTS ?= scripts/performance-analysis/throughput-tests.txt
PWR_TESTS ?= scripts/performance-analysis/power-tests.txt

#CAESAR AND CARUS PL Netlist and SDF
CARUS_PL_SDF := $(ROOT_DIR)/hw/vendor/nm-carus-backend-opt/implementation/pnr/outputs/nm-carus/sdf/NMCarus_top_pared.sdf
CAESAR_PL_SDF := $(ROOT_DIR)/hw/vendor/nm-caesar-backend-opt/implementation/pnr/outputs/nm-caesar/sdf/NMCaesar_top_pared.sdf

#HEEPATIA PL Netlist and SDF
# HEEPATIA_PL_NET := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia_pg.v
# HEEPATIA_PL_SDF := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia.sdf

# ==============================================================================
# Power analysis
# ==============================================================================
PWR_TYPE ?= postsynth
SYNTH_DIR ?= $(ROOT_DIR)/implementation/synthesis/last_output
HEEPATIA_PL_NET := $(SYNTH_DIR)/netlist.v
HEEPATIA_PL_SDF := $(SYNTH_DIR)/netlist.sdf  # NOT REQUIRED
PWR_VCD ?= $(QUESTA_SIM_POSTSYNTH_DIR)/logs/waves-0.vcd  # private/simcommons/log_carus-matmul_2ns/waves-0.vcd
PWR_ANALYSIS_MODE ?= tt_0p80_25 # tt_0p50_25, tt_0p65_25, tt_0p80_25, tt_0p90_25, wc
# HEEPATIA_PL_NET_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia_pg_power_analysis.v
# HEEPATIA_PL_SDF_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia.sdf
# HEEPATIA_PL_SDF_PATCHED_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia.patched.sdf

# ----- BUILD RULES ----- #


# Get the path of this Makefile to pass to the Makefile help generator
MKFILE_PATH = $(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))")
export FILE_FOR_HELP = $(MKFILE_PATH)/makefile
export XHEEP_DIR


## Print the Makefile help
## @param WHICH=xheep,all,<none> Which Makefile help to print. Leaving blank (<none>) prints only HEEPatia's.
help:
ifndef WHICH
	${XHEEP_DIR}/util/MakefileHelp
else ifeq ($(filter $(WHICH),xheep x-heep),)
	${XHEEP_DIR}/util/MakefileHelp
	$(MAKE) -C $(XHEEP_DIR) help
else
	$(MAKE) -C $(XHEEP_DIR) help
endif

## @section Conda
.PHONY: conda
conda: 
	conda env create -f environment.yml
# conda: environment.yml
# 	conda env create -f environment.yml

# environment.yml: python-requirements.txt
# 	util/python-requirements2conda.sh


# Default alias
# -------------
.PHONY: all
all: heepatia-gen

## @section RTL & SW generation

## X-HEEP MCU system
.PHONY: mcu-gen
mcu-gen: $(MCU_GEN_LOCK)
ifeq ($(TARGET), asic)
$(MCU_GEN_LOCK): $(MCU_CFG) $(PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen LINK_FOLDER=$(LINK_FOLDER)
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else ifeq ($(TARGET), pynq-z2)
$(MCU_GEN_LOCK): $(PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU for PYNQ-Z2..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen LINK_FOLDER=$(LINK_FOLDER) X_HEEP_CFG=$(X_HEEP_CFG_FPGA) MCU_CFG_PERIPHERALS=$(MCU_CFG_PERIPHERALS)
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else ifeq ($(TARGET), zcu104)
$(MCU_GEN_LOCK): $(PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU for zcu104..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen LINK_FOLDER=$(LINK_FOLDER) X_HEEP_CFG=$(X_HEEP_CFG_FPGA) MCU_CFG_PERIPHERALS=$(MCU_CFG_PERIPHERALS)
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else
	$(error ### ERROR: Unsupported target implementation: $(TARGET))
endif

.PHONY: extract-vcd-timing
extract-vcd-timing: 
	@echo "Running VCD timing extraction script..."
	@./scripts/extract_vcd_timing.sh $(QUESTA_SIM_DIR) $(QUESTA_SIM_POSTSYNTH_DIR)

MAKE_DIR_RUN_SIM ?= "yes"
.PHONY: run-sim
run-sim: 
	@echo "Running simulation for $(SIM_NAME)..."
	@SIM_NAME=$(SIM_NAME) PROJECT=$(PROJECT) VCD_MODE=$(VCD_MODE) TB_SYSCLK=$(TB_SYSCLK) PWR_ANALYSIS_MODE=$(PWR_ANALYSIS_MODE) \
  	bash ./scripts/sim/create-sim-dir.sh "$(MAKE_DIR_RUN_SIM)"

TB_SYSCLK ?=  10ns # ps values: 8200ps, 2880ps, 1730ps, or 1450ps
.PHONY: set-tb-sysclk
set-tb-sysclk:
	@if [ -z "$(TB_SYSCLK)" ]; then \
		echo "Error: TB_SYSCLK is not defined. Please specify TB_SYSCLK when running this target."; \
		exit 1; \
	fi
	@sed -i 's/const time SIM_CLK_PERIOD = [^;]*/const time SIM_CLK_PERIOD = $(TB_SYSCLK)/' tb/tb_top.sv
	@echo "SIM_CLK_PERIOD has been set to $(TB_SYSCLK)."

.PHONY: read-tb-sysclk
read-tb-sysclk:
	# Extract the TB_SYSCLK value directly from tb/tb_top.sv
	@TB_SYSCLK=$$(grep -E 'const time SIM_CLK_PERIOD = [0-9]+[ ]*(ps|ns)' tb/tb_top.sv | sed -E 's/.*= *([0-9]+[ ]*(ps|ns));.*/\1/'); \
	if [ -z "$$TB_SYSCLK" ]; then \
		echo "Error: Could not read TB_SYSCLK from tb/tb_top.sv."; \
		exit 1; \
	fi; \
	echo "Read TB_SYSCLK from file: $$TB_SYSCLK"; \
	TB_SYSCLK_VALUE=$$(echo "$$TB_SYSCLK" | sed 's/[a-zA-Z]*//g' | tr -d ' '); \
	TB_SYSCLK_UNIT=$$(echo "$$TB_SYSCLK" | sed 's/[0-9. ]*//g'); \
	if [ "$$TB_SYSCLK_UNIT" = "ps" ]; then \
		clk_ns=$$(echo "scale=3; $$TB_SYSCLK_VALUE / 1000" | bc); \
		echo "Converted TB_SYSCLK from ps to ns: $$clk_ns"; \
	elif [ "$$TB_SYSCLK_UNIT" = "ns" ]; then \
		clk_ns=$$TB_SYSCLK_VALUE; \
		echo "TB_SYSCLK is already in ns: $$clk_ns"; \
	else \
		echo "Error: TB_SYSCLK must be specified in either ns or ps."; \
		exit 1; \
	fi; \
	echo "$$clk_ns" > $(BUILD_DIR)/sim-common/clk_ns.txt


.PHONY: heepatia-gen-force
heepatia-gen-force:
	rm -rf build/.mcu-gen.lock build/.heepatia-gen.lock;
	$(MAKE) heepatia-gen

## Generate HEEPatia files
## @param TARGET=asic(default),pynq-z2,zcu104
.PHONY: heepatia-gen
heepatia-gen: $(HEEPATIA_GEN_LOCK)
$(HEEPATIA_GEN_LOCK): $(HEEPATIA_GEN_CFG) $(HEEPATIA_GEN_TPL) $(HEEPATIA_TOP_TPL) $(PAD_RING_TPL) $(MCU_GEN_LOCK) $(ROOT_DIR)/tb/tb_util.svh.tpl
ifeq ($(TARGET), asic)
	@echo "### Generating heepatia top and pad rings for ASIC..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/tb/ \
		--tpl-sv $(ROOT_DIR)/tb/tb_util.svh.tpl
	# python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
	# 	--outdir $(ROOT_DIR)/implementation/pnr/inputs/ \
	# 	--tpl-sv $(PAD_IO_TPL)
	@echo "### Generating heepatia files..."
else ifeq ($(TARGET), pynq-z2)
	@echo "### Generating heepatia top and padrings for PYNQ-Z2..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/tb/ \
		--tpl-sv $(ROOT_DIR)/tb/tb_util.svh.tpl
else ifeq ($(TARGET), zcu104)
	@echo "### Generating heepatia top and padrings for zcu104..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/tb/ \
		--tpl-sv $(ROOT_DIR)/tb/tb_util.svh.tpl
else
	$(error ### ERROR: Unsupported target implementation: $(TARGET))
endif
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir hw/ip/heepatia-ctrl/data \
		--tpl-sv hw/ip/heepatia-ctrl/data/heepatia_ctrl.hjson.tpl
	sh hw/ip/heepatia-ctrl/gen-heepatia-ctrl.sh
	sh sw/external/lib/drivers/fll/fll_regs_gen.sh
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir hw/ip/heepatia-ctrl/rtl \
		--tpl-sv hw/ip/heepatia-ctrl/rtl/heepatia_ctrl_reg.sv.tpl
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir hw/ip/packages \
		--tpl-sv hw/ip/packages/heepatia_pkg.sv.tpl \
		--corev_pulp $(COREV_PULP)
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir sw/external/lib/runtime \
		--tpl-c sw/external/lib/runtime/heepatia.h.tpl
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir sw/external/lib/drivers/carus/ \
		--tpl-c sw/external/lib/drivers/carus/carus.h.tpl
	python3 util/heepatia-gen.py $(HEEPATIA_GEN_OPTS) \
		--outdir sw/external/lib/drivers/carus \
		--tpl-c sw/external/lib/drivers/carus/carus.c.tpl		
ifeq ($(FORMAT), true)
	util/format-verible
	$(FUSESOC) run --no-export --target lint epfl:heepatia:heepatia
endif
	@echo "### DONE! heepatia files generated successfully"
	touch $@

# # Verible format
# .PHONY: format
# format: $(HEEPATIA_GEN_LOCK)
# 	@echo "### Formatting heepatia RTL files..."
# 	fusesoc run --no-export --target format epfl:heepatia:heepatia

# # Static analysis
# .PHONY: lint
# lint: $(HEEPATIA_GEN_LOCK)
# 	@echo "### Checking heepatia syntax and code style..."
# 	fusesoc run --no-export --target lint epfl:heepatia:heepatia

## @section Simulation

## @subsection Verilator RTL simulation

## Build simulation model (do not launch simulation)
.PHONY: verilator-build
verilator-build: $(HEEPATIA_GEN_LOCK)
	$(FUSESOC) run --no-export --target sim --tool verilator --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS)

## Build simulation model and launch simulation
.PHONY: verilator-sim
verilator-sim: | check-firmware verilator-build .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation
.PHONY: verilator-run
verilator-run: | check-firmware .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation without waveform dumping
.PHONY: verilator-opt
verilator-opt: | check-firmware .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		--trace=false \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Open dumped waveform with GTKWave
.PHONY: verilator-waves
verilator-waves: $(BUILD_DIR)/sim-common/waves.fst | .check-gtkwave
	gtkwave -a tb/misc/verilator-waves.gtkw $<

## @subsection QuestaSim RTL simulation

## Build simulation model
.PHONY: questasim-build
questasim-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_DIR) ; make opt

## Build simulation model and launch simulation
.PHONY: questasim-sim
questasim-sim: | check-firmware questasim-build $(QUESTA_SIM_DIR)/logs/
	$(FUSESOC) run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation
.PHONY: questasim-run
questasim-run: | check-firmware $(QUESTA_SIM_DIR)/logs/
	$(FUSESOC) run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation in GUI mode
.PHONY: questasim-gui
questasim-gui: | check-firmware $(QUESTA_SIM_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE)"

## Open dumped waveforms in GTKWave
.PHONY: questasim-waves
questasim-waves: $(SIM_VCD) | .check-gtkwave
	gtkwave -a tb/misc/questasim-waves.gtkw $<

$(BUILD_DIR)/sim-common/questa-waves.fst: $(BUILD_DIR)/sim-common/waves.vcd | .check-gtkwave
	@echo "### Converting $< to FST..."
	vcd2fst $< $@

## DPI libraries for QuestaSim
.PHONY: tb-dpi
tb-dpi: $(DPI_LIBS)
$(BUILD_DIR)/sw/sim/uartdpi.so: hw/vendor/x-heep/hw/vendor/lowrisc_opentitan/hw/dv/dpi/uartdpi/uartdpi.c | $(BUILD_DIR)/sw/sim/
	$(CC) -shared -Bsymbolic -fPIC -o $@ $< -lutil

## @section Post-synthesis and post-layout simulations

## @subsection QuestaSim RTL + tsmc16 netlist of black boxes simulation

## Build simulation model
.PHONY: questasim-gf22-build
questasim-gf22-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
	fusesoc run --no-export --target sim_rtl_gf22 --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_RTL_GF22_DIR) ; make opt

## Launch simulation. Bootmode set to flash.
.PHONY: questasim-gf22-run
questasim-gf22-run:
	fusesoc run --no-export --target sim_rtl_gf22 --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=flash \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation in GUI mode. Bootmode set to flash.
.PHONY: questasim-gf22-gui
questasim-gf22-gui: | $(QUESTA_SIM_RTL_GF22_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_RTL_GF22_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=flash vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"


# Questasim PostSynth Simulation (with no timing)
.PHONY: questasim-postsynth-build
questasim-postsynth-build: set-tb-sysclk $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_postsynthesis --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS);
	cd $(QUESTA_SIM_POSTSYNTH_DIR) ; make opt | tee fusesoc_questasim_postsynthesis.log

## Questasim Postsynth run
.PHONY: questasim-postsynth-run
questasim-postsynth-run: | check-firmware $(QUESTA_SIM_POSTSYNTH_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_postsynthesis --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log
	$(MAKE) extract-vcd-timing
	$(MAKE) read-tb-sysclk
	@cp $(BUILD_DIR)/sim-common/clk_ns.txt $(QUESTA_SIM_POSTSYNTH_DIR)/logs/

## Launch simulation in GUI mode
.PHONY: questasim-postsynth-gui
questasim-postsynth-gui: | check-firmware $(QUESTA_SIM_POSTSYNTH_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_POSTSYNTH_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"

## @subsection QuestaSim PostLayout simulation

## Questasim PostLayout Simulation (with no timing)
.PHONY: questasim-postlayout-build
questasim-postlayout-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_postlayout --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS);
	cd $(QUESTA_SIM_POSTLAYOUT_DIR) ; make opt | tee fusesoc_questasim_postslayout.log

## Questasim post-layout run
.PHONY: questasim-postlayout-run
questasim-postlayout-run: check-firmware
	$(FUSESOC) run --no-export --target sim_postlayout --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch simulation in GUI mode
.PHONY: questasim-postlayout-gui
questasim-postlayout-gui: check-firmware
	$(MAKE) -C $(QUESTA_SIM_POSTLAYOUT_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"


## @section Synthesis

## HEEperator synthesis with Synopsys DC Shell
.PHONY: synthesis
synthesis: $(HEEPATIA_GEN_LOCK)
	$(FUSESOC) run --no-export --target asic_synthesis --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS) 2>&1 | tee fusesoc_synthesis.log

## @section Place and Route

## PnR debug
.PHONY: pnr_debug
pnr_debug: 
	pushd implementation/pnr/ ; ./run_pnr_flow.csh debug; popd;

## PnR only
.PHONY: pnr
pnr: 
	pushd implementation/pnr/ ; ./run_pnr_flow.csh; popd;

# Launch Innovus GUI
.PHONY: launch-ivs
launch-ivs: .check-innovus
	innovus -common_ui -execute "gui_show"
# Check tools
.PHONY: .check-innovus
.check-innovus:
	@if [ `which innovus &> /dev/null` ]; then \
	printf -- "### ERROR: 'innovus' is not in PATH.\n" >&2; \
	exit 1; fi

## @section FPGA implementation

## Synthesis for FPGA
.PHONY: vivado-fpga-synth
vivado-fpga-synth:
	@echo "### Running FPGA implementation..."
	$(FUSESOC) run --no-export --target $(TARGET) --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia $(FUSESOC_ARGS)

## Program the FPGA using Vivado
.PHONY: vivado-fpga-pgm
vivado-fpga-pgm:
	@echo "### Programming the FPGA..."
	$(MAKE) -C $(FUSESOC_BUILD_DIR)/$(TARGET)-vivado pgm

# Flash ESL programmer
.PHONY:flash-prog
flash-prog:
	@echo "### Programming the flash..."
	cd $(XHEEP_DIR)/sw/vendor/yosyshq_icestorm/iceprog && make; \
	./iceprog -d i:0x0403:0x6011 -I B $(ROOT_DIR)/$(BUILD_DIR)/sw/app-flash/main.hex;

# ==============================================================================
# EVE analysis
# ==============================================================================
EVE_DATA_DIR  ?= private/matmul_postsynth_sims
EVE_DIR  := $(ROOT_DIR)/scripts/eve
EVE_DATA_DIR_FULL := $(ROOT_DIR)/$(EVE_DATA_DIR)

.PHONY: eve_power_analysis
eve_power_analysis:
	@echo "### Running EVE power analysis..."
	python3 $(EVE_DIR)/power-analysis.py \
	--data_dir=$(EVE_DATA_DIR_FULL) \
	--root_dir=$(ROOT_DIR) \
	--eve_dir=$(EVE_DIR) 

## @section Benchmarks

## Launch benchmark simulations on Verilator and generate CSV throughput report
.PHONY: benchmark-throughput
benchmark-throughput: build/performance-analysis/throughput.csv
build/performance-analysis/throughput.csv: $(THR_TESTS) | build/performance-analysis/
	@echo "### Running benchmark simulations for throughput extraction..."
	python3 scripts/performance-analysis/throughput-analysis.py \
		$(THR_TESTS) $@

## Launch benchmark simulations on post-layout netlist and generate CSV power report
.PHONY: benchmark-power
benchmark-power: build/performance-analysis/power.csv
build/performance-analysis/power.csv: $(PWR_TESTS) | build/performance-analysis/
	@echo "### Running benchmark simulations for power extraction..."
	python3 scripts/performance-analysis/power-analysis.py \
		$(PWR_TESTS) \
		build/sim-common $@

## Generate throughput benchmark chart
.PHONY: charts
charts: build/performance-analysis/power.csv build/performance-analysis/throughput.csv
	@echo "### Generating charts..."
	python3 scripts/performance-analysis/benchmark-charts.py $^ build/performance-analysis

## @section Power Analysis

## PAth files
.PHONY: patch-files-power-analysis
patch-files-power-analysis: $(BUILD_DIR)/.patch-files-power-analysis.lock
$(BUILD_DIR)/.patch-files-power-analysis.lock: $(HEEPATIA_PL_NET) $(HEEPATIA_PL_SDF).gz
#   the LIB and LEF of the FLL are wrong as the VDDA power pin is missing, thus deleting it so that power analysis can be done
	cp $(HEEPATIA_PL_NET) $(HEEPATIA_PL_NET_PA)
	sed -i '/.VDDA(VDD)/d' $(HEEPATIA_PL_NET_PA)
	touch $(BUILD_DIR)/.patch-files-power-analysis.lock

## Perform power analysis
.PHONY: power-analysis
power-analysis:
# power-analysis: $(BUILD_DIR)/.patch-files-power-analysis.lock $(PWR_VCD)
	@echo "### Running power analysis..."
	rm -rf implementation/power_analysis/reports/*
	pushd implementation/power_analysis/; ./run_pwr_flow.sh $(PWR_VCD) $(HEEPATIA_PL_NET) $(HEEPATIA_PL_SDF) heepatia_top $(PWR_ANALYSIS_MODE); popd;

## @section Software

## HEEPatia applications
## @param TOOLCHAIN=OHW(default),GCC,POS
.PHONY: app
app: $(HEEPATIA_GEN_LOCK) | carus-sw $(BUILD_DIR)/sw/app/
ifneq ($(APP_MAKE),)
	$(MAKE) -C $(dir $(APP_MAKE))
endif
ifeq ($(TOOLCHAIN), OHW)
	@echo "### Building application with OHW compiler..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) $(MAKECMDGOALS) LINK_FOLDER=$(LINK_FOLDER) COMPILER_PREFIX=riscv32-corev- ARCH=rv32imfc_zicsr_zifencei_xcvhwlp_xcvmem_xcvmac_xcvbi_xcvalu_xcvsimd_xcvbitmanip
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app/ \;
else ifeq ($(TOOLCHAIN), POS)
	@echo "### Building application for SRAM execution with Clang+GCC posit compiler..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) $(MAKECMDGOALS) LINK_FOLDER=$(LINK_FOLDER) COMPILER=clang ARCH=rv32gcxposit1 RISCV=/shares/eslfiler1/common/esl/HEEPatia/compilers/rv32gcxposit
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app/ \;
else
	@echo "### Building application for SRAM execution with GCC compiler..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) $(MAKECMDGOALS) LINK_FOLDER=$(LINK_FOLDER) ARCH=rv32imc
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app/ \;
endif

## NM-Carus kernels and startup code
.PHONY: carus-sw
carus-sw:
	$(MAKE) -C $(SW_DIR) carus

## Dummy target to force software rebuild
$(PARAMS):
	@echo "### Rebuilding software..."

## @section Utilities

## Check if the firmware is compiled
.PHONY: .check-firmware
check-firmware:
	@if [ ! -f $(FIRMWARE) ]; then \
		echo "\033[31mError: FIRMWARE has not been compiled! Simulation won't work!\033[0m"; \
		exit 1; \
	fi

## Update vendored IPs
.PHONY: vendor-update
vendor-update:
	@echo "### Updating vendored IPs..."
	find hw/vendor -maxdepth 1 -type f -name "*.vendor.hjson" -exec python3 util/vendor.py -vU '{}' \;
	$(MAKE) clean-lock
	$(MAKE) heepatia-gen

## Check if fusesoc is available
.PHONY: .check-fusesoc
.check-fusesoc:
	@if [ ! `which fusesoc` ]; then \
	printf -- "### ERROR: 'fusesoc' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

## Check if GTKWave is available
.PHONY: .check-gtkwave
.check-gtkwave:
	@if [ ! `which gtkwave` ]; then \
	printf -- "### ERROR: 'gtkwave' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

## Check simulation parameters
.PHONY: .verilator-check-params
.verilator-check-params:
	@if [ "$(BOOT_MODE)" = "flash" ]; then \
		echo "### ERROR: Verilator simulation with flash boot is not supported" >&2; \
		exit 1; \
	fi

## Run HEEPatia tests
.PHONY: test
test: heepatia-gen-force
	$(RM) test/*.log
	python test/test_apps.py $(TEST_FLAGS) 2>&1 | tee test/test_apps.log
	@echo "You can also find the output in test/test_apps.log"

## Create directories
%/:
	mkdir -p $@


## @section Cleaning

## Clean build directory
.PHONY: clean clean-lock
clean:
	$(RM) $(HEEPATIA_GEN_LOCK)
	$(RM) hw/ip/heepatia_top.sv
	$(RM) hw/ip/pad-ring/pad-ring.sv
	$(RM) hw/ip/heepatia-ctrl/data/*.hjson
	$(RM) hw/ip/heepatia-ctrl/rtl/heepatia_ctrl_reg_top.sv
	$(RM) hw/ip/heepatia-ctrl/rtl/heepatia_ctrl_reg_pkg.sv
	$(RM) hw/ip/heepatia-ctrl/rtl/heepatia_ctrl_reg.sv
	$(RM) sw/external/lib/drivers/carus/carus.h
	$(RM) sw/external/lib/drivers/carus/carus.c
	$(RM) sw/device/include/heepatia.h
	$(RM) sw/device/include/heepatia_ctrl_reg.h
	$(RM) -r $(BUILD_DIR)
	$(MAKE) -C $(HEEP_DIR) clean-all
	$(MAKE) -C $(SW_DIR) clean
clean-lock:
	$(RM) $(BUILD_DIR)/.*.lock


## @section Format and Variables

## Verible format
.PHONY: format
format: $(HEEPATIA_GEN_LOCK)
	@echo "### Formatting heepatia RTL files..."
	util/format-verible

.PHONY: lint
	@echo "### Linting heepatia RTL files..."
	$(FUSESOC) run --no-export --target lint epfl:heepatia:heepatia


## Static analysis
.PHONY: lint
lint: $(HEEPATIA_GEN_LOCK)
	@echo "### Checking heepatia syntax and code style..."
	$(FUSESOC) run --no-export --target lint epfl:heepatia:heepatia

## Print variables
.PHONY: .print
.print:
	@echo "APP_MAKE: $(APP_MAKE)"
	@echo "KERNEL_PARAMS: $(KERNEL_PARAMS)"
	@echo "FUSESOC_ARGS: $(FUSESOC_ARGS)"


# ----- INCLUDE X-HEEP RULES ----- #
export X_HEEP_CFG
export MCU_CFG_PERIPHERALS
export PAD_CFG
export EXTERNAL_DOMAINS
export FLASHWRITE_FILE
export HEEP_DIR = $(ROOT_DIR)/hw/vendor/x-heep
XHEEP_MAKE 		= $(HEEP_DIR)/external.mk
include $(XHEEP_MAKE)
