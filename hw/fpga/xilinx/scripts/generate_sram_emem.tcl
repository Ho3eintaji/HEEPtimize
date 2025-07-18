
set ipName xilinx_emem_gen_carus

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name $ipName

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Algorithm {Minimum_Area}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {128}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips $ipName]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips $ipName]
launch_run -jobs 8 ${ipName}_synth_1
wait_on_run ${ipName}_synth_1
