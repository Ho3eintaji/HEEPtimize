#!/usr/bin/env python3

# Copyright 2022 EPFL
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

import argparse
import hjson
import pathlib
import sys
import re
import logging
from subprocess import run
import csv
from jsonref import JsonRef
from mako.template import Template

def main():
    parser = argparse.ArgumentParser(prog="mcugen")

    parser.add_argument("--spice",
                        "-n",
                        metavar="file",
                        type=argparse.FileType('r'),
                        required=True,
                        help="the spice netlist to patch")

    parser.add_argument("--output",
                        "-o",
                        required=True,
                        help="the patched spice netlist")


    args = parser.parse_args()

    # Read spice
    with args.spice as file:
        try:
            netlist_obj = file.read().splitlines()
        except ValueError:
            raise SystemExit(sys.exc_info()[1])

    # Write output spice
    if args.output != None:
        print("Creating " + args.output)
        file = open(args.output, "w")
        file.write("* THIS FILE IS PATCHED by patch_spice_output.py!!!\n")

        looking_for_end = 0
        line_number = 0
        patched = 0
        i = 0
        print("analyzing " + str(len(netlist_obj)) + " line")
        for curr_line in netlist_obj:
            curr_line = curr_line.lstrip()
            if i!=len(netlist_obj)-1: #if not last
                next_line = netlist_obj[i+1]
                if ("VDDPAD=VDD" in curr_line):
                    if (("POC=POC_CGRA" in next_line) or ("POC=POC_CGRA" in curr_line)):
                        curr_line = curr_line.replace("VDDPAD=VDD", "VDDPAD=VDD_CGRA_TOP")
                        patched += 1
                    if (("POC=POC_BLADE" in next_line) or ("POC=POC_BLADE" in curr_line)):
                        curr_line = curr_line.replace("VDDPAD=VDD", "VDDPAD=VDD_BLADE1")
                        patched += 1
                if ("Xcgra_gnd1 PVSS3CDG" in curr_line):
                    curr_line += "VDD=VDD_CGRA_TOP"
                    patched += 1
                if ("Xcgra_gnd2 PVSS3CDG" in curr_line):
                    curr_line += "VDD=VDD_CGRA_TOP"
                    patched += 1
                if ("XVDDIO2 PVDD2POC" in curr_line):
                    curr_line += "VDD=VDD_CGRA_TOP"
                    patched += 1
                if ("XVSSIO2 PVSS3CDG" in curr_line):
                    curr_line += "VDD=VDD_CGRA_TOP"
                    patched += 1
                if ("XVDDIO3 PVDD2POC" in curr_line):
                    curr_line += "VDD=VDD_BLADE1"       
                    patched += 1
                if ("XVSSIO3 PVSS3CDG" in curr_line):
                    curr_line += "VDD=VDD_BLADE1"       
                    patched += 1
                #if ("Xbladetest_esdvss PVSS2ANA" in curr_line):
                #    curr_line += "AVSS=bladetest_gnd"
                #    patched += 1
            file.write(curr_line+"\n")
            i+=1

        print("Patched " + str(patched) + " VDDPADs")
        file.close()

    else:
        print("Output file name is missing")
        raise SystemExit(sys.exc_info()[1])


if __name__ == "__main__":
    main()