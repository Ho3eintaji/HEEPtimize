#!/bin/bash

echo "========================"
echo "Test PHEE8 on Questasim"
echo "========================"
date
echo ""

make app RISCV=$HOME/shares/tools/rv32gcxposit PROJECT=posit8_testsuite COMPILER=clang ARCH=rv32gcxposit1 TARGET=sim

make questasim-build FUSESOC_FLAGS="--flag=use_posit8 --flag=use_quire"

make questasim-run BOOT_MODE=force MAX_CYCLES=5000000
make questasim-run BOOT_MODE=flash MAX_CYCLES=5000000

echo "========================"
echo "Test PHEE16 on Questasim"
echo "========================"
date
echo ""

make app RISCV=$HOME/shares/tools/rv32gcxposit PROJECT=posit16_testsuite COMPILER=clang ARCH=rv32gcxposit1 TARGET=sim

make questasim-build FUSESOC_FLAGS="--flag=use_posit16 --flag=use_quire"

make questasim-run BOOT_MODE=force MAX_CYCLES=5000000
make questasim-run BOOT_MODE=flash MAX_CYCLES=5000000

echo "========================"
echo "Test PHEE32 on Questasim"
echo "========================"
date
echo ""

make app RISCV=$HOME/shares/tools/rv32gcxposit PROJECT=posit32_testsuite COMPILER=clang ARCH=rv32gcxposit1 TARGET=sim

make questasim-build FUSESOC_FLAGS="--flag=use_posit32 --flag=use_quire"

make questasim-run BOOT_MODE=force MAX_CYCLES=5000000
make questasim-run BOOT_MODE=flash MAX_CYCLES=5000000
