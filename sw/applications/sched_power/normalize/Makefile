
# File: makefile
# Author: Francesco Poluzzi
# Date: 04/12/2024
# Description: Makefile for the power normalization application

#############################
# ----- CONFIGURATION ----- #
#############################

POWER_TARGET       ?= carus # carus or cpu
KERNEL_PARAMS      ?= int16 --input_dim 400 --seq_len 28 --decimal_bits 12 --seed 8 --powertarget $(POWER_TARGET)

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../../nmc python3 datagen.py $(KERNEL_PARAMS)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h