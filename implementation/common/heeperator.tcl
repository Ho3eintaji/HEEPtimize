#
# HEEPpocrates SoC - EPFL / ESL 2022
# Christoph Müller (christoph.mueller@epfl.ch)
#

global design

set design(TOPLEVEL) heeperator
set design(FLOW_ROOT) $::env(FLOW_ROOT)

set design(ALL_LEFS) {}
set design(ALL_GDS) {}

set design(default_sdc) $design(FLOW_ROOT)/implementation/synthesis/last_output/netlist.sdc

set design(netlist) $design(FLOW_ROOT)/implementation/synthesis/last_output/netlist.v
set design(upf_file) $design(FLOW_ROOT)/heeperator.post_synthesis.upf
set design(mmmc_view_file) $design(FLOW_ROOT)/implementation/common/heeperator.view

set design(map_file) inputs/gds2.map

set design(all_ground_nets) {VSS VSSIO}
set design(all_power_nets) {VDD}

set design(fillers) {FILL1LVT FILL2LVT FILL4LVT FILL8LVT FILL16LVT FILL32LVT FILL64LVT}

set design(always_on_pins_tvdd) {PTINVD1HVT PTINVD2HVT PTINVD4HVT PTINVD8HVT PTINVD8HVT PTINVD4HVT PTINVD2HVT PTINVD1HVT PTINVD8HVT PTINVD4HVT PTINVD2HVT PTINVD1HVT PTBUFFD1HVT PTBUFFD2HVT PTBUFFD4HVT PTBUFFD8HVT PTBUFFD8HVT PTBUFFD4HVT PTBUFFD2HVT PTBUFFD1HVT PTBUFFD8HVT PTBUFFD4HVT PTBUFFD2HVT PTBUFFD1HVT}
set design(always_on_pins_tvss) { PTFBUFFD1HVT  PTFBUFFD2HVT  PTFBUFFD4HVT  PTFBUFFD8HVT  PTFDFCND1HVT  PTFINVD1HVT  PTFINVD2HVT  PTFINVD4HVT  PTFINVD8HVT  PTFINVD8HVT  PTFINVD4HVT  PTFINVD2HVT  PTFINVD1HVT  PTFDFCND1HVT  PTFBUFFD8HVT  PTFBUFFD4HVT  PTFBUFFD2HVT PTFBUFFD1HVT PTFINVD8HVT PTFINVD4HVT PTFINVD2HVT  PTFINVD1HVT PTFDFCND1HVT PTFBUFFD8HVT PTFBUFFD4HVT PTFBUFFD2HVT PTFBUFFD1HVT}
set design(always_on_pins) {}
foreach cell $design(always_on_pins_tvdd) {lappend design(always_on_pins) ${cell}:TVDD}
foreach cell $design(always_on_pins_tvss) {lappend design(always_on_pins) ${cell}:TVSS}

set design(lvs_excludes) $design(fillers)
lappend design(lvs_excludes) {*}{PFILLER05 PFILLER1 PFILLER5 PFILLER10 PFILLER20 PCORNER PRCUT UCSRN UCSRN_NOVIA01 UCSRN_NOVIA001 CORNER_B N65CHIPCDU2_New PAD60L RNPOLYWO RPPOLYWO heep_cover}
# Technology lefs
# none added - seems to be part of the standard cell lef file...???

lappend design(ALL_LEFS) {/dkits/tsmc/65nm/IP_65nm/utilities/PRTF_EDI_65nm_001_Cad_V24a/PR_tech/Cadence/LefHeader/HVH/PRTF_EDI_N65_9M_6X1Z1U_RDL.24a.tlef}

# Standard cells
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Back_End/lef/tcbn65lplvt_200a/lef/tcbn65lplvt_9lmT2.lef
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Back_End/lef/tcbn65lpcglvt_200a/lef/tcbn65lpcglvt_9lmT2.lef
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Back_End/lef/tcbn65lpcghvt_200a/lef/tcbn65lpcghvt_9lmT2.lef

lappend design(ALL_GDS) $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tcbn65lplvt_200a/tcbn65lplvt.gds
lappend design(ALL_GDS) $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tcbn65lpcglvt_200a/tcbn65lpcglvt.gds
lappend design(ALL_GDS) $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tcbn65lpcghvt_200a/tcbn65lpcghvt.gds

# we don't have backend views atm.
#lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/STDCELLs/Back_End/lef/tcbn65lplvt_200a/lef/tcbn65lplvt_9lmT2.lef

# IOs

lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Back_End/lef/tpan65lpnv2od3_200a/mt_2/9lm/lef/tpan65lpnv2od3_9lm.lef
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Back_End/lef/tpan65lpnv2od3_200a/mt_2/9lm/lef/antenna_9lm.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tpan65lpnv2od3_200a/mt_2/9lm/tpan65lpnv2od3.gds
#lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Back_End/lef/tpdn65lpnv2od3_140b/mt_2/9lm/lef/tpdn65lpnv2od3_9lm.lef
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/pads/tpdn65lpnv2od3_9lm_modified.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tpdn65lpnv2od3_140b/mt_2/9lm/tpdn65lpnv2od3.gds

lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/PADs/Back_End/lef/tpdn65lpnv2od3_140b/mt_2/9lm/lef/antenna_9lm.lef

# we don't have the GDS of this library?
lappend design(ALL_LEFS) {/dkits/tsmc/65nm/IP_65nm/LP/STDCELL_IO/designPackage/iolib/IO2.5V/LINEAR/Back_End/lef/tphn65lpnv2od3_sl_200b/mt_2/9lm/lef/tphn65lpnv2od3_sl_9lm.lef}
#lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/

lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/sealring/lef/sealring.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/sealring/gds/sealring.gds
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/bondpads/lef/tpbn65v_9lm.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/IP/Back_End/gds/tpbn65v_200b/wb/9m/9M_6X1Z1U/tpbn65v.gds
# Memories
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/lef/sram8192x32m8.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/sram8192x32m8/gds/sram8192x32m8.gds2
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/lef/rf128x32m2.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/symlinks/ARM_Memories/rf128x32m2/gds/rf128x32m2.gds2

# Memory power gates
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/mem-power-switches/lef/mem_power_switches.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/mem-power-switches/gds/mem_power_switches.gds

# FLL
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/fll/lef/tsmc65_FLL.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/fll/gds_merged/tsmc65_FLL.gds

# Caesar
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/vendor/nm-caesar-backend-opt/implementation/pnr/outputs/nm-caesar/lef/NMCaesar.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/vendor/nm-caesar-backend-opt/implementation/pnr/outputs/nm-caesar/gds/NMCaesar.gds

# Carus
lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/vendor/nm-carus-backend-opt/implementation/pnr/outputs/nm-carus/lef/NMCarus.lef
lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/vendor/nm-carus-backend-opt/implementation/pnr/outputs/nm-carus/gds/NMCarus.gds

# Heep Cover
# lappend design(ALL_LEFS) $design(FLOW_ROOT)/hw/asic/logo_cover/lef/heep_cover.lef
# lappend design(ALL_GDS)  $design(FLOW_ROOT)/hw/asic/logo_cover/gds/heep_cover.gds

source $design(FLOW_ROOT)/implementation/common/helper_functions.tcl

set ::env(CALIBRE_HOME) /softs/mentor/calibre/2022.2
set ::env(MGC_CALIBRE_INNOVUS_CUI_MODE) "1298096"

source $::env(CALIBRE_HOME)/lib/cal_enc.tcl