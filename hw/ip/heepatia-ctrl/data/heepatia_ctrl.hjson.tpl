// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: heepatia_ctrl.hjson
// Author: Michele Caon, Luigi Giuffrida
// Date: 29/04/2024
// Description: heepatia near-memory computing control registers

{
    name: "heepatia_ctrl"
    clock_primary: "clk_i",
    reset_primary: "rst_ni",
    bus_interfaces: [
        {
            protocol: "reg_iface",
            direction: "device",
        },
    ],
    regwidth: "32",
    registers: [
        {
            name: "OP_MODE",
            desc: "NM-Carus operating mode control",
            fields: [
% for inst in range(carus_num):
                {
                    bits: "${inst}",
                    name: "CARUS_IMC_${inst}",
                    desc: '''
                        When this bit is set, NM-Carus enters configuration mode, where it is possible to access the integrated instruction cache and its configuration registers.
                        When this bit is cleared, NM-Carus enters memory mode, where bus requests are interpreted as memory accesses.
                    ''',
                    swaccess: "rw",
                    hwaccess: "hro",
                    resval: "0",
                },
% endfor
            ],
            swaccess: "ro",
            hwaccess: "none",
            resval: "0",
        },

        { name:     "CGRA_MEM_SW_MONITOR",
        desc:     "CGRA Mem - Used to read Schmitt Trigger",
        swaccess: "ro",
        hwaccess: "hrw",
        fields: [
            { bits: "3:0", name: "CGRA_MEM_SW_MONITOR", desc: "Mem CGRA Schmitt Trigger" }
        ]
        },
        { name:     "CGRA_ENABLE",
        desc:     "Enable CGRA top wrapper clock gating unit",
        resval:   1, // enable by default
        swaccess: "rw",
        hwaccess: "hro",
        fields: [
            { bits: "1:0", name: "CGRA_ENABLE", desc: "Enable CGRA top wrapper clock gating unit" }
        ]
        },
    ],
}
