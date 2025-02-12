# File: makefile
# Author: Francesco Poluzzi
# Date: 04/12/2024
# Description: Makefile for the batch normalization application

#############################
# ----- CONFIGURATION ----- #
#############################

KERNEL_PARAMS 		?= int32 --vector_num 28 --vector_len 200 --decimal_bits 12 --seed 8 --fxp False

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../nmc python3 datagen.py $(KERNEL_PARAMS)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h
