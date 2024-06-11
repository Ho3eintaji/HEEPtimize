# Activate conda environment
conda activate core-v-mini-mcu

# Verilator
if [ -d "/softs/verilator/v4.220/bin" ]; then
    PATH="/softs/verilator/v4.220/bin:$PATH"
fi

# Questasim
export MODEL_TECH=/softs/mentor/qsta/2020.4/bin
export MGLS_LICENSE_FILE=1717@edalicsrv.epfl.ch
export MTI_VCO_MODE=64
PATH="/softs/mentor/qsta/2020.4/bin/:$PATH"

# RISC-V compiler
export RISCV=/shares/eslfiler1/apps/linux/Development/rv32imc

# Xilinx Vivado
export XILINX_VIVADO="/softs/xilinx/vivado/2023.1_lin64/"
export XILINXD_LICENSE_FILE=2100@ielsrv01.epfl.ch
PATH="$XILINX_VIVADO/bin:/edadk_repo/softs/xilinx/DocNav:$PATH"

# VERIBLE
export VERIBLE=/shares/eslfiler1/apps/linux/Development/verible
PATH=$VERIBLE/bin:$PATH

# Synopsys tools (Design Compiler, Library Compiler, VCS)
export SNPS_DC_VERS=2020.09
export SNPS_LC_VERS=2020.09
export SNPS_PP_VERS=2019.12
export SNPS_SITE_PATH=/softs/synopsys/site
export SNPSLMD_LICENSE_FILE=27020@edalicsrv.epfl.ch
export SYNOPSYS_DC=/softs/synopsys/dc/$SNPS_DC_VERS
export SYNOPSYS_PP=/softs/synopsys/ppower/$SNPS_PP_VERS
export SYNOPSYS_LC=/softs/synopsys/lc/$SNPS_LC_VERS
export MANPATH=/softs/synopsys/dc/$SNPS_DC_VERS/doc/dc/man
export VCS_HOME="/softs/synopsys/vcs/2022/"
export VERDI_HOME="/softs/synopsys/verdi/2022.06/"
PATH=$SNPS_SITE_PATH/bin:$SYNOPSYS_DC/bin:$SYNOPSYS_PP/bin:$SYNOPSYS_LC/bin:$VCS_HOME/bin:$VERDI_HOME/bin:$PATH

# Innovus
export CDS_LIC_FILE=5280@edalicsrv.epfl.ch
export INNOVUS_SITE=/softs/cadence/innovus/20.1/bin/
export GENUS_SITE=/softs/cadence/genus/20.1/bin/
export XCELIUM_SITE=/softs/cadence/xcelium/23.03/tools/xcelium/bin/

PATH=$INNOVUS_SITE:$GENUS_SITE:$XCELIUM_SITE:$PATH

# Export the updated PATH
export PATH
