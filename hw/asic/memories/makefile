# Copyright 2024 EPFL and Universidad Complutense de Madrid
#
# Author: David Mallasén
# Date: 24/04/2024
# Description: makefile to interact with the memory compilers to
#     generate the memories.

# NOTE: based on the same file from HEEPerator

#############################
# ----- CONFIGURATION ----- #
#############################

# Project root directory
ROOT 		:= $(realpath ../../..)
MEM_DIR 	:= $(realpath .)

# Memory compiler executable
SRAM_COMPILER	:= /dkits/arm/tsmc/cln16fcll001/sram_sp_hde_svt_mvt/r4p1/bin/sram_sp_hde_svt_mvt
RF_COMPILER		:= /dkits/arm/tsmc/cln16fcll001/rf_sp_hde_svt_mvt_sga1/r1p2/bin/rf_sp_hde_svt_mvt_sga1
LIB2DB_SCRIPT	:= mem_lib2db.tcl

# Memory compiler configuration
NUM_ROWS		?= 2048
NUM_BITS		?= 32
MUX 			?= 8
FREQUENCY 		?= 250
CORNERS 		:= ffgnp_0p88v_0p88v_125c,ffgnp_0p88v_0p88v_m40c,ssgnp_0p675v_0p675v_125c,ssgnp_0p675v_0p675v_m40c,tt_0p80v_0p80v_25c
SRAM_INST_NAME 	:= sram$(NUM_ROWS)x$(NUM_BITS)m$(MUX)
RF_INST_NAME 	:= rf$(NUM_ROWS)x$(NUM_BITS)m$(MUX)
LIB_NAME		:= arm_mem_$(NUM_ROWS)x$(NUM_BITS)m$(MUX)
COMMON_OPT		:= \
	-name_case upper \
	-libname $(LIB_NAME) \
	-mvt LP \
	-bus_notation on \
	-ser none \
	-site_def off \
	-check_instname on \
	-frequency $(FREQUENCY) \
	-bmux on \
	-diodes on \
	-PG_PINS_domain VDDPE \
	-activity_factor 50 \
	-words $(NUM_ROWS) \
	-drive 6 \
	-bits $(NUM_BITS) \
	-retention on \
	-write_mask on \
	-atf off \
	-left_bus_delim "[" \
	-pwr_gnd_rename vddpe:VDDPE,vddce:VDDCE,vsse:VSSE \
	-right_bus_delim "]" \
	-rows_p_bl 256 \
	-wp_size 1 \
	-lren_bankmask off \
	-metal_stack 2Xa1Xd \
	-cust_comment "" \
	-write_thru off \
	-pipeline off \
	-prefix "" \
	-power_gating off \
	-ema on \
	-vmin_assist off \
	-mux $(MUX) \
	-corners $(CORNERS)
SRAM_COMMON_OPT	:= \
	-instname $(SRAM_INST_NAME) \
	-rcols 2 \
	-rrows 0 \
	-flexible_banking 4 \
	-redundancy on \
	$(COMMON_OPT)
RF_COMMON_OPT	:= \
	-instname $(RF_INST_NAME) \
	-flexible_banking 1 \
	-redundancy off \
	$(COMMON_OPT)

# Directory configuration
SRAM_OUT_DIR	:= $(SRAM_INST_NAME)
RF_OUT_DIR		:= $(RF_INST_NAME)
LIB_DIR			:= lib
DB_DIR			:= db
LEF_DIR			:= lef
GDS_DIR			:= gds
CDL_DIR 		:= cdl
VLOG_DIR		:= vlog
DOC_DIR			:= doc

# Library compiler
TOOL_INIT_SCRIPT	:= $(ROOT)/$(TOOL_INIT_SCRIPT)
BUILD_DIR			:= $(ROOT)/build/memories
SRAM_LIB_CACHE 		:= $(BUILD_DIR)/.$(SRAM_INST_NAME)-lib-compiled.cache
RF_LIB_CACHE 		:= $(BUILD_DIR)/.$(RF_INST_NAME)-lib-compiled.cache
LC_SRAM_DIR 		:= $(MEM_DIR)/$(SRAM_OUT_DIR)
LC_RF_DIR 			:= $(MEM_DIR)/$(RF_OUT_DIR)

# Output files
COMMA_CHAR 		:= ,
CORNER_NAMES	:= $(subst $(COMMA_CHAR), , $(CORNERS))

SRAM_NLDM_FILES	:= $(patsubst %, $(SRAM_OUT_DIR)/$(LIB_DIR)/$(SRAM_INST_NAME)_%.lib, $(CORNER_NAMES))
SRAM_CCS_FILES 	+= $(patsubst %, $(SRAM_OUT_DIR)/$(LIB_DIR)/$(SRAM_INST_NAME)_%.lib_ccs_tn, $(CORNER_NAMES))
SRAM_DB_FILES 	+= $(patsubst %, $(SRAM_OUT_DIR)/$(DB_DIR)/$(SRAM_INST_NAME)_%.db, $(CORNER_NAMES))
SRAM_LEF_FILES	:= $(SRAM_OUT_DIR)/$(LEF_DIR)/$(SRAM_INST_NAME)_antenna.clf \
				   $(SRAM_OUT_DIR)/$(LEF_DIR)/$(SRAM_INST_NAME).lef
