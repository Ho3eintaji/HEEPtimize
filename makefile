# Copyright 2022 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Michele Caon, Luigi Giuffrida, Hossein Taji
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

FORMAT ?= true

# NMC slaves number
CARUS_NUM			?= 1
CAESAR_NUM			?= 1

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
	--config $(X_HEEP_CFG) \
	--cfg_peripherals $(MCU_CFG_PERIPHERALS) \
	--pads_cfg $(PAD_CFG) \
	--external_domains $(EXTERNAL_DOMAINS)
MCU_GEN_OPTS_FPGA	:= \
	--config $(X_HEEP_CFG_FPGA) \
	--cfg_peripherals $(MCU_CFG_PERIPHERALS) \
	--pads_cfg $(PAD_CFG_FPGA) \
	--external_domains $(EXTERNAL_DOMAINS)
HEEPATIA_TOP_TPL		:= $(ROOT_DIR)/hw/ip/heepatia_top.sv.tpl
PAD_RING_TPL			:= $(ROOT_DIR)/hw/ip/pad-ring/pad_ring.sv.tpl
MCU_GEN_LOCK			:= $(BUILD_DIR)/.mcu-gen.lock

# heepatia configuration
HEEPATIA_GEN_CFG	:= config/heepatia-cfg.hjson
HEEPATIA_GEN_OPTS	:= \
	--cfg $(HEEPATIA_GEN_CFG) \
	--carus_num $(CARUS_NUM) \
	--caesar_num $(CAESAR_NUM) 
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
BOOT_MODE			?= flash # jtag: wait for JTAG (DPI module), flash: boot from flash, force: load firmware into SRAM
FIRMWARE			?= $(ROOT_DIR)/build/sw/app/main.hex
FIRMWARE_FLASH 		?= $(ROOT_DIR)/build/sw/app-flash/main.hex
VCD_MODE			?= 0 # QuestaSim-only - 0: no dumo, 1: dump always active, 2: dump triggered by GPIO 0
BYPASS_FLL          ?= 0 # 0: FLL enabled, 1: FLL bypassed (TODO: make FLL work and set this to 0 by default)
MAX_CYCLES			?= 10000000
FUSESOC_FLAGS		?=
FUSESOC_ARGS		?=

# QuestaSim
FLL_FOLDER_PATH := $(ROOT_DIR)/hw/asic/fll/rtl
# ACCESSIBLE := $(shell if [ -d "$(FLL_FOLDER_PATH)" ] && [ -r "$(FLL_FOLDER_PATH)" ]; then echo true; else echo false; fi)
FUSESOC_BUILD_DIR			= $(shell find $(BUILD_DIR) -type d -name 'epfl_heepatia_heepatia_*' 2>/dev/null | sort | head -n 1)
# ifeq ($(ACCESSIBLE), true)
# 	QUESTA_SIM_DIR=$(FUSESOC_BUILD_DIR)/sim-modelsim
# else
# 	QUESTA_SIM_DIR=$(FUSESOC_BUILD_DIR)/sim-nofll-modelsim
# endif
QUESTA_SIM_DIR=$(FUSESOC_BUILD_DIR)/sim-modelsim
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

# Custom preprocessor definitions
CDEFS				?=

# Software build configuration
SW_DIR		:= sw

# Dummy target to force software rebuild
PARAMS = $(PROJECT)

# power analysis
PWR_TYPE ?= postsynth
# Conditional check to set PWR_VCD based on PWR_TYPE
ifeq ($(PWR_TYPE),postlayout)
    PWR_VCD ?= $(QUESTA_SIM_POSTLAYOUT_DIR)/logs/waves-0.vcd
else ifeq ($(PWR_TYPE),postsynth)
    PWR_VCD ?= $(QUESTA_SIM_POSTSYNTH_DIR)/logs/waves-0.vcd
else
    $(error "Unknown SIM_TYPE specified. Use 'postlayout' or 'postsynth'.")
endif

# Benchmarking configuration
THR_TESTS ?= scripts/performance-analysis/throughput-tests.txt
PWR_TESTS ?= scripts/performance-analysis/power-tests.txt

#CAESAR AND CARUS PL Netlist and SDF
CARUS_PL_SDF := $(ROOT_DIR)/hw/vendor/nm-carus-backend-opt/implementation/pnr/outputs/nm-carus/sdf/NMCarus_top_pared.sdf
CAESAR_PL_SDF := $(ROOT_DIR)/hw/vendor/nm-caesar-backend-opt/implementation/pnr/outputs/nm-caesar/sdf/NMCaesar_top_pared.sdf

