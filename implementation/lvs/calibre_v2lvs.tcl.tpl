### THIS FILE IS GENERATED, DO NOT MODIFY IT DIRECTLY ###

v2lvs::set_warning_level -level 4
v2lvs::override_globals -default_not_connected
v2lvs::combine_interface_info  -enable

v2lvs::load_verilog -filename netlist_lvs.v

v2lvs::load_verilog -filename ./verilog.stub.inc -lib_mode

v2lvs::load_spice   -filename ./spice.inc -range_mode

v2lvs::set_includes -filename ./spice.inc

#v2lvs::override_globals -supply0 VSS
#v2lvs::override_globals -supply1 VDD

v2lvs::add_actual_port -module * -inst * -connect_formal_actual { VSS VSS }

v2lvs::add_formal_port -port POC -under heepocrates_pad_ring
v2lvs::add_formal_port -port VDDPST -under heepocrates_pad_ring
v2lvs::add_formal_port -port VDDPAD -under heepocrates_pad_ring

% for pad in pads:

${pad}

v2lvs::add_actual_port -module * -connect_formal_actual { POC_${pad.ring_side.upper()} POC } -force -group heepocrates_pad_ring
v2lvs::add_actual_port -module heepocrates_pad_ring -inst ${pad.instance} -connect_formal_actual { POC POC_${pad.ring_side.upper()} }
v2lvs::add_actual_port -module * -connect_formal_actual { VDDPST_${pad.ring_side.upper()} VDDPST } -force -group heepocrates_pad_ring
v2lvs::add_actual_port -module heepocrates_pad_ring -inst ${pad.instance} -connect_formal_actual { VDDPST VDDPST_${pad.ring_side.upper()} }

v2lvs::add_actual_port -module * -connect_formal_actual { ${pad.vdd_ring} VDDPAD } -force -group heepocrates_pad_ring
v2lvs::add_actual_port -module heepocrates_pad_ring -inst ${pad.instance} -connect_formal_actual { VDDPAD ${pad.vdd_ring} }
v2lvs::add_actual_port -module * -connect_formal_actual { VDDPAD VDD } -force -group pad_cell_*
v2lvs::add_actual_port -module pad_cell_* -inst pad_*_i -connect_formal_actual { VDD VDDPAD }

%endfor


v2lvs::write_output -filename heepocrates_lvs.cdl
exit