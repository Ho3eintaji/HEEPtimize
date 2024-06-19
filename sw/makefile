# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Michele Caon
# Date: 22/07/2023
# Description: Makefile for NM-Carus computing kernels

#############################
# ----- CONFIGURATION ----- #
#############################

# Global configuration
ROOT_DIR				:= $(realpath ..)
BUILD_DIR				:= $(ROOT_DIR)/build/sw
CARUS_BUILD_DIR			:= $(BUILD_DIR)/carus

# NM-Carus startup code header
CARUS_INIT_HEADER_DIR	:= external/lib/drivers/carus
CARUS_INIT_SRC			:= external/lib/drivers/carus/init.S
CARUS_INIT_DST			:= $(CARUS_INIT_SRC:external/lib/drivers/carus/%=$(CARUS_BUILD_DIR)/%)
CARUS_INIT_ELF			:= $(CARUS_INIT_DST:.S=.elf)
CARUS_INIT_BIN			:= $(CARUS_INIT_DST:.S=.bin)
CARUS_INIT_HEADER		:= $(CARUS_INIT_HEADER_DIR)/carus_loader.h

# NM-Carus kernel source files
CARUS_KERNEL_SRC_DIR	:= nmc/kernels/carus
CARUS_KERNEL_OUT_DIR	?= external/carus/kernels
CARUS_KERNEL_SRCS_ASM	:= $(wildcard $(CARUS_KERNEL_SRC_DIR)/*.S)
CARUS_KERNEL_SRCS_C		:= $(wildcard $(CARUS_KERNEL_SRC_DIR)/*.c)
CARUS_KERNEL_SRCS		:= $(CARUS_KERNEL_SRCS_ASM) $(CARUS_KERNEL_SRCS_C)
CARUS_KERNEL_DSTS_ASM	:= $(CARUS_KERNEL_SRCS_ASM:$(CARUS_KERNEL_SRC_DIR)/%=$(CARUS_BUILD_DIR)/%)
CARUS_KERNEL_DSTS_C		:= $(CARUS_KERNEL_SRCS_C:$(CARUS_KERNEL_SRC_DIR)/%=$(CARUS_BUILD_DIR)/%)
CARUS_KERNEL_DSTS		:= $(CARUS_KERNEL_DSTS_ASM) $(CARUS_KERNEL_DSTS_C)
CARUS_KERNEL_ELFS_ASM	:= $(CARUS_KERNEL_DSTS_ASM:.S=.elf)
CARUS_KERNEL_ELFS_C		:= $(CARUS_KERNEL_DSTS_C:.c=.elf)
CARUS_KERNEL_ELFS		:= $(CARUS_KERNEL_ELFS_ASM) $(CARUS_KERNEL_ELFS_C)
CARUS_KERNEL_BINS		:= $(CARUS_KERNEL_ELFS:.elf=.bin)
CARUS_KERNEL_HEADERS	:= $(CARUS_KERNEL_BINS:.bin=.h)
CARUS_KERNEL_OUTS_ASM	:= $(CARUS_KERNEL_DSTS_ASM:$(CARUS_BUILD_DIR)/%.S=$(CARUS_KERNEL_OUT_DIR)/%.h)
CARUS_KERNEL_OUTS_C		:= $(CARUS_KERNEL_DSTS_C:$(CARUS_BUILD_DIR)/%.c=$(CARUS_KERNEL_OUT_DIR)/%.h)
CARUS_KERNEL_OUTS  		:= $(CARUS_KERNEL_HEADERS:$(CARUS_BUILD_DIR)/%=$(CARUS_KERNEL_OUT_DIR)/%)

# Toolchain configuration
RVV					?= /shares/eslfiler1/common/esl/HEEPerator/IP/carus_compiler_new
RVV_PREFIX			:= $(RVV)/bin/riscv32-unknown-elf-
RVV_CFLAGS			:= \
	-march=rv32eczve32x_zicsr \
	-mabi=ilp32e \
	-ffreestanding \
	-O2 \
	-Wall
RVV_LDFLAGS			:= \
	-T $(ROOT_DIR)/sw/external/carus/linker/emem.ld \
	-nostdlib \
	-nostartfiles \
	-static \
	-Wl,--gc-sections
RVV_INCLUDES		:= \
	-I $(ROOT_DIR)/sw/external/carus/include \
	-I $(ROOT_DIR)/sw/external/carus/device/include
RVV_DEP_FLAGS		= -MT $@ -MMD -MP -MF $*.d

###########################
# ----- BUILD RULES ----- #
###########################

# Default alias
# -------------
.PHONY: all
all: carus-init carus-kernels

.PHONY: carus
carus: carus-init carus-kernels

# Build NM-Carus startup code
# ---------------------------
.PHONY: carus-init
carus-init: $(CARUS_INIT_HEADER)

# Gnerate header
$(CARUS_INIT_HEADER): $(CARUS_INIT_BIN)
	@echo "## Generating NM-Carus startup code header $@..."
	python3 $(ROOT_DIR)/sw/nmc/c_gen.py $@ $^

# Extract startup code binary
$(CARUS_INIT_BIN): %.bin: %.elf
	@echo "## Extracting NM-Carus startup code binary $@..."
	$(RVV_PREFIX)objcopy -O binary $< $@

# Compile startup code
$(CARUS_INIT_ELF): %.elf: %.S
	@echo "## Assembling NM-Carus startup code $@..."
	$(RVV_PREFIX)gcc $(RVV_DEP_FLAGS) $(RVV_CFLAGS) $(RVV_LDFLAGS) -o $@ $< $(RVV_INCLUDES)
	@echo "## Disassembling NM-Carus startup code $@..."
	$(RVV_PREFIX)objdump -M numeric -j .init -D $@ > $(@:.elf=.dis)

# Copy source files
$(CARUS_INIT_DST): $(CARUS_BUILD_DIR)/%: $(CARUS_INIT_SRC) | $(CARUS_BUILD_DIR)/
	@echo "## Copying NM-Carus startup code source file $@..."
	cp $< $@

# Build kernel headers
# --------------------
.PHONY: carus-kernels
carus-kernels: $(CARUS_KERNEL_OUTS)

# Generate header
$(CARUS_KERNEL_OUTS_ASM): $(CARUS_KERNEL_OUT_DIR)/%.h: $(CARUS_BUILD_DIR)/%.bin $(CARUS_BUILD_DIR)/%.S | $(CARUS_KERNEL_OUT_DIR)/
	@echo "## Generating NM-Carus kernel header $@..."
	python3 $(ROOT_DIR)/sw/nmc/c_gen.py $@ $^
$(CARUS_KERNEL_OUTS_C): $(CARUS_KERNEL_OUT_DIR)/%.h: $(CARUS_BUILD_DIR)/%.bin $(CARUS_BUILD_DIR)/%.c
	@echo "## Generating NM-Carus kernel header $@..."
	python3 $(ROOT_DIR)/sw/nmc/c_gen.py $@ $^

# Extract kernel binary and disassemble it
$(CARUS_KERNEL_BINS): %.bin: %.elf
	@echo "## Extracting NM-Carus kernel binary $@..."
	$(RVV_PREFIX)objcopy -O binary $< $@

# Compile kernel
$(CARUS_KERNEL_ELFS_ASM): %.elf: %.S
	@echo "## Assembling NM-Carus kernel $@..."
	$(RVV_PREFIX)gcc $(RVV_DEP_FLAGS) $(RVV_CFLAGS) $(RVV_LDFLAGS) -o $@ $< $(RVV_INCLUDES)
	@echo "## Disassembling NM-Carus kernel $@..."
	$(RVV_PREFIX)objdump -M numeric -j .text -j .data -D $@ > $(@:.elf=.dis)
$(CARUS_KERNEL_ELFS_C): %.elf: %.c
	@echo "## Compiling NM-Carus kernel $@..."
	$(RVV_PREFIX)gcc $(RVV_DEP_FLAGS) $(RVV_CFLAGS) $(RVV_LDFLAGS) -o $@ $< $(RVV_INCLUDES)
	@echo "## Disassembling NM-Carus kernel $@..."
	$(RVV_PREFIX)objdump -M numeric -j .text -j .data -D $@ > $(@:.elf=.dis)

# Copy source files
$(CARUS_KERNEL_DSTS): $(CARUS_BUILD_DIR)/%: $(CARUS_KERNEL_SRC_DIR)/% | $(CARUS_BUILD_DIR)/
	@echo "## Copying NM-Carus kernel source file $@..."
	cp $< $@

# Utilities
# ---------
# Create directories
%/: 
	mkdir -p $@

# Clean
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) -r $(CARUS_KERNEL_OUT_DIR)
	$(RM) $(CARUS_KERNEL_HEADERS)
	$(RM) $(CARUS_INIT_HEADER)

.PHONY: .print
.print:
	@echo "ROOT_DIR:             $(ROOT_DIR)"
	@echo "CARUS_BUILD_DIR:      $(CARUS_BUILD_DIR)"
	@echo "CARUS_KERNEL_SRCS:    $(CARUS_KERNEL_SRCS)"
	@echo "CARUS_KERNEL_DSTS:    $(CARUS_KERNEL_DSTS)"
	@echo "CARUS_KERNEL_HEADERS: $(CARUS_KERNEL_HEADERS)"
	@echo "CARUS_KERNEL_BINS:    $(CARUS_KERNEL_BINS)"
	@echo "CARUS_KERNEL_ELFS:    $(CARUS_KERNEL_ELFS)"
	@echo "CARUS_KERNEL_OUTS:    $(CARUS_KERNEL_OUTS)"