#HEEPATIA PL Netlist and SDF
# HEEPATIA_PL_NET := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia_pg.v
# HEEPATIA_PL_SDF := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia.sdf

# Using post synthesis files
# Conditional check of PWR_TYPE
ifeq ($(PWR_TYPE),postlayout)
    HEEPATIA_PL_NET := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia_pg.v
	HEEPATIA_PL_SDF := $(ROOT_DIR)/build/innovus_latest/artefacts/export/heepatia.sdf
else ifeq ($(PWR_TYPE),postsynth)
	HEEPATIA_PL_NET := $(BUILD_DIR)/epfl_heepatia_heepatia_0.3.0/asic_synthesis-design_compiler/report/netlist.v
	HEEPATIA_PL_SDF := $(BUILD_DIR)/epfl_heepatia_heepatia_0.3.0/asic_synthesis-design_compiler/report/netlist.sdf
else
    $(error "Unknown SIM_TYPE specified. Use 'postlayout' or 'postsynth'.")
endif

#for power analysis
HEEPATIA_PL_NET_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia_pg_power_analysis.v
HEEPATIA_PL_SDF_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia.sdf
HEEPATIA_PL_SDF_PATCHED_PA := $(ROOT_DIR)/implementation/power_analysis/heepatia.patched.sdf

# Dependent variables
# -------------------
ifeq ($(BOOT_MODE),flash)
	FIRMWARE		:= $(FIRMWARE_FLASH)
endif

###########################
# ----- BUILD RULES ----- #
###########################

# Default alias
# -------------
.PHONY: all
all: heepatia-gen

