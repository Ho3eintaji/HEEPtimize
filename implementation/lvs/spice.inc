.INCLUDE ./source.added
.INCLUDE ../../ARM_Memories/sram8192x32m8/cdl/sram8192x32m8.cdl
.INCLUDE ../../ARM_Memories/rf128x32m2/cdl/rf128x32m2.cdl
.INCLUDE ./tsmc65_FLL.patched.cdl
.INCLUDE ../../hw/asic/coubertin/cdl/bladeFF_3006.cdl
.INCLUDE ../../hw/asic/blade/cdl/blade_top.cdl
.INCLUDE ../../STDCELLs_SPICE/tcbn65lpcghvt_200a/tcbn65lpcghvt_200a.spi
.INCLUDE ../../STDCELLs_SPICE/tcbn65lpcglvt_200a/tcbn65lpcglvt_200a.spi
.INCLUDE ../../STDCELLs_SPICE/tcbn65lplvt_200a/tcbn65lplvt_200a.spi
.INCLUDE ../../ARM_IPs/sc8_base_hvt/r0p0/cdl/sc8_cln65lp_base_hvt.cdl
.INCLUDE ../../ARM_IPs/sc8_base_lvt/r0p0/cdl/sc8_cln65lp_base_lvt.cdl
.INCLUDE ../../ARM_IPs/sc8_base_rvt/r0p0/cdl/sc8_cln65lp_base_rvt.cdl
.INCLUDE ./tpdn65lpnv2od3_3.patched.spi
.INCLUDE ../../hw/asic/mem_power_switches/cdl/switch_cell_mem.cdl


.GLOBAL POC_MAIN
.GLOBAL POC_BLADE
.GLOBAL POC_CGRA
.GLOBAL VDDPST_MAIN
.GLOBAL VDDPST_CGRA
.GLOBAL VDDPST_BLADE
.GLOBAL VDD_CGRA_TOP
.GLOBAL VDD_BLADE1
.GLOBAL bladetest_gnd