SRAM_GDS_FILES	:= $(SRAM_OUT_DIR)/$(GDS_DIR)/$(SRAM_INST_NAME).gds2
SRAM_CDL_FILES	:= $(SRAM_OUT_DIR)/$(CDL_DIR)/$(SRAM_INST_NAME).cdl
SRAM_VLOG_FILES	:= $(SRAM_OUT_DIR)/$(VLOG_DIR)/$(SRAM_INST_NAME).v
SRAM_PS_FILES	:= $(patsubst %, $(SRAM_OUT_DIR)/$(DOC_DIR)/$(SRAM_INST_NAME)_%.ps, $(CORNER_NAMES))
SRAM_PDF_FILES	:= $(patsubst %, $(SRAM_OUT_DIR)/$(DOC_DIR)/$(SRAM_INST_NAME)_%.pdf, $(CORNER_NAMES))

RF_NLDM_FILES	:= $(patsubst %, $(RF_OUT_DIR)/$(LIB_DIR)/$(RF_INST_NAME)_%.lib, $(CORNER_NAMES))
RF_DB_FILES 	+= $(patsubst %, $(RF_OUT_DIR)/$(DB_DIR)/$(RF_INST_NAME)_%.db, $(CORNER_NAMES))
RF_LEF_FILES	:= $(RF_OUT_DIR)/$(LEF_DIR)/$(RF_INST_NAME)_antenna.clf \
				   $(RF_OUT_DIR)/$(LEF_DIR)/$(RF_INST_NAME).lef
RF_GDS_FILES	:= $(RF_OUT_DIR)/$(GDS_DIR)/$(RF_INST_NAME).gds2
RF_CDL_FILES	:= $(RF_OUT_DIR)/$(CDL_DIR)/$(RF_INST_NAME).cdl
RF_VLOG_FILES	:= $(RF_OUT_DIR)/$(VLOG_DIR)/$(RF_INST_NAME).v
RF_PS_FILES	:= $(patsubst %, $(RF_OUT_DIR)/$(DOC_DIR)/$(RF_INST_NAME)_%.ps, $(CORNER_NAMES))
RF_PDF_FILES	:= $(patsubst %, $(RF_OUT_DIR)/$(DOC_DIR)/$(RF_INST_NAME)_%.pdf, $(CORNER_NAMES))

#######################
# ----- TARGETS ----- #
#######################

# SRAM target (default)
.PHONY: sram
sram: $(SRAM_DB_FILES) $(SRAM_LEF_FILES) $(SRAM_GDS_FILES) $(SRAM_CDL_FILES) $(SRAM_VLOG_FILES) $(SRAM_PDF_FILES)
$(SRAM_DB_FILES): sram-db
$(SRAM_LEF_FILES): sram-lef
$(SRAM_GDS_FILES): sram-gds
$(SRAM_CDL_FILES): sram-cdl
$(SRAM_VLOG_FILES): sram-vlog

# Register file targets
.PHONY: rf
rf: $(RF_DB_FILES) $(RF_LEF_FILES) $(RF_GDS_FILES) $(RF_CDL_FILES) $(RF_VLOG_FILES) $(RF_PDF_FILES)
$(RF_DB_FILES): rf-db
$(RF_LEF_FILES): rf-lef
$(RF_GDS_FILES): rf-gds
$(RF_CDL_FILES): rf-cdl
$(RF_VLOG_FILES): rf-vlog

# SRAM targets
# ------------
.PHONY: sram-lib sram-ccs sram-lef sram-gds sram-cdl sram-vlog sram-doc
$(SRAM_LIB_CACHE): sram-lib
	touch $@
sram-lib: | $(SRAM_OUT_DIR)/$(LIB_DIR)/
	@echo "Generating liberty NLDM files..."
	cd $(SRAM_OUT_DIR)/$(LIB_DIR) && \
	$(SRAM_COMPILER) liberty -libertyviewstyle nldm $(SRAM_COMMON_OPT)

