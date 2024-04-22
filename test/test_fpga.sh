#!/bin/bash

echo "=================================="
echo "Test bitstream complete on PYNQ-Z2"
echo "=================================="
date
echo ""

make fpga FUSESOC_FLAGS="--flag=use_posit16 --flag=use_quire"
