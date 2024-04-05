# Copyright EPFL contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

REG_DIR=$(dirname $0)
ROOT=$(realpath "$(dirname $0)/../../../../..")
REGTOOL=$ROOT/hw/vendor/x-heep/hw/vendor/pulp_platform_register_interface/vendor/lowrisc_opentitan/util/regtool.py

echo "Generating FLL control software header..."
$REGTOOL --cdefines -o $ROOT/sw/external/lib/drivers/fll/fll_regs.h $REG_DIR/fll_regs.hjson