sram-db: | $(SRAM_LIB_CACHE) .check-lc $(SRAM_OUT_DIR)/$(DB_DIR)/ $(BUILD_DIR)/
	cd $(BUILD_DIR) && \
	export MEMORIES_LIB_DIR=$(LC_SRAM_DIR)/$(LIB_DIR) && \
	export MEMORIES_DB_DIR=$(LC_SRAM_DIR)/$(DB_DIR) && \
	lc_shell -batch -f ../../hw/asic/memories/$(LIB2DB_SCRIPT)
	$(RM) -r $(BUILD_DIR)/*.log $(BUILD_DIR)/*.txt

sram-lef: | $(SRAM_OUT_DIR)/$(LEF_DIR)/
	@echo "Generating LEF files..."
	cd $(SRAM_OUT_DIR)/$(LEF_DIR) && \
	$(SRAM_COMPILER) lef-fp $(SRAM_COMMON_OPT)

sram-gds: | $(SRAM_OUT_DIR)/$(GDS_DIR)/
	@echo "Generating GDS2 files..."
	cd $(SRAM_OUT_DIR)/$(GDS_DIR) && \
	$(SRAM_COMPILER) gds2 $(SRAM_COMMON_OPT)

sram-cdl: | $(SRAM_OUT_DIR)/$(CDL_DIR)/
	@echo "Generating CDL files..."
	cd $(SRAM_OUT_DIR)/$(CDL_DIR) && \
	$(SRAM_COMPILER) lvs $(SRAM_COMMON_OPT)

sram-vlog: | $(SRAM_OUT_DIR)/$(VLOG_DIR)/
	@echo "Generating Verilog files..."
	cd $(SRAM_OUT_DIR)/$(VLOG_DIR) && \
	$(SRAM_COMPILER) verilog $(SRAM_COMMON_OPT)

$(SRAM_PDF_FILES): %.pdf: %.ps
	@echo "Generating documentation..."
	ps2pdf $< $@
$(SRAM_PS_FILES): %.ps: sram-doc
	:
sram-doc: | $(SRAM_OUT_DIR)/$(DOC_DIR)/
	cd $(SRAM_OUT_DIR)/$(DOC_DIR) && \
	$(SRAM_COMPILER) postscript $(SRAM_COMMON_OPT)

# Register file targets
# ---------------------
.PHONY: rf-lib rf-lef rf-gds rf-cdl rf-vlog rf-doc
$(RF_LIB_CACHE): rf-lib
	touch $@
rf-lib: | $(RF_OUT_DIR)/$(LIB_DIR)/
	@echo "Generating liberty NLDM files..."
	cd $(RF_OUT_DIR)/$(LIB_DIR) && \
	$(RF_COMPILER) liberty -libertyviewstyle nldm $(RF_COMMON_OPT)

rf-db: | $(RF_LIB_CACHE) .check-lc $(RF_OUT_DIR)/$(DB_DIR)/ $(BUILD_DIR)/
	cd $(BUILD_DIR) && \
	export MEMORIES_LIB_DIR=$(LC_RF_DIR)/$(LIB_DIR) && \
	export MEMORIES_DB_DIR=$(LC_RF_DIR)/$(DB_DIR) && \
	lc_shell -batch -f ../../hw/asic/memories/$(LIB2DB_SCRIPT)

rf-lef: | $(RF_OUT_DIR)/$(LEF_DIR)/
	@echo "Generating LEF files..."
	cd $(RF_OUT_DIR)/$(LEF_DIR) && \
	$(RF_COMPILER) lef-fp $(RF_COMMON_OPT)

rf-gds: | $(RF_OUT_DIR)/$(GDS_DIR)/
	@echo "Generating GDS2 files..."
	cd $(RF_OUT_DIR)/$(GDS_DIR) && \
	$(RF_COMPILER) gds2 $(RF_COMMON_OPT)

rf-cdl: | $(RF_OUT_DIR)/$(CDL_DIR)/
	@echo "Generating CDL files..."
	cd $(RF_OUT_DIR)/$(CDL_DIR) && \
	$(RF_COMPILER) lvs $(RF_COMMON_OPT)

rf-vlog: | $(RF_OUT_DIR)/$(VLOG_DIR)/
	@echo "Generating Verilog files..."
	cd $(RF_OUT_DIR)/$(VLOG_DIR) && \
	$(RF_COMPILER) verilog $(RF_COMMON_OPT)

$(RF_PDF_FILES): %.pdf: %.ps
	ps2pdf $< $@
$(RF_PS_FILES): %.ps: rf-doc
	:
rf-doc: | $(RF_OUT_DIR)/$(DOC_DIR)/
	@echo "Generating documentation..."
	cd $(RF_OUT_DIR)/$(DOC_DIR) && \
	$(RF_COMPILER) postscript $(RF_COMMON_OPT)

# Utilities
# ---------
# Output directories
%/:
	@mkdir -p $@

# Clean build
.PHONY: clean
clean:
	$(RM) -r $(SRAM_OUT_DIR)
	$(RM) -r $(RF_OUT_DIR)
	$(RM) -r $(BUILD_DIR)

.PHONY: .check-lc
.check-lc:
	@if [ `which lc_shell &> /dev/null` ]; then \
	printf -- "### ERROR: 'lc_shell' is not in PATH.\n" >&2; \
	exit 1; fi

.PHONY: .test
.test:
	@echo $(CORNER_NAMES)
	@echo $(SRAM_NLDM_FILES)
	@echo $(LC_SRAM_DIR)
