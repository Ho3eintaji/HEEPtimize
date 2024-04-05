# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

REG_DIR=$(dirname $0)
ROOT=$(realpath "$(dirname $0)/../../..")
REGTOOL=$ROOT/hw/vendor/x-heep/hw/vendor/pulp_platform_register_interface/vendor/lowrisc_opentitan/util/regtool.py
HJSON_FILE=$REG_DIR/data/heeperator_ctrl.hjson
RTL_DIR=$REG_DIR/rtl
SW_DIR=$ROOT/sw/external/lib/drivers/heeperator-ctrl

mkdir -p $RTL_DIR $SW_DIR
printf -- "Generating HEEPerator control registers RTL..."
$REGTOOL -r -t $RTL_DIR $HJSON_FILE
[ $? -eq 0 ] && printf " OK\n" || exit $?
printf -- "Generating HEEPerator control software header..."
$REGTOOL --cdefines -o $SW_DIR/heeperator_ctrl_reg.h $HJSON_FILE
[ $? -eq 0 ] && printf " OK\n" || exit $?
