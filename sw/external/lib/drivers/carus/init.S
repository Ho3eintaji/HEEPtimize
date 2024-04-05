#include "carus_ctl_reg.h"
#include "carus_addr_map.h"
#include "carus_riscv_csr_encoding.h"

.section .init
.balign 4
.global _start
_start:
    # Do not enable interrupts so the core does not call the interrupt handler
    # when it wakes up from the wfi instruction.
    # csrwi mstatus, MSTATUS_MIE # enable interrupts (MIE)
    la sp, _sp # initialize stack pointer
    li t0, MIP_MEIP # set MEIE (and MEIP) bit
    csrw mie, t0 # enable external interrupts (MEIE)
    wfi # wait for interrupt (from control registers)
    lw ra, CTL_REG_KERNEL_REG_ADDR(zero) # load kernel address from control registers
    sw zero, CTL_REG_OP_CTL_REG_ADDR(zero) # clear op_ctl reg. (done, fetch_en, and start bits)
    jalr ra # call the user-defined vector kernel
    # NOTE: set the done enable bit. The done bit (done interrupt) is then
    # automatically set when the last vector instruction finishes.
    li t1, (0x1 << CARUS_CTL_OP_CTL_DONE_EN_BIT) # set done enable bit
    sw t1, CTL_REG_OP_CTL_REG_ADDR(zero) # write done enable bit
    j _start # restart
