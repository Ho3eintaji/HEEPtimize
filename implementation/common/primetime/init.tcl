# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

set design(FLOW_ROOT) $::env(FLOW_ROOT)

set SET_LIBS     ../common/primetime/set_libs.tcl
set REPORTS_PATH ./reports
# set CONSTRAINTS  $design(FLOW_ROOT)/implementation/synthesis/last_output/netlist.sdc

# create reports folder
sh mkdir -p $REPORTS_PATH

# set libraries
source ${SET_LIBS}

# read libraries
read_db $target_library

# read netlist
read_verilog $NETLIST

# link design
current_design ${TOP_MODULE}
link_design -verbose
