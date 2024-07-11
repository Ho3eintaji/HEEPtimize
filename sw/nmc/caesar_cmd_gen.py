# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: caesar_cmd_gen.py
# Author: Michele Caon
# Date: 31/10/2023
# Description: Caesar command generator

import gen_caesar_instructions as gci

class CaesarCmdGen:
    """
    Class for generating Caesar commands
    """

    def __init__(self) -> None:
        self.opcode_list = gci.instructions
        self.width_list = {32: 0, 16: 1, 8: 2}
        self.width = 32

    # Set data width
    def set_width(self, width: int) -> None:
        if width not in [8, 16, 32]:
            raise ValueError("Width must be 8, 16 or 32")
        if (width & (width - 1)) != 0:
            raise ValueError("Width must be a power of 2")
        self.width = width

    # Get configuration command
    def get_csr_cmd(self, csr_addr: int, width: int = None) -> (str, str):
        if width is None:
            width = self.width
        w = self.width_list[width]
        opcode = 'CSRW'
        if opcode not in self.opcode_list:
            raise ValueError(f"Invalid opcode: {opcode}")
        opcode = self.opcode_list[opcode]
        cmd = (opcode << 26) | (w << 13) | w
        cmd = hex(cmd)
        addr = hex(csr_addr)
        return (cmd, addr)

    # Build command
    def get_cmd(self, opcode: str, dest_baddr: int, src1_addr: int, src2_addr: int) -> (str, str):
        if opcode not in self.opcode_list:
            raise ValueError(f"Invalid opcode: {opcode}")
        opcode = self.opcode_list[opcode]
        cmd = (opcode << 26) | (src1_addr >> 2 << 13) | src2_addr >> 2
        cmd = hex(cmd)
        addr = hex(dest_baddr)
        return (cmd, addr)

    # Print a command
    def print_cmd(self, cmd: str, dest_addr: str) -> str:
        c = int(cmd, 16)
        opcode = c >> 26
        opcode_str = list(self.opcode_list.keys())[opcode]
        dest_addr = int(dest_addr, 16)
        arg1 = ((c >> 13) & 0x1FFF)
        arg2 = (c & 0x1FFF)
        if opcode_str == 'CSRW':
            cmd_str = f"CSR    <= 0x{arg1:01x}"
        else:
            arg1 = arg1 << 2 # convert to byte address
            arg2 = arg2 << 2 # convert to byte address
            cmd_str = f"0x{dest_addr:04x} <= {opcode_str:6} 0x{arg1:04x} 0x{arg2:04x}"
        return cmd_str
