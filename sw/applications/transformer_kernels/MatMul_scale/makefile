
#############################
# ----- CONFIGURATION ----- #
#############################

PARAM_MAT_SIZE ?= 64
PARAM_SHIFT_SCALE ?= 2

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../../nmc python3 datagen.py --mat_size $(PARAM_MAT_SIZE) --shift_scale $(PARAM_SHIFT_SCALE)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h