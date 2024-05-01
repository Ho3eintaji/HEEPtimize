#!/usr/bin/env python3

# Copyright 2023 EPFL
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

#
# CAREFUL, THIS SCRIPT DOES NOT WORK with MORE THAN 1 INSTANCE of CARUS
# IT MAKES ASSUMPTIONS, and IT HAS BEEN TESTED ONLY FOR ONE PURPOSE
#

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

    parser.add_argument("--sdf",
                        "-n",
                        metavar="file",
                        type=argparse.FileType('r'),
                        required=True,
                        help="the sdf with cells")

    parser.add_argument("--output",
                        "-o",
                        required=True,
                        help="The file containing the leafcell that are in both sdf and module files")

    parser.add_argument("--verbose",
                        "-v",
                        required=False,
                        action='store_true',
                        help="if debug info are print")

    args = parser.parse_args()

    # Read sdf
    with args.sdf as file:
        try:
            sdf_obj = file.readlines()
        except ValueError:
            raise SystemExit(sys.exc_info()[1])


    # Write sdf
    if args.output != None:
        print("Creating " + args.output)
        file = open(args.output, "w")
    else:
        print("Output file name is missing")
        raise SystemExit(sys.exc_info()[1])


    if args.verbose != None:
        print("Verbose script")
        verbose= True
    else:
        verbose= False

    line_number = 0
    len_file = len(sdf_obj)
    if(verbose):

        print("SDF file is " + str(len_file) + " long")

    found_appended_sdf = False
    found_carus = False
    start_copying = False
    found_carus_lib = False

    #first, remove the appended information as SDFFILE etc
    for sdf_obj_line in sdf_obj:

        sdf_line = sdf_obj_line.split()
        next_sdf_line = ""

        if(line_number < len_file-1):
            next_sdf_obj_line = sdf_obj[line_number+1]
            next_sdf_line = next_sdf_obj_line.lstrip()

        if(line_number < len_file-2):
            next_next_sdf_obj_line = sdf_obj[line_number+2]
            next_next_sdf_line = next_next_sdf_obj_line.lstrip()

        line_number+=1

        if ("DELAYFILE" not in next_sdf_line) and found_appended_sdf == False:

            modified_string = False
            if ("DELAYFILE" in next_sdf_line) and found_appended_sdf == False:
                if(verbose):
                    print("Found DELAYFILE in line " + str(line_number))

            if ("DELAYFILE" in next_sdf_line) and found_appended_sdf == True:
                start_copying = False
                found_appended_sdf = False
                if(verbose):
                    print("Found another DELAYFILE in line " + str(line_number))

            found_appended_sdf = True

            try:
                if ("(CELL" == sdf_line[0]):
                    start_copying = True
            except:
                pass

            if("(CELLTYPE  \"carus_top" in sdf_obj_line):
                found_carus = True
                if(verbose):
                    print("Found carus_top" + sdf_obj_line + " in line " + str(line_number))

            if ("(INTERCONNECT" in sdf_obj_line):
                if found_carus:
                    sdf_line[1] = "u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/" + sdf_line[1]
                    sdf_line[2] = "u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/" + sdf_line[2]
                if start_copying:
                    file.write(" ".join(sdf_line) + "\n")
                    modified_string = True


            if ("(INSTANCE" in sdf_line):
                if found_carus:
                    sdf_line[1] = "u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper/u_carus_top/" + sdf_line[1]
                if start_copying:
                    file.write(" ".join(sdf_line) + "\n")
                    modified_string = True

            if modified_string == False:
                if start_copying:
                    file.write(sdf_obj_line)

    file.close()


if __name__ == "__main__":
    main()