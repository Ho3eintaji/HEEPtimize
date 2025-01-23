
#############################
# ----- CONFIGURATION ----- #
#############################

PARAM_SEQ_LEN ?= 32
PARAM_HEAD_HIDDEN_SIZE ?= 64
PARAM_NUM_HEADS ?= 4

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=../../../nmc python3 datagen.py --seq_len $(PARAM_SEQ_LEN) --head_hidden_size $(PARAM_HEAD_HIDDEN_SIZE) --num_heads $(PARAM_NUM_HEADS)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h
