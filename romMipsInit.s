/* romMipsInit.s - MIPS ROM initialization module */

/* Copyright 2001-2002 Wind River Systems, Inc. */
	.data
	.globl	copyright_wind_river

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */

/*
modification history
--------------------
01i,12dec02,zmm  Remove references to R4000 CPU.
01h,07jun02,jmt  Modified to add override of exception table
01g,18jan02,tlc  Add .set noreorder to ROM vector section.
01f,05sep01,tlc  Revise boot vector table.
01e,16jul01,tlc  Add CofE copyright.
01d,18jun01,tlc  Add relocation to KSEG0 for cached ROMS.  Add sysLedErr()
                 routine.
01c,14jun01,tlc  Remove definition of WBFL.
01b,13jun01,tlc  Use INITIAL_SR macro to initialize the Status Register.
01a,12jun01,tlc  Add MACRO to perform initial configuration of CONFIG
                 register.

*/
		
/*
DESCRIPTION
This module contains the common MIPS entry code for the VxWorks bootrom.
The entry point romInit, is the first code executed on power-up.  All MIPS
BSPs can utilize this source code for the romInit routine.  To do so 
requires the following steps.

1) Determine if the CONFIG_INIT macro is suitable for the target BSP.  It
   simply loads the macro INITIAL_CFG into the CONFIG register.  BSPs
   requiring other means of CONFIG register initialization should #define
   an alternative CONFIG_INIT macro in their romInit.s files.
2) Determine if the CONFIG1_INIT macro is suitable for the target BSP.  Some
   MIPS targets have a CONFIG 1 coprocessor register that must be initialized
   in romInit.s.  The default simply performs a noop.  BSPs requiring CONFIG 1
   initialization should #define an alternative CONFIG1_INIT macro in their
   romInit.s files.
3) Determine if the default vector table is suitable for the target BSP.  Some
   MIPS targets have a larger vector table or different vector requirements.
   To create a different vector table, define the MIPS_VECTOR_TABLE macro in
   romInit.s.  The macro should be of the form:
      #define MIPS_VECTOR_TABLE \
              RVECENT(__romInit,0)            /@ PROM entry point @/ \
              RVECENT(romReboot,1)            /@ software reboot @/ \
              RVECENT(romReserved,2) \
              RVECENT(romReserved,3) \
              ...
   If no MIPS_VECTOR_TABLE macro is defined, the default table is included.
4) Create a routine, sysMemInit(), which is called by the common romInit()
   routine and initializes memory for the system.  This routine should be
   placed in the BSP's romInit.s file.
5) Create a routine, sysCacheInit(), which is called by the common romInit()
   routine and initializes memory for the system.  This routine should be
   placed in the BSP's romInit.s file.
6) Create a macro, INITIAL_SR, which defines the initial Status Register
   value for the system.  This may be placed in <bsp>.h.
7) Include the file "romMipsInit.s" at the *top* of the BSP's romInit.s file;
   just after header and macro definitions.	
*/

#ifndef	CONFIG1_INIT
#define CONFIG1_INIT ssnop
#endif
	
/* Relocate an address.
 * 
 * This macro is used to call routines from romInit() that are
 * outside the romInit.s file.  This routine trashes the ra
 * register.
 *
 * NOTE: This macro may have already been defined by the BSP.
 */
#ifndef RELOC
#define	RELOC(toreg,address) \
	bal	9f; \
9:; \
	la	toreg,address; \
	addu	toreg,ra; \
	la	ra,9b; \
	subu	toreg,ra
#endif

#define RVECENT(f,n) \
	b f; nop
#define XVECENT(f,bev) \
	b f; li k0,bev

#if 1

#define GPIOLED_SET(val)    \
    li  t0, 0xbfe0011c;     \
    li  t1, 0x00;           \
    sw  t1, 0x04(t0);       \
    li  t1, val;            \
    sw  t1, 0x00(t0);       \

#endif

	/* internals */

	.globl	romInit			/* start of system code */

	/* externals */

	.extern	romStart		/* system initialization routine */
 
	.data

	/* ensure data segment is 16-byte aligned */

	.align 	4
