# NM-Carus address map
#include "carus_addr_map.h"

.section .text
.balign 4
.global _start

# considering output regs starting from v17
# supponing vl and vtype are set before calling this kernel (it's meant
# to be called rigth after the first carus_matmul_fixed kernel in the 
# self attention block)

_start:
    lw t0, CTL_REG_VL_REG_ADDR(zero) # load VL from control registers
    lw t1, CTL_REG_VTYPE_REG_ADDR(zero) # load SEW and LMUL from ctl. registers
    vsetvl zero, t0, t1 # set VL and SEW
    vsra.vi v1, v17, 1
    vsra.vi v2, v18, 1
    vsra.vi v3, v19, 1
    vsra.vi v4, v20, 1
    ret