# X-HEEP MCU system
# -----------------
# Build X-HEEP's core-v-mini-mcu
.PHONY: mcu-gen
mcu-gen: $(MCU_GEN_LOCK)
ifeq ($(TARGET), asic)
$(MCU_GEN_LOCK): $(MCU_CFG) $(PAD_CFG) $(EXT_PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else ifeq ($(TARGET), pynq-z2)
$(MCU_GEN_LOCK): $(PAD_CFG) $(EXT_PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU for PYNQ-Z2..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen X_HEEP_CFG=$(X_HEEP_CFG_FPGA) MCU_CFG_PERIPHERALS=$(MCU_CFG_PERIPHERALS)
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else ifeq ($(TARGET), zcu104)
$(MCU_GEN_LOCK): $(PAD_CFG) $(EXT_PAD_CFG) | $(BUILD_DIR)/
	@echo "### Building X-HEEP MCU for zcu104..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen X_HEEP_CFG=$(X_HEEP_CFG_FPGA) MCU_CFG_PERIPHERALS=$(MCU_CFG_PERIPHERALS)
	touch $@
	$(RM) -f $(HEEPATIA_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"
else
	$(error ### ERROR: Unsupported target implementation: $(TARGET))
endif

.PHONY: heepatia-gen-force
heepatia-gen-force:
	rm -rf build/.mcu-gen.lock build/.heepatia-gen.lock;
	$(MAKE) heepatia-gen

# Generate heepatia files
# @param TARGET=asic(default),pynq-z2,zcu104
.PHONY: heepatia-gen
heepatia-gen: $(HEEPATIA_GEN_LOCK)
$(HEEPATIA_GEN_LOCK): $(HEEPATIA_GEN_CFG) $(HEEPATIA_GEN_TPL) $(HEEPATIA_TOP_TPL) $(PAD_RING_TPL) $(MCU_GEN_LOCK) $(ROOT_DIR)/tb/tb_util.svh.tpl
ifeq ($(TARGET), asic)
	@echo "### Generating heepatia top and pad rings for ASIC..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outdir $(ROOT_DIR)/tb/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(ROOT_DIR)/tb/tb_util.svh.tpl
	@echo "### Generating heepatia files..."
else ifeq ($(TARGET), pynq-z2)
	@echo "### Generating heepatia top and padrings for PYNQ-Z2..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/tb/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(ROOT_DIR)/tb/tb_util.svh.tpl
else ifeq ($(TARGET), zcu104)
	@echo "### Generating heepatia top and padrings for zcu104..."
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(HEEPATIA_TOP_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/hw/ip/pad-ring/ \
		--external_pads $(EXT_PAD_CFG) \
		--tpl-sv $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS_FPGA) \
		--outdir $(ROOT_DIR)/tb/ \
		--external_pads $(EXT_PAD_CFG) \
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
	fusesoc run --no-export --target format epfl:heepatia:heepatia
	# fusesoc run --no-export --target lint epfl:heepatia:heepatia
	@echo "### DONE! heepatia files generated successfully"
	touch $@

# Verible format
.PHONY: format
format: $(HEEPATIA_GEN_LOCK)
	@echo "### Formatting heepatia RTL files..."
	fusesoc run --no-export --target format epfl:heepatia:heepatia

# Static analysis
.PHONY: lint
lint: $(HEEPATIA_GEN_LOCK)
	@echo "### Checking heepatia syntax and code style..."
	fusesoc run --no-export --target lint epfl:heepatia:heepatia

# Verilator RTL simulation
# ------------------------
# Build simulation model (do not launch simulation)
.PHONY: verilator-build
verilator-build: $(HEEPATIA_GEN_LOCK)
	fusesoc run --no-export --target sim --tool verilator --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS)

# Build simulation model and launch simulation
.PHONY: verilator-sim
verilator-sim: | verilator-build .verilator-check-params
	fusesoc run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

# Launch simulation
.PHONY: verilator-run
verilator-run: | .verilator-check-params
	fusesoc run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

# Launch simulation without waveform dumping
.PHONY: verilator-opt
verilator-opt: | .verilator-check-params
	fusesoc run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		--trace=false \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

# Open dumped waveform with GTKWave
.PHONY: verilator-waves
verilator-waves: $(BUILD_DIR)/sim-common/waves.fst | .check-gtkwave
	gtkwave -a tb/misc/verilator-waves.gtkw $<

# QuestaSim RTL simulation
# ------------------------
# Build simulation model
.PHONY: questasim-build
questasim-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
# ifeq ($(ACCESSIBLE), true)
	@echo "### Building simulation model with FLL..."
	$(FUSESOC) run --no-export --target sim --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_DIR) ; make opt
# else
# 	@echo "### Building simulation model with FLL behavioural model..."
# 	$(FUSESOC) run --no-export --target sim-nofll --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
# 		$(FUSESOC_ARGS)
# 	cd $(QUESTA_SIM_DIR) ; make opt
# endif

# Build simulation model and launch simulation
.PHONY: questasim-sim
questasim-sim: | questasim-build $(QUESTA_SIM_DIR)/logs/
# ifeq ($(ACCESSIBLE), true)
	fusesoc run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log
# else
# 	fusesoc run --no-export --target sim-nofll --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
# 		--firmware=$(FIRMWARE) \
# 		--bypass_fll_opt=$(BYPASS_FLL) \
# 		--boot_mode=$(BOOT_MODE) \
# 		--vcd_mode=$(VCD_MODE) \
# 		--max_cycles=$(MAX_CYCLES) \
# 		$(FUSESOC_ARGS)
# 	cat $(BUILD_DIR)/sim-common/uart.log
# endif

# Launch simulation
.PHONY: questasim-run
questasim-run: | $(QUESTA_SIM_DIR)/logs/
# ifeq ($(ACCESSIBLE), true)
	fusesoc run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log
# else
# 	fusesoc run --no-export --target sim-nofll --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
# 		--firmware=$(FIRMWARE) \
# 		--bypass_fll_opt=$(BYPASS_FLL) \
# 		--boot_mode=$(BOOT_MODE) \
# 		--vcd_mode=$(VCD_MODE) \
# 		--max_cycles=$(MAX_CYCLES) \
# 		$(FUSESOC_ARGS)
# 	cat $(BUILD_DIR)/sim-common/uart.log
# endif
	

# Launch simulation in GUI mode
.PHONY: questasim-gui
questasim-gui: | questasim-build $(QUESTA_SIM_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"

# Open dumped waveforms in GTKWave
.PHONY: questasim-waves
questasim-waves: $(SIM_VCD) | .check-gtkwave
	gtkwave -a tb/misc/questasim-waves.gtkw $<

$(BUILD_DIR)/sim-common/questa-waves.fst: $(BUILD_DIR)/sim-common/waves.vcd | .check-gtkwave
	@echo "### Converting $< to FST..."
	vcd2fst $< $@

# DPI libraries for QuestaSim
.PHONY: tb-dpi
tb-dpi: $(DPI_LIBS)
$(BUILD_DIR)/sw/sim/uartdpi.so: hw/vendor/x-heep/hw/vendor/lowrisc_opentitan/hw/dv/dpi/uartdpi/uartdpi.c | $(BUILD_DIR)/sw/sim/
	$(CC) -shared -Bsymbolic -fPIC -o $@ $< -lutil

# Post-sysnthesis and post-layout simulations
# -------------------------------------------

# QuestaSim RTL + gf22 netlist of black boxes simulation

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
questasim-postsynth-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
	fusesoc run --no-export --target sim_postsynthesis --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS);
	cd $(QUESTA_SIM_POSTSYNTH_DIR) ; make opt | tee fusesoc_questasim_postsynthesis.log

.PHONY: questasim-postsynth-run
questasim-postsynth-run:
	fusesoc run --no-export --target sim_postsynthesis --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

# Launch simulation in GUI mode
.PHONY: questasim-postsynth-gui
questasim-postsynth-gui:
	$(MAKE) -C $(QUESTA_SIM_POSTSYNTH_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"

# Questasim PostLayout Simulation (with no timing)
.PHONY: questasim-postlayout-build
questasim-postlayout-build: $(HEEPATIA_GEN_LOCK) $(DPI_LIBS)
	fusesoc run --no-export --target sim_postlayout --tool modelsim --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS);
	cd $(QUESTA_SIM_POSTLAYOUT_DIR) ; make opt | tee fusesoc_questasim_postslayout.log

.PHONY: questasim-postlayout-run
questasim-postlayout-run:
	fusesoc run --no-export --target sim_postlayout --tool modelsim --run $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		--firmware=$(FIRMWARE) \
		--bypass_fll_opt=$(BYPASS_FLL) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

# Launch simulation in GUI mode
.PHONY: questasim-postlayout-gui
questasim-postlayout-gui:
	$(MAKE) -C $(QUESTA_SIM_POSTLAYOUT_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) bypass_fll_opt=$(BYPASS_FLL) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE) max_cycles=$(MAX_CYCLES)"


# Synthesis
# ---------
# HEEperator synthesis with Synopsys DC Shell
.PHONY: synthesis
synthesis: $(HEEPATIA_GEN_LOCK)
	fusesoc run --no-export --target asic_synthesis --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia \
		$(FUSESOC_ARGS) 2>&1 | tee fusesoc_synthesis.log

implementation/pnr/inputs/heepocrates.io:
	pushd implementation/pnr/inputs/ ; ./create_io_file_from_spreadsheet.py; popd;

# Place & Route
# -------------
.PHONY: pnr_debug
pnr_debug: implementation/pnr/inputs/heepocrates.io
	pushd implementation/pnr/ ; ./run_pnr_flow.csh debug; popd;

.PHONY: pnr
pnr: implementation/pnr/inputs/heepocrates.io
	pushd implementation/pnr/ ; ./run_pnr_flow.csh; popd;


# FPGA-implementation
# -------------------
.PHONY: fpga
vivado-fpga-synth:
	@echo "### Running FPGA implementation..."
	fusesoc run --no-export --target $(TARGET) --build $(FUSESOC_FLAGS) epfl:heepatia:heepatia $(FUSESOC_ARGS)

.PHONY: prog-fpga
vivado-fpga-pgm:
	@echo "### Programming the FPGA..."
	$(MAKE) -C $(FUSESOC_BUILD_DIR)/$(TARGET)-vivado pgm

# Flash ESL programmer
.PHONY:flash-prog
flash-prog:
	@echo "### Programming the flash..."
	cd $(XHEEP_DIR)/sw/vendor/yosyshq_icestorm/iceprog && make; \
	./iceprog -d i:0x0403:0x6011 -I B $(ROOT_DIR)/$(BUILD_DIR)/sw/app-flash/main.hex;

# Benchmarks
# ----------
# Launch benchmark simulations on Verilator and generate CSV throughput report
.PHONY: benchmark-throughput
benchmark-throughput: build/performance-analysis/throughput.csv
build/performance-analysis/throughput.csv: $(THR_TESTS) | build/performance-analysis/
	@echo "### Running benchmark simulations for throughput extraction..."
	python3 scripts/performance-analysis/throughput-analysis.py \
		$(THR_TESTS) $@

# Launch benchmark simulations on post-layout netlist and generate CSV power report
.PHONY: benchmark-power
benchmark-power: build/performance-analysis/power.csv
build/performance-analysis/power.csv: $(PWR_TESTS) | build/performance-analysis/
	@echo "### Running benchmark simulations for power extraction..."
	python3 scripts/performance-analysis/power-analysis.py \
		$(PWR_TESTS) \
		build/sim-common $@

# Generate throughput benchmark chart
.PHONY: charts
charts: build/performance-analysis/power.csv build/performance-analysis/throughput.csv
	@echo "### Generating charts..."
	python3 scripts/performance-analysis/benchmark-charts.py $^ build/performance-analysis

# Power analysis
# --------------
.PHONY: patch-files-power-analysis
patch-files-power-analysis: $(BUILD_DIR)/.patch-files-power-analysis.lock
# TODO: is bellow correct ?
# $(BUILD_DIR)/.patch-files-power-analysis.lock: $(HEEPATIA_PL_NET) $(HEEPATIA_PL_SDF).gz 
$(BUILD_DIR)/.patch-files-power-analysis.lock: $(HEEPATIA_PL_NET) $(HEEPATIA_PL_SDF)
#   the LIB and LEF of the FLL are wrong as the VDDA power pin is missing, thus deleting it so that power analysis can be done
	cp $(HEEPATIA_PL_NET) $(HEEPATIA_PL_NET_PA)
# sed -i '/.VDDA(VDD)/d' $(HEEPATIA_PL_NET_PA)
	touch $(BUILD_DIR)/.patch-files-power-analysis.lock

.PHONY: power-analysis
power-analysis: $(BUILD_DIR)/.patch-files-power-analysis.lock $(PWR_VCD)
	@echo "### Running power analysis..."
	rm -rf implementation/power_analysis/reports/*
	pushd implementation/power_analysis/; ./run_pwr_flow.sh $(PWR_VCD) $(HEEPATIA_PL_NET_PA) $(HEEPATIA_PL_SDF) heepatia_top; popd;
# pushd implementation/power_analysis/; ./run_pwr_flow.sh $(PWR_VCD) $(HEEPATIA_PL_NET_PA) $(HEEPATIA_PL_SDF).gz heepatia_top; popd;

# Software
# --------
# heepatia applications
.PHONY: app
app: $(HEEPATIA_GEN_LOCK) | carus-sw $(BUILD_DIR)/sw/app/ $(BUILD_DIR)/sw/app-flash/
ifneq ($(APP_MAKE),)
	$(MAKE) -C $(dir $(APP_MAKE))
endif
	@echo "### Building application for SRAM execution..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) $(MAKECMDGOALS)
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app/ \;
	@echo "### Building application for flash load..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) LINKER=flash_load $(MAKECMDGOALS)
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app-flash/ \;

# NM-Carus kernels and startup code
.PHONY: carus-sw
carus-sw:
	$(MAKE) -C $(SW_DIR) carus

# Dummy target to force software rebuild
$(PARAMS):
	@echo "### Rebuilding software..."

# Utilities
# ---------
# Update vendored IPs
.PHONY: vendor-update
vendor-update:
	@echo "### Updating vendored IPs..."
	find hw/vendor -maxdepth 1 -type f -name "*.vendor.hjson" -exec python3 util/vendor.py -vU '{}' \;
	$(MAKE) clean-lock
	$(MAKE) heepatia-gen

# Check if fusesoc is available
.PHONY: .check-fusesoc
.check-fusesoc:
	@if [ ! `which fusesoc` ]; then \
	printf -- "### ERROR: 'fusesoc' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

# Check if GTKWave is available
.PHONY: .check-gtkwave
.check-gtkwave:
	@if [ ! `which gtkwave` ]; then \
	printf -- "### ERROR: 'gtkwave' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

# Check simulation parameters
.PHONY: .verilator-check-params
.verilator-check-params:
	@if [ "$(BOOT_MODE)" = "flash" ]; then \
		echo "### ERROR: Verilator simulation with flash boot is not supported" >&2; \
		exit 1; \
	fi

# # Run PHEE-Coprosit tests
# .PHONY: test
# test:
# 	$(RM) test/*.log
# 	./test/test_all.sh

# Create directories
%/:
	mkdir -p $@

# Clean build directory
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
	$(RM) sw/device/include/heepatia.h
	$(RM) sw/device/include/heepatia_ctrl_reg.h
	$(RM) -r $(BUILD_DIR)
	$(MAKE) -C $(HEEP_DIR) clean-all
	$(MAKE) -C $(SW_DIR) clean
clean-lock:
	$(RM) $(BUILD_DIR)/.*.lock

# Print variables
.PHONY: .print
.print:
	@echo "APP_MAKE: $(APP_MAKE)"
	@echo "KERNEL_PARAMS: $(KERNEL_PARAMS)"
	@echo "FUSESOC_ARGS: $(FUSESOC_ARGS)"

####################################
# ----- INCLUDE X-HEEP RULES ----- #
####################################
export X_HEEP_CFG
export MCU_CFG_PERIPHERALS
export PAD_CFG
export EXT_PAD_CFG
export EXTERNAL_DOMAINS
export HEEP_DIR = $(ROOT_DIR)/hw/vendor/x-heep
XHEEP_MAKE 		= $(HEEP_DIR)/external.mk
include $(XHEEP_MAKE)