_sdata:
	.asciiz	"start of data"

	.text
	.set	noreorder
promEntry:
romInit:
_romInit:
#ifdef MIPS_VECTOR_TABLE
	MIPS_VECTOR_TABLE

#else /* MIPS_VECTOR_TABLE */
	RVECENT(__romInit,0)		/* PROM entry point */
	RVECENT(romReboot,1)		/* software reboot */
	RVECENT(romReserved,2)
	RVECENT(romReserved,3)
	RVECENT(romReserved,4)
	RVECENT(romReserved,5)
	RVECENT(romReserved,6)
	RVECENT(romReserved,7)
	RVECENT(romReserved,8)
	RVECENT(romReserved,9)
	RVECENT(romReserved,10)
	RVECENT(romReserved,11)
	RVECENT(romReserved,12)
	RVECENT(romReserved,13)
	RVECENT(romReserved,14)
	RVECENT(romReserved,15)
	RVECENT(romReserved,16)
	RVECENT(romReserved,17) 
	RVECENT(romReserved,18)
	RVECENT(romReserved,19)
	RVECENT(romReserved,20)
	RVECENT(romReserved,21)
	RVECENT(romReserved,22)
	RVECENT(romReserved,23)
	RVECENT(romReserved,24)
	RVECENT(romReserved,25)
	RVECENT(romReserved,26)
	RVECENT(romReserved,27)
	RVECENT(romReserved,28)
	RVECENT(romReserved,29)
	RVECENT(romReserved,30)
	RVECENT(romReserved,31)
	RVECENT(romReserved,32)
	RVECENT(romReserved,33)
	RVECENT(romReserved,34)
	RVECENT(romReserved,35)
	RVECENT(romReserved,36)
	RVECENT(romReserved,37)
	RVECENT(romReserved,38)
	RVECENT(romReserved,39)
	RVECENT(romReserved,40)
	RVECENT(romReserved,41)
	RVECENT(romReserved,42)
	RVECENT(romReserved,43)
	RVECENT(romReserved,44)
	RVECENT(romReserved,45)
	RVECENT(romReserved,46)
	RVECENT(romReserved,47)
	RVECENT(romReserved,48)
	RVECENT(romReserved,49)
	RVECENT(romReserved,50)
	RVECENT(romReserved,51)
	RVECENT(romReserved,52)
	RVECENT(romReserved,53)
	RVECENT(romReserved,54)
	RVECENT(romReserved,55)
	RVECENT(romReserved,56)
	RVECENT(romReserved,57)
	RVECENT(romReserved,58)
	RVECENT(romReserved,59)
	RVECENT(romReserved,60)
	RVECENT(romReserved,61)
	RVECENT(romReserved,62)
	RVECENT(romReserved,63)	
	XVECENT(romExcHandleTlbMiss,0x200)	/* bfc00200: tlbmiss vector */
	RVECENT(romReserved,65)
	RVECENT(romReserved,66)
	RVECENT(romReserved,67)
	RVECENT(romReserved,68)
	RVECENT(romReserved,69)
	RVECENT(romReserved,70)
	RVECENT(romReserved,71)
	RVECENT(romReserved,72)
	RVECENT(romReserved,73)
	RVECENT(romReserved,74)
	RVECENT(romReserved,75)
	RVECENT(romReserved,76)
	RVECENT(romReserved,77)
	RVECENT(romReserved,78)
	RVECENT(romReserved,79)	
	XVECENT(romExcHandle,0x280)	/* bfc00280: xtlbmiss vector */
	RVECENT(romReserved,81)
	RVECENT(romReserved,82)
	RVECENT(romReserved,83)
	RVECENT(romReserved,84)
	RVECENT(romReserved,85)
	RVECENT(romReserved,86)
	RVECENT(romReserved,87)
	RVECENT(romReserved,88)
	RVECENT(romReserved,89)
	RVECENT(romReserved,90)
	RVECENT(romReserved,91)
	RVECENT(romReserved,92)
	RVECENT(romReserved,93)
	RVECENT(romReserved,94)
	RVECENT(romReserved,95)	
	XVECENT(romExcHandle,0x300)	/* bfc00300: cache vector */
	RVECENT(romReserved,97)
	RVECENT(romReserved,98)
	RVECENT(romReserved,99)
	RVECENT(romReserved,100)
	RVECENT(romReserved,101)
	RVECENT(romReserved,102)
	RVECENT(romReserved,103)
	RVECENT(romReserved,104)
	RVECENT(romReserved,105)
	RVECENT(romReserved,106)
	RVECENT(romReserved,107)
	RVECENT(romReserved,108)
	RVECENT(romReserved,109)
	RVECENT(romReserved,110)
	RVECENT(romReserved,111)
	XVECENT(romExcHandle,0x380)	/* bfc00380: general vector */
	RVECENT(romReserved,113)
	RVECENT(romReserved,114)
	RVECENT(romReserved,115)
	RVECENT(romReserved,116)
	RVECENT(romReserved,117)
	RVECENT(romReserved,118)
	RVECENT(romReserved,119)
	RVECENT(romReserved,120)
	RVECENT(romReserved,121)
	RVECENT(romReserved,122)
	RVECENT(romReserved,123)
	RVECENT(romReserved,124)
	RVECENT(romReserved,125)
	RVECENT(romReserved,126)
	RVECENT(romReserved,127)

	/* We hope there are no more reserved vectors!
	 * 128 * 8 == 1024 == 0x400
	 * so this is address R_VEC+0x400 == 0xbfc00400
	 */

