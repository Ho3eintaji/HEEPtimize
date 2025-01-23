
#############################
# ----- CONFIGURATION ----- #
#############################

PARAM_DATA_SIZE ?= 64

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../../nmc python3 datagen.py --data_size $(PARAM_DATA_SIZE) 

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h