#!/bin/bash

# Some colors are defined to help debugging in the console.
WHITE="\033[37;1m "
RED="\033[31;1m "
GREEN="\033[32;1m "
WARNING="\033[33;1m "
RESET="\033[0m"

# Some delimiters are defined to assist the process of detecting each app in the console.
LONG_G="${GREEN}================================================================================${RESET}"
LONG_R="${RED}================================================================================${RESET}"
LONG_W="${WHITE}================================================================================${RESET}"
LONG_WAR="${WARNING}================================================================================${RESET}"


# =============
# Initial setup
# =============
echo "============="
echo "Test HEEPatia"
echo "============="
date
echo ""

echo -n "Running heeperator-gen-force..."
make heeperator-gen-force > test/heeperator-gen-force.log 2>&1
echo "DONE. Output in test/heeperator-gen-force.log"
echo ""

# =========
# Verilator
# =========
echo "==========================="
echo "Running all Verilator tests"
echo "==========================="
date
echo ""

./test/test_verilator.sh > test/test_verilator.log 2>&1

# Check verilator results
if grep -Fxq "hello world!" test/test_verilator.log
then
    echo -e ${LONG_G}
    echo -e "${GREEN}Successfully simulated hello_world using Verilator${RESET}"
    echo -e ${LONG_G}
else
    echo -e ${LONG_R}
    echo -e "${RED}Error when simulating hello_world using Verilator${RESET}"
    echo -e "${RED}Check test/test_verilator.log for further information${RESET}"
    echo -e ${LONG_R}
fi

echo "============================"
echo "Finished all Verilator tests"
echo "============================"
date
echo ""

# =========
# Questasim
# =========
echo "==========================="
echo "Running all Questasim tests"
echo "==========================="
date
echo ""

./test/test_questasim.sh > test/test_questasim.log 2>&1

# Check questasim results
test_ok_count=$(grep -o 'test OK' test/test_questasim.log | wc -l)
if [ $test_ok_count -eq 120 ]; then
    echo -e ${LONG_G}
    echo -e "${GREEN}Successfully simulated all tests using Questasim${RESET}"
    echo -e ${LONG_G}
else
    echo -e ${LONG_R}
    echo -e "${RED}Error when simulating tests using Questasim${RESET}"
    echo -e "${RED}Check test/test_questasim.log for further information${RESET}"
    echo -e ${LONG_R}
fi

echo "============================"
echo "Finished all Questasim tests"
echo "============================"
date
echo ""

# ==============
# FPGA bitstream
# ==============
echo -n "Running heeperator-gen-force for the pynq-z2..."
make heeperator-gen-force TARGET=pynq-z2 > test/heeperator-gen-force_fpga.log 2>&1
echo "DONE. Output in test/heeperator-gen-force_fpga.log"
echo ""

echo "======================"
echo "Running all FPGA tests"
echo "======================"
date
echo ""

./test/test_fpga.sh > test/test_fpga.log 2>&1

# Check FPGA results
if grep -Fxq "Bitstream generation completed" test/test_fpga.log && ! tail -n 10 test/test_fpga.log | grep -Fxq "Error"; then
    echo -e ${LONG_G}
    echo -e "${GREEN}Successfully built pynq-z2 bitstream${RESET}"
    echo -e ${LONG_G}
else
    echo -e ${LONG_R}
    echo -e "${RED}Error when building the pynq-z2 bitstream${RESET}"
    echo -e "${RED}Check test/test_fpga.log for further information${RESET}"
    echo -e ${LONG_R}
fi

echo "======================="
echo "Finished all FPGA tests"
echo "======================="
date
echo ""

echo "=================="
echo "Finished all tests"
echo "=================="
date
echo ""