#endif /* MIPS_VECTOR_TABLE */
	
	.align 4
	.set	reorder
	
/******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*

* romInit 
*     (
*     int startType
*     )
*/

__romInit:	
	/* force power-on startType */

	li	a0, BOOT_CLEAR

	/*
	 * If there was some way to distinguish between a cold and warm
	 * restart AND the memory system is guaranteed to be intact then
	 * we could load BOOT_NORMAL instead
	*/

romReboot:
	/* sw reboot inherits a0 startType */
	move	s0, a0			/* save startType */
	move  s7, ra       /*just for debug*/
	/* clear software interrupts */
	
	mtc0	zero, C0_CAUSE
	mtc0	zero, C0_WATCHLO
	mtc0	zero, C0_WATCHHI

	/* initialize status register */
	
	li	t0, INITIAL_SR
	mtc0	t0, C0_SR
	
    /* set GPIO */
	bal  		watchdog_init
	nop

	#include "preInitcom.s"
#if 0
#undef 		PRINTSTR(x)
#undef		TTYDBG(x)
#define 		PRINTSTR(x)
#define 		TTYDBG(x)
#endif
	PRINTSTR("COM1 Init done!\r\n")
	
	/* absolutely basic initialization to allow things to continue */

/*  
 * now, we just write ddr2 parameters directly. 
 * we should use i2c for memory auto detecting. 
 */
gs_2f_v3_ddr2_cfg:
	PRINTSTR("\r\nenable register space of MEMORY\r\n")
        li  	t2,0xbfe00180
        ld  	a1,0x0(t2)
	and 	a1,a1,0x6ff 
        sd  a1,0x0(t2)

	PRINTSTR("DDR2 config begin_whd\r\n")
	b	ddr2_config
	nop
ddr2_config_done:
	PRINTSTR("DDR2 config end\r\n")

	

#if 1 /* read ddr2 registers */
        li	t0, 0xaffffe00
        
not_locked:
        ld	t1, 0x10(t0)
        andi    t1, 0x01
        beqz    t1, not_locked
        nop

        PRINTSTR("DDR2 DLL locked\r\n")
        
        ld	t1, 0xf0(t0)
        move    a0, t1
        bal     hexserial
        nop

#endif
        
    ###disable the reg space###
#if 1
	PRINTSTR("\r\ndisable register space of MEMORY\r\n")
        li  t2,0xbfe00180
        lw  a1,0x0(t2)
        or  a1,a1,0x700 
        xor a1, 0x600
        sw  a1,0x0(t2)
