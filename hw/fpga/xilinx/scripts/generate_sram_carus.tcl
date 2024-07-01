set ipName xilinx_mem_gen_carus_2k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_2k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {512}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_2k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_2k]
launch_run -jobs 8 xilinx_mem_gen_carus_2k_synth_1

set ipName xilinx_mem_gen_carus_4k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_4k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {1024}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_4k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_4k]
launch_run -jobs 8 xilinx_mem_gen_carus_4k_synth_1

set ipName xilinx_mem_gen_carus_8k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_8k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {2048}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_8k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_8k]
launch_run -jobs 8 xilinx_mem_gen_carus_8k_synth_1

set ipName xilinx_mem_gen_carus_16k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_16k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {4096}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_16k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_16k]
launch_run -jobs 8 xilinx_mem_gen_carus_16k_synth_1

set ipName xilinx_mem_gen_carus_32k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_32k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {8192}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_32k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_32k]
launch_run -jobs 8 xilinx_mem_gen_carus_32k_synth_1

set ipName xilinx_mem_gen_carus_64k

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name xilinx_mem_gen_carus_64k

set_property -dict [list CONFIG.Enable_32bit_Address {false} \
                        CONFIG.Use_Byte_Write_Enable {true}  \
                        CONFIG.Byte_Size {8}  \
                        CONFIG.Write_Width_A {32}  \
                        CONFIG.Write_Depth_A {16384}  \
                        CONFIG.Read_Width_A {32}  \
                        CONFIG.Enable_A {Use_ENA_Pin}  \
                        CONFIG.Write_Width_B {32}  \
                        CONFIG.Read_Width_B {32}  \
                        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}  \
                        CONFIG.Use_RSTA_Pin {false}  \
                        CONFIG.EN_SAFETY_CKT {false}] [get_ips xilinx_mem_gen_carus_64k]

#generate_target {instantiation_template} [get_ips $ipName]

#export_ip_user_files -of_objects [get_ips $ipName] -no_script -sync -force -quiet

create_ip_run [get_ips xilinx_mem_gen_carus_64k]
launch_run -jobs 8 xilinx_mem_gen_carus_64k_synth_1

wait_on_run xilinx_mem_gen_carus_2k_synth_1
wait_on_run xilinx_mem_gen_carus_4k_synth_1
wait_on_run xilinx_mem_gen_carus_8k_synth_1
wait_on_run xilinx_mem_gen_carus_16k_synth_1
wait_on_run xilinx_mem_gen_carus_32k_synth_1
wait_on_run xilinx_mem_gen_carus_64k_synth_1
