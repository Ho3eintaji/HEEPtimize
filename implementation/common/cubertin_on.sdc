# enable the domain
set_case_analysis 1 [get_pins heepocrates_peripheral_i/coubertin_subsystem_i/coubertin_clk_gating_i/clk_gate_i/clk_gating_i/E]

# overwrite the system clock with a slower one
create_clock [get_pins heepocrates_peripheral_i/fll_subsystem_i/fll_i/FLLCLK]  -name FLL_CLK  -period 8  -waveform {0 4}
set_multicycle_path 1 -setup -through heepocrates_peripheral_i/coubertin_subsystem_i/bladeFF_3006_i/data_out*

# false path the acknowledge chains from the power gating cells.
set_false_path -to core_v_mini_mcu_i/ao_peripheral_subsystem_i/power_manager_i/sync_cpu_ack_i/reg_q_reg_0_/D
set_false_path -to core_v_mini_mcu_i/ao_peripheral_subsystem_i/power_manager_i/sync_periph_ack_i/reg_q_reg_0_/D
set_false_path -to core_v_mini_mcu_i/ao_peripheral_subsystem_i/power_manager_i/sync_external_*_ack_i/reg_q_reg_0_/D
