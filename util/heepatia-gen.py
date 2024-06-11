#!/usr/bin/env python3

# Copyright 2022 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: heepatia-gen.py
# Author: Michele Caon, Luigi Giuffrida
# Date: 30/04/2024
# Description: Generate heepatia HDL files based on configuration.

# Based on heepocrates_gen.py (https://eslgit.epfl.ch/heep/HEEPpocrates/-/blob/main/util/heepocrates_gen.py), which is based on occamygen.py from ETH Zurich (https://github.com/pulp-platform/snitch/blob/master/util/occamygen.py)

import argparse
import pathlib
import re
import sys
import logging
import math

import hjson
from jsonref import JsonRef
from mako.template import Template

# Compile a regex to trim trailing whitespaces on lines
re_trailws = re.compile(r"[ \t\r]+$", re.MULTILINE)


def int2hexstr(n, nbits) -> str:
    """
    Converts an integer to a hexadecimal string representation.

    Args:
        n (int): The integer to be converted.
        nbits (int): The number of bits to represent the hexadecimal string.

    Returns:
        str: The hexadecimal string representation of the integer.

    """
    return hex(n)[2:].zfill(nbits // 4).upper()


def write_template(tpl_path, outdir, **kwargs):
    if tpl_path is not None:
        tpl_path = pathlib.Path(tpl_path).absolute()
        if tpl_path.exists():
            tpl = Template(filename=str(tpl_path))
            with open(
                outdir / tpl_path.with_suffix("").name, "w", encoding="utf-8"
            ) as f:
                code = tpl.render_unicode(**kwargs)
                code = re_trailws.sub("", code)
                f.write(code)
        else:
            raise FileNotFoundError(f"Template file {tpl_path} not found")


def main():
    # Parser for command line arguments
    parser = argparse.ArgumentParser(
        prog="heepatia-gen.py",
        description="Generate heepatia HDL files based on the provided configuration.",
    )
    parser.add_argument(
        "--cfg",
        "-c",
        metavar="FILE",
        type=argparse.FileType("r"),
        required=True,
        help="Configuration file in HJSON format",
    )
    parser.add_argument(
        "--outdir",
        "-o",
        metavar="DIR",
        type=pathlib.Path,
        required=True,
        help="Output directory",
    )
    parser.add_argument(
        "--tpl-sv", "-s", type=str, metavar="SV", help="SystemVerilog template filename"
    )
    parser.add_argument(
        "--tpl-c", "-C", type=str, metavar="C_SOURCE", help="C template filename"
    )
    parser.add_argument(
        "--carus_num",
        type=int,
        metavar="CARUS_NUM",
        help="Number of NM-Carus instances",
    )
    parser.add_argument(
        "--carus_size",
        type=str,
        metavar="CARUS_SIZE",
        help="Size of every NM-Carus instances",
    )
    parser.add_argument(
        "--carus_num_banks",
        type=int,
        metavar="CARUS_NUM_BANKS",
        help="Number of banks of every NM-Carus instances",
    )
    parser.add_argument(
        "--corev_pulp", nargs="?", type=bool, help="CORE-V PULP extension"
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Increase verbosity"
    )
    args = parser.parse_args()

    # Set verbosity level
    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Read HJSON configuration file
    with args.cfg as f:
        try:
            cfg = hjson.load(f, use_decimal=True)
            cfg = JsonRef.replace_refs(cfg)
        except ValueError as exc:
            raise SystemExit(sys.exc_info()[1]) from exc

    # Check if the output directory is valid
    if not args.outdir.is_dir():
        exit(f"Output directory {args.outdir} is not a valid path")

    # Create output directory
    args.outdir.mkdir(parents=True, exist_ok=True)

    # Get configuration parameters
    # ----------------------------
    # CORE-V-MINI-MCU configuration
    cpu_features = cfg["cpu_features"]
    if args.corev_pulp != None:
        cpu_features["corev_pulp"] = args.corev_pulp

    # OECGRA
    oecgra_start_address = int(cfg["ext_xbar_slaves"]["oecgra"]["offset"], 16)
    oecgra_start_address_hex = int2hexstr(oecgra_start_address, 32)
    oecgra_size = int(cfg["ext_xbar_slaves"]["oecgra"]["length"], 16)
    oecgra_size_hex = int2hexstr(oecgra_size, 32)

    xbar_nslaves = 1

    # NM-Carus instance number
    carus_num = int(cfg["ext_xbar_slaves"]["carus"]["num"])
    if args.carus_num is not None:
        carus_num = args.carus_num
    if carus_num < 0 or carus_num > 16:
        exit(f"NM-Carus instances number must be <16: {carus_num}")

    carus_num_banks = cfg["ext_xbar_slaves"]["carus"]["num_banks"]
    if args.carus_num_banks is not None:
        carus_num_banks = args.carus_num_banks
    for i in range(carus_num):
        if isinstance(carus_num_banks, list):
            if carus_num_banks[i] < 0 or carus_num_banks[i] > 16:
                exit(
                    f"NM-Carus number of banks must be <16: isntance {i} - {carus_num_banks[i]}"
                )
        else:
            if carus_num_banks < 0 or carus_num_banks > 16:
                exit(
                    f"NM-Carus number of banks must be <16: isntance {i} - {carus_num_banks}"
                )

    # Check if carus_num_banks is a list
    if isinstance(carus_num_banks, list):
        print(
            "A list is provided for carus_num_banks,",
            "every instance will have its own number of banks",
        )
        carus_num_banks = [int(x) for x in carus_num_banks]
    else:
        carus_num_banks = int(carus_num_banks)

    carus_size = cfg["ext_xbar_slaves"]["carus"]["length"]
    if args.carus_size is not None:
        carus_size = int(args.carus_size, 16)
    if isinstance(carus_size, list):
        print(
            "A list is provided for carus_size,",
            "every instance will have its own size",
        )
        carus_size = [int(x, 16) for x in carus_size]
    else:
        carus_size = int(carus_size, 16)
    for i in range(carus_num):
        if isinstance(carus_size, list):
            if carus_size[i] < 0 or carus_size[i] > 2**32:
                exit(f"NM-Carus size must be <2^32: instance {i} - {carus_size[i]}")
        else:
            if carus_size < 0 or carus_size > 2**32:
                exit(f"NM-Carus size must be <2^32: instance {i} - {carus_size}")

    # Bus configuration
    xbar_nmasters = int(cfg["ext_xbar_masters"])
    xbar_nslaves = xbar_nslaves + carus_num
    periph_nslaves = len(cfg["ext_periph"])

    carus_start_address = []
    carus_start_address_hex = []
    carus_size_hex = []
    # Slaves map
    if carus_num == 1:
        carus_start_address.append(int(cfg["ext_xbar_slaves"]["carus"]["offset"], 16))
        carus_start_address_hex.append(int2hexstr(carus_start_address[0], 32))
        carus_size_hex.append(int2hexstr(carus_size[0], 32))
    else:
        for i in range(carus_num):
            carus_size_hex.append(int2hexstr(carus_size[i], 32))
            if i == 0:
                carus_start_address.append(
                    int(cfg["ext_xbar_slaves"]["carus"]["offset"], 16)
                )
            else:
                carus_start_address.append(
                    int(
                        cfg["ext_xbar_slaves"]["carus"]["offset"],
                        16,
                    )
                    + carus_size[i - 1]
                )
            carus_start_address_hex.append(int2hexstr(carus_start_address[i], 32))

    # OECGRA bus
    oecgra_context_mem_start_address = int(cfg["oecgra_bus_slaves"]["oecgra_context_mem"]["offset"], 16)
    oecgra_context_mem_start_address_hex = int2hexstr(oecgra_context_mem_start_address, 32)
    oecgra_context_mem_size = int(cfg["oecgra_bus_slaves"]["oecgra_context_mem"]["length"], 16)
    oecgra_context_mem_size_hex = int2hexstr(oecgra_context_mem_size, 32)

    oecgra_config_regs_start_address = int(cfg["oecgra_bus_slaves"]["oecgra_config_regs"]["offset"], 16)
    oecgra_config_regs_start_address_hex = int2hexstr(oecgra_config_regs_start_address, 32)
    oecgra_config_regs_size = int(cfg["oecgra_bus_slaves"]["oecgra_config_regs"]["length"], 16)
    oecgra_config_regs_size_hex = int2hexstr(oecgra_config_regs_size, 32)

    # Peripherals map
    fll_start_address = int(cfg["ext_periph"]["fll"]["offset"], 16)
    fll_start_address_hex = int2hexstr(fll_start_address, 32)
    fll_size = int(cfg["ext_periph"]["fll"]["length"], 16)
    fll_size_hex = int2hexstr(fll_size, 32)

    heepatia_ctrl_start_address = int(cfg["ext_periph"]["heepatia_ctrl"]["offset"], 16)
    heepatia_ctrl_start_address_hex = int2hexstr(heepatia_ctrl_start_address, 32)
    heepatia_ctrl_size = int(cfg["ext_periph"]["heepatia_ctrl"]["length"], 16)
    heepatia_ctrl_size_hex = int2hexstr(heepatia_ctrl_size, 32)

    # Dependent parameters
    carus_bank_addr_width = []
    for a_size, a_num_banks in zip(carus_size, carus_num_banks):
        if not math.log2(a_size).is_integer():
            sys.exit(f"Carus size must be a power of 2: {a_size}")
        if not math.log2(a_num_banks).is_integer():
            sys.exit(f"Carus number of banks must be a power of 2: {a_num_banks}")
        if a_size % a_num_banks != 0:
            sys.exit(
                "Carus size must be a multiple of the number of banks: ",
                f"{a_size} % {a_num_banks} != 0",
            )
        carus_bank_addr_width.append(int(math.ceil(math.log2(a_size // a_num_banks))))

    # Explicit arguments
    kwargs = {
        "cpu_corev_pulp": int(cpu_features["corev_pulp"]),
        "cpu_corev_xif": int(cpu_features["corev_xif"]),
        "cpu_fpu": int(cpu_features["fpu"]),
        "cpu_riscv_zfinx": int(cpu_features["riscv_zfinx"]),
        "xbar_nmasters": xbar_nmasters,
        "xbar_nslaves": xbar_nslaves,
        "periph_nslaves": periph_nslaves,
        "oecgra_start_address": oecgra_start_address_hex,
        "oecgra_size": oecgra_size_hex,
        "carus_num": carus_num,
        "carus_num_banks": carus_num_banks,
        "carus_bank_addr_width": carus_bank_addr_width,
        "carus_start_address": carus_start_address_hex,
        "carus_size": carus_size_hex,
        "carus_mem_name": "xilinx_mem_gen_carus",
        "carus_tile_size": [
            cs // cn // 4 for cs, cn in zip(carus_size, carus_num_banks)
        ],
        "carus_vlen_max": [cs // 32 for cs in carus_size],
        "oecgra_context_mem_start_address": oecgra_context_mem_start_address_hex,
        "oecgra_context_mem_size": oecgra_context_mem_size_hex,
        "oecgra_config_regs_start_address": oecgra_config_regs_start_address_hex,
        "oecgra_config_regs_size": oecgra_config_regs_size_hex,
        "fll_start_address": fll_start_address_hex,
        "fll_size": fll_size_hex,
        "heepatia_ctrl_start_address": heepatia_ctrl_start_address_hex,
        "heepatia_ctrl_size": heepatia_ctrl_size_hex,
    }

    # Generate SystemVerilog package
    if args.tpl_sv is not None:
        write_template(args.tpl_sv, args.outdir, **kwargs)

    # Generate C header
    if args.tpl_c is not None:
        write_template(args.tpl_c, args.outdir, **kwargs)


if __name__ == "__main__":
    main()
