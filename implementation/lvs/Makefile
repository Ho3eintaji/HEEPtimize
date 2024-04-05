verilog.stub.inc: netlist_lvs_stubs.v netlist_lvs.v create_stub.py
	python3 create_stub.py

heepocrates_lvs.cdl: verilog.stub.inc netlist_lvs.v netlist_lvs_stubs.v v2lvs.sh
	./v2lvs.sh

heepocrates_lvs.patched.cdl: heepocrates_lvs.cdl patch_spice_output.py
	python3 patch_spice_output.py --spice heepocrates_lvs.cdl --output heepocrates_lvs.patched.cdl

show_layout:
	/softs/mentor/calibre/2022.2/bin/calibredrv heepocrates.gds.gz -l /dkits/tsmc/65nm/IP_65nm/CALIBRE_LAYER/65nm_9m6x1z1u.layerprops

lvs: heepocrates_lvs.patched.cdl
	/softs/mentor/calibre/2022.2/bin/calibre -lvs -hcell hcells.txt -hier -turbo -turbo_all -hyper lvs_rules 

results:
	/softs/mentor/calibre/2022.2/bin/calibre -rve svdb/

clean:
	rm -rf heepocrates_lvs.cdl heepocrates_lvs.patched.cdl verilog.stub.inc verilog_stubs calibre_v2lvs.tcl