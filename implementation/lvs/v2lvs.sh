#first, patch the FLL patH to remote ETH absolute path
cp ../../hw/asic/fll/cdl/tsmc65_FLL.cdl tsmc65_FLL.patched.cdl 
sed -i 's/.INCLUDE \/usr\/pack\/tsmc-65-kgf\/tsmc\/pdk\/v1.7b\/Calibre\/lvs\/source.added/ /' tsmc65_FLL.patched.cdl 

#then patch the IO library
cp ../../STDCELLs_SPICE/tpdn65lpnv2od3_140b/tpdn65lpnv2od3_3.spi tpdn65lpnv2od3_3.patched.spi
sed -i 's/.GLOBAL VDD VSS VDDPST POC/*.GLOBAL VDD VSS VDDPST POC/' tpdn65lpnv2od3_3.patched.spi
sed -i '/.SUBCKT PD/ s/$/ VDD VSS VDDPST POC/' tpdn65lpnv2od3_3.patched.spi
sed -i 's/.SUBCKT PVDD2POC VDDPST/.SUBCKT PVDD2POC VDDPST POC VDD/' tpdn65lpnv2od3_3.patched.spi
sed -i 's/.SUBCKT PVSS2ANA AVSS/.SUBCKT PVSS2ANA AVSS VDDPST/' tpdn65lpnv2od3_3.patched.spi
sed -i 's/.SUBCKT PVSS3CDG VSS/.SUBCKT PVSS3CDG VSS VDDPST VDD/' tpdn65lpnv2od3_3.patched.spi

python3 create_v2lvs_script.py

/softs/mentor/calibre/2022.2/bin/v2lvs -tcl calibre_v2lvs.tcl | grep -v -e "Warning: Unsupported compiler directive" \
                                         -e "Warning: Duplicate declaration" \
                                         -e "Warning: Duplicate port/net" \
                                         -e "Second declaration ignored" \
                                         -e "Warning: Module instantiation .* has pin mismatches"

# finally patch the generated cdl, because we are weak in Calibre

