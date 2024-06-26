create_pblock {pblock_gn_srm_bnks[0].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[0].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[0].u_mem_bank}]]
resize_pblock [get_pblocks {pblock_gn_srm_bnks[0].u_mm_bnk}] -add {RAMB18_X1Y108:RAMB18_X1Y119}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[0].u_mm_bnk}] -add {RAMB36_X1Y54:RAMB36_X1Y59}
create_pblock {pblock_gn_srm_bnks[1].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[1].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[1].u_mem_bank}]]
resize_pblock [get_pblocks {pblock_gn_srm_bnks[1].u_mm_bnk}] -add {RAMB18_X1Y96:RAMB18_X1Y107}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[1].u_mm_bnk}] -add {RAMB36_X1Y48:RAMB36_X1Y53}
create_pblock {pblock_gn_srm_bnks[2].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[2].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[2].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[2].u_mm_bnk}] -add {DSP48E2_X3Y84:DSP48E2_X5Y95}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[2].u_mm_bnk}] -add {RAMB18_X1Y84:RAMB18_X1Y95}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[2].u_mm_bnk}] -add {RAMB36_X1Y42:RAMB36_X1Y47}
create_pblock {pblock_gn_srm_bnks[3].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[3].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[3].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[3].u_mm_bnk}] -add {DSP48E2_X3Y74:DSP48E2_X5Y81}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[3].u_mm_bnk}] -add {RAMB18_X1Y74:RAMB18_X1Y81}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[3].u_mm_bnk}] -add {RAMB36_X1Y37:RAMB36_X1Y40}
create_pblock {pblock_gn_srm_bnks[4].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[4].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[4].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[4].u_mm_bnk}] -add {DSP48E2_X3Y60:DSP48E2_X5Y71}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[4].u_mm_bnk}] -add {RAMB18_X1Y60:RAMB18_X1Y71}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[4].u_mm_bnk}] -add {RAMB36_X1Y30:RAMB36_X1Y35}
create_pblock {pblock_gn_srm_bnks[5].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[5].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[5].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[5].u_mm_bnk}] -add {DSP48E2_X3Y48:DSP48E2_X5Y57}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[5].u_mm_bnk}] -add {RAMB18_X1Y48:RAMB18_X1Y57}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[5].u_mm_bnk}] -add {RAMB36_X1Y24:RAMB36_X1Y28}
create_pblock {pblock_gn_srm_bnks[6].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[6].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[6].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[6].u_mm_bnk}] -add {DSP48E2_X1Y132:DSP48E2_X3Y143}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[6].u_mm_bnk}] -add {RAMB18_X1Y132:RAMB18_X1Y143}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[6].u_mm_bnk}] -add {RAMB36_X1Y66:RAMB36_X1Y71}
create_pblock {pblock_gn_srm_bnks[7].u_mm_bnk}
add_cells_to_pblock [get_pblocks {pblock_gn_srm_bnks[7].u_mm_bnk}] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vrf/gen_sram_banks[7].u_mem_bank}]]
# resize_pblock [get_pblocks {pblock_gn_srm_bnks[7].u_mm_bnk}] -add {DSP48E2_X1Y120:DSP48E2_X3Y131}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[7].u_mm_bnk}] -add {RAMB18_X1Y120:RAMB18_X1Y131}
resize_pblock [get_pblocks {pblock_gn_srm_bnks[7].u_mm_bnk}] -add {RAMB36_X1Y60:RAMB36_X1Y65}
create_pblock pblock_u_vector_pipeline
add_cells_to_pblock [get_pblocks pblock_u_vector_pipeline] [get_cells -quiet [list {i_heepatia_top/u_heepatia_peripherals/gen_carus[1].u_nm_carus_wrapper/u_carus_top/u_vector_subsystem/u_vector_pipeline}]]
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X48Y181:SLICE_X57Y208}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X48Y149:SLICE_X58Y179}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {BUFG_PS_X0Y48:BUFG_PS_X0Y71}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X48Y120:SLICE_X57Y147}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X42Y330:SLICE_X51Y359}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X42Y300:SLICE_X51Y329}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X42Y270:SLICE_X51Y300}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {DSP48E2_X2Y108:DSP48E2_X3Y119}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X42Y240:SLICE_X51Y269}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {DSP48E2_X2Y96:DSP48E2_X3Y107}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X48Y210:SLICE_X56Y239}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {BUFG_PS_X0Y72:BUFG_PS_X0Y95}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {SLICE_X58Y120:SLICE_X65Y358}
resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {DSP48E2_X6Y48:DSP48E2_X6Y141}
# resize_pblock [get_pblocks pblock_u_vector_pipeline] -add {URAM288_X0Y32:URAM288_X0Y91}
