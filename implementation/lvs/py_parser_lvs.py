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

class Module:

 def __init__(self, name):

    self.name = name

 def add_body(self, body):

    self.body = body
 
def main():
    parser = argparse.ArgumentParser(prog="mcugen")

    parser.add_argument("--netlist",
                        "-n",
                        metavar="file",
                        type=argparse.FileType('r'),
                        required=True,
                        help="the netlist with leafcells")

    parser.add_argument("--module",
                        "-m",
                        metavar="file",
                        type=argparse.FileType('r'),
                        required=True,
                        help="the file containing the needed leafcells")

    parser.add_argument("--output",
                        "-o",
                        required=True,
                        help="The file containing the leafcell that are in both netlist and module files")


    args = parser.parse_args()

    # Read netlist
    with args.netlist as file:
        try:
            netlist_obj = file.readlines()
        except ValueError:
            raise SystemExit(sys.exc_info()[1])

    # Read modules
    with args.module as file:
        try:
            module_obj = file.read()
        except ValueError:
            raise SystemExit(sys.exc_info()[1])

    # Write output stub
    if args.output != None:
        print("Creating " + args.output)
        file = open(args.output, "w")
        file.write("// THIS FILE IS GENERATED!!!\n")
        file.close()
    else:
        print("Output file name is missing")
        raise SystemExit(sys.exc_info()[1])

    list_of_modules = []

    looking_for_end = 0
    body = ""
    name = ""
    line_number = 0
    m = Module("foo")

    for module in netlist_obj:
        line_number+=1
        module = module.lstrip()
        if looking_for_end == 1:
            body += module
            if ("endmodule" in module): 
                #print("endmodule at line " + str(line_number) + " for module " + name)
                looking_for_end = 0
                m.add_body(body)
                list_of_modules.append(m)   
        else:
            if ("module" in module) and not (module.startswith('//') or module.startswith('##')):

                try:
                    name = module.split()[1]
                    #print("module at line " + str(line_number) + " " + name)
                    m = Module(name)
                except:
                    continue

                if(name in module_obj):
                    print("Processing: " + name)
                    body = module

                    if not ("endmodule" in module): 
                        looking_for_end = 1
                        continue
        

    print("Found " + str(len(list_of_modules)) + " module")

    # Write output stub
    if args.output != None:
        file = open(args.output, "a")

        for module in list_of_modules:
            file.write(module.body + "\n")

        file.close()
    else:
        print("Output file name is missing")
        raise SystemExit(sys.exc_info()[1])


if __name__ == "__main__":
    main()