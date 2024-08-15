# STDCELLS for CLK TREE and CLK GATING 

I used cells from the HVT library --> GF22FDX_SC8T_104CPP_BASE_CSC28H

You can find the documentation here : 
/dkits/synopsys/DesignWare_logic_libs/22fdx/STDCELLS/GF22FDX_SC8T_104CPP_BASE_CSC28H_FDK_RELV05R30/doc


# PADS 

0. Power/ ground cells

  - Cell name : IN22FDX_GPIO18_10M3S40PI_SPVDDIO_H / IN22FDX_GPIO18_10M3S40PI_SPVDDIO_V
  - Ports : 
    - ...


1. GPIO

  - Cell name : IN22FDX_GPIO18_10M3S40PI_IO_H / IN22FDX_GPIO18_10M3S40PI_IO_V
  - Ports : 
    - input TRIEN: active high tri state enable (chose driver or receiver mode)
    - input NDIN: NAND tree input (don t care ?)
    - output NDOUT: NAND tree output (don t care ?)
    - input [1:0] DRV: Drive strength control pin (don t care, can be whatever)
    - inout PAD: PAD pin where data is received/transmitted
    - inout PWROK: Core power detection (pad_attribute[0])
    - inout IOPWROK: IO power detection (pad_attribute[1])
    - output Y: Data received from PAD
    - input RXEN: Active high receiver enable
    - input PDEN: Pull down control (don t care)
    - input PUEN: Pull up control (dont care)
    - input DATA: Input data to be transmitted (from core to pad)
    - inout RETC: Retention mode active low --> should be 1
    - inout BIAS: BIAS for level shifter ? for data from core to pad (pad_attribute[2])
    - input SMT: Schmitt trigger enable, should be 1 in receiver mode
    - input SLW: 0 if VDDIO =1.8V , 1 if VDDIO = 1.2 or 1.5V 


