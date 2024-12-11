//Activate FPU instructions
//#define FS_INITIAL 0x01
//CSR_SET_BITS(CSR_REG_MSTATUS, (FS_INITIAL << 13));
lui	    t0,0x2
csrs	mstatus,t0