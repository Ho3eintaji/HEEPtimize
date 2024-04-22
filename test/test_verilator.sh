#!/bin/bash

echo "============================="
echo "Test Hello World on Verilator"
echo "============================="
date
echo ""

make app PROJECT=hello_world

make verilator-build
make verilator-opt
