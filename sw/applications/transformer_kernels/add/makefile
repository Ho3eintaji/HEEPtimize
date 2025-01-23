# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Hossein Taji
# Date: 23/01/2025

#############################
# ----- CONFIGURATION ----- #
#############################

# CC = gcc
# CFLAGS = -I. -I../common -O2

# KERNEL_PARAMS 		?= int32 --row_a 8 --col_a 8 --col_b 256
PARAM_SEQ_LEN ?= 120
PARAM_INPUT_DIM ?= 16

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../../nmc python3 datagen.py --seq_len $(PARAM_SEQ_LEN) --input_dim $(PARAM_INPUT_DIM)
# PYTHONPATH=../../nmc python3 datagen.py $(KERNEL_PARAMS)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h 