#endif  

	bal	CPU_TLBClear
	nop

/*
 *  RM7000 config register bits.
 */
#define CF_7_SE         (1 << 3)        /* Secondary cache enable */
#define CF_7_SC         (1 << 31)       /* Secondary cache not present */
#define CF_7_TE         (1 << 12)       /* Tertiary cache enable */
#define CF_7_TC         (1 << 17)       /* Tertiary cache not present */
#define CF_7_TS         (3 << 20)       /* Tertiary cache size */
#define CF_7_TS_AL      20              /* Shift to align */
#define NOP8 nop;nop;nop;nop;nop;nop;nop;nop

do_caches:
	PRINTSTR("Sizing caches...\r\n");

	mfc0	t3, C0_CONFIG	/* t3 = original config */
	and	t3, 0xffffeff0		/* Make sure coherency is OK */

	and	t3, ~(CF_7_TE|CF_7_SE|CF_7_TC|CF_7_SC)  /* disable L2/L3 cache */
	mtc0    t3, C0_CONFIG

	PRINTSTR("godson2 caches found\r\n")
        bal     godson2_cache_init
        nop

        mfc0   a0,C0_CONFIG
        and    a0,a0,~((1<<12) | 3)
    	or     a0,a0,2
        mtc0   a0,C0_CONFIG
	
.set noreorder
	PRINTSTR("Cache Init done!\r\n")
.set reorder


	andi	t0, s0, BOOT_CLEAR
	beqz	t0, romWarm

	/* bal	romClearEdac */
	
romWarm:
#if (ROM_TEXT_ADRS == 0x9fc00000)
	/* Switch to cached space so that copying ROM into RAM is faster */
	RELOC(t0,0f)
	and	t0, ~0x20000000
	jal	t0
0:
#endif

	/* Set stack to grow down from beginning of data and call init          */

	la	gp, _gp				/* set global ptr from compiler */
	la	sp, STACK_ADRS-(4*_RTypeSize)	/* set stack to begin of data   */
	PRINTSTR("stack addr : 0x")
	move	a0, sp
	bal	hexserial
	nop
	
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("to call romStart!\r\n")
.set reorder
#endif


	move	a0, s0				/* push arg = start type        */
	sync					/* flush any last-minute writes */
	RELOC(t0, romStart)
	jal	t0				/* starts kernel, never returns */

	bal	sysLedErr			/* Error in starting kernel     */
1:		
	b	1b

		
/***************************************************************************
*
* romReserved -	 Handle a jump to an unknown vector
*
*
* 
*/

	.ent	romReserved
romReserved:
	b	romInit	    /* just start over */
	.end	romReserved


#include "ddr2_config.s"

LEAF(watchdog_init)
watchdog_init:
	move	t5, ra

	GPIOLED_SET(0x0|0x0|0x1)
	li		t0, 0xbfe0011c
	li		t1, 0x08|0x04
	sw		t1, 0x04(t0)

	jr		t5
	nop

END(watchdog_init)

#define MTC0		dmtc0
#define PG_SIZE_16K    			0x00006000
#define COP_0_TLB_PG_MASK		$5
#define COP_0_TLB_HI				$10
#define COP_0_TLB_LO0			$2
#define COP_0_TLB_LO1			$3
#define COP_0_TLB_INDEX			$0

LEAF(CPU_TLBClear)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_16K
	MTC0   a2, COP_0_TLB_PG_MASK   # Whatever...

1:
	MTC0   zero, COP_0_TLB_HI	# Clear entry high.
	MTC0   zero, COP_0_TLB_LO0	# Clear entry low0.
	MTC0   zero, COP_0_TLB_LO1	# Clear entry low1.

	mtc0    a3, COP_0_TLB_INDEX	# Set the index.
	addiu	a3, 1
	li	a2, 64
	nop
	nop
	tlbwi				# Write the TLB

	bne	a3, a2, 1b
	nop

	li a3,4
	mtc0 a3,$22 #flush itlb

	jr	ra
	nop
END(CPU_TLBClear)
