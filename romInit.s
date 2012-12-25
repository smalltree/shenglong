/* romInit.s - CP7000G ROM initialization module */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */

	.data
	.globl	copyright_wind_river

/*
modification history
--------------------
01b,22nov02,zmm  Cleanups after code review.
01a,27sep00,bvn  created 
*/

/*
DESCRIPTION
This module contains the entry code for the VxWorks bootrom.
The entry point romInit, is the first code executed on power-up.

The routine sysToMonitor() jumps to romInit() to perform a
"warm boot".
 
*/
 
#define _ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "cacheLib.h"
#include "config.h"
#include "asm.h"
#include "esf.h"
#include "Lsn2eCpciSBC.h"

#include "bonito64.h"
#include "sdcfg.h"
 

/* RM7000 additions */
#undef Index_Store_Tag_S
#undef Index_Store_Tag_T
#define Index_Store_Tag_S       0xB         	/* 2       3 */
#define Index_Store_Tag_T       0xA         	/* 2       2 */
#define Index_Page_Invalidate_T 0x16         	/* 5       2 */
#define CFG_TE		0x00001000		/* Tertiary cache enable */
#define CFG_SE      0x00000008		/* RM7000 L2 cache enable */
#define CFG_TC		0x00020000		/* Tertiary cache present */

#include "romMipsInit.s"
 
	/* The following flags control what parts of the original startup
	 * code is selected. They also control additional debugging stuff.
	 */

	/* internals */

    .globl  romReboot               /* sw reboot address */
    .globl  romCacheReset           /* sw reboot address */
    .globl  romCacheResetEnd           /* sw reboot address */
	.globl	gtRelocate
	.globl	gtRelocateRAM
	.globl	gtRelocateRAMEnd
	.globl	_bfillLongs		/* Display a progress counter */
	.globl	_bcopyLongs		/* Display a progress counter */

	/* external defs */

	/* romDiags.s contains basic functions that use ONLY t0-t9 and,
	 * are ONLY leaf functions. Thus these do not use the stack.
	 * The following defines are used to track progress of the BootROM.
	 */

/****************************************************************************
*
* sysMemInit - configure memory
*
* This routine actually only relocates the GT-64240.  We leave memory
* configuration to sysCacheInit, which is run immediately after.
*
* Note that, if this is a warm boot, we need to just skip this entire
* process, as the GT-64240 has already been relocated.
*
* RETURNS: N/A
*/
sysMemInit:
/*
#define Index_Store_Tag_D			0x09
#define Index_Invalidate_I			0x00
#define Index_Writeback_Inv_D			0x01
#define Index_Store_Tag_S			0x0b
*/
#define Index_Writeback_Inv_S			0x03

LEAF(godson2_cache_init)
####part 2####
cache_detect_2way:
	mfc0	t4, C0_CONFIG
	andi	t5, t4, 0x0e00
	srl	t5, t5, 9
	andi	t6, t4, 0x01c0
	srl	t6, t6, 6
	addiu	t6, t6, 12
	addiu	t5, t5, 12
	addiu	t4, $0, 1
	sllv	t6, t4, t6
	srl	t6,2
	sllv	t5, t4, t5
	srl	t5,2
	addiu	t7, $0, 2
####part 3####
	lui	a0, 0x8000
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_d2way:
#a0=0x80000000, a1=icache_size, a2=dcache_size
#a3, v0 and v1 used as local registers
	mtc0	$0, C0_TAGHI
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	mtc0	$0, C0_TAGLO
	cache	Index_Store_Tag_D, 0x0(v0)
	mtc0	$0, C0_TAGLO
	cache	Index_Store_Tag_D, 0x1(v0)
	mtc0	$0, C0_TAGLO
	cache   Index_Store_Tag_D, 0x2(v0)
	mtc0	$0, C0_TAGLO
	cache   Index_Store_Tag_D, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20

#if 1
1:
cache_init_l24way:
        mtc0    $0, C0_TAGHI
        addu    v0, $0, a0
        addu    v1, a0, 128*1024
1:      slt     a3, v0, v1
        beq     a3, $0, 1f
        nop
        mtc0    $0, C0_TAGLO
        cache   Index_Store_Tag_S, 0x0(v0)
        mtc0    $0, C0_TAGLO
        cache   Index_Store_Tag_S, 0x1(v0)
        mtc0    $0, C0_TAGLO
        cache   Index_Store_Tag_S, 0x2(v0)
        mtc0    $0, C0_TAGLO
        cache   Index_Store_Tag_S, 0x3(v0)
        beq     $0, $0, 1b
        addiu   v0, v0, 0x20


1:
cache_flush_4way:
	addu	v0, $0, a0
	addu	v1, a0, 128*1024
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_S, 0x0(v0)
	cache	Index_Writeback_Inv_S, 0x1(v0)
	cache	Index_Writeback_Inv_S, 0x2(v0)
	cache	Index_Writeback_Inv_S, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
# endif

1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Invalidate_I, 0x0(v0)
#	cache	Index_Invalidate_I, 0x1(v0)
#	cache	Index_Invalidate_I, 0x2(v0)
#	cache	Index_Invalidate_I, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_d2way:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	cache	Index_Writeback_Inv_D, 0x0(v0)
	cache	Index_Writeback_Inv_D, 0x1(v0)
	cache	Index_Writeback_Inv_D, 0x2(v0)
	cache	Index_Writeback_Inv_D, 0x3(v0)
	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_init_finish:
	nop
	jr	ra
	nop

cache_init_panic:
	TTYDBG("cache init panic\r\n");
1:	b	1b
	nop
	.end	godson2_cache_init
/****************************************************************************
*
* sysCacheInit - initialize and configure devices
*
* While this callback from romMipsInit.s is designed to only initialize
* the caches, the initialization requirements of the CP7000G require us to
* abuse this a little bit.
*
* This routine will program the access modes for the devices on the device
* bus, configure the CPU interface to the GT-64240, perform SDRAM Auto 
* configuration, and then configure the system caches.
*
* RETURNS: N/A
*/
sysCacheInit:

#if COMMENTED
	/* Clear the Bit LED */
	LA(t0, PLD_REG(INTCLR))
	li	t1, INTCLR_BLED
        sb	t1, 0(t0)

	/* Set up the Galileo device BootCS/CS selects */
	/* Boot ROM */
	LA(s2, GT64240_BASE_ADR)
	li	t0, PSWAP(FLASH_GAL_DP)
	sw	t0, FLASH_GAL_BANK(s2)

	/* Flash Disk */
	li	t0, PSWAP(DOC_GAL_DP)
	sw	t0, DOC_GAL_BANK(s2)

	/* UART */
	li	t0, PSWAP(UART_GAL_DP)
	sw	t0, UART_GAL_BANK(s2)

	/* PLD */
	li	t0, PSWAP(PLD_GAL_DP)
	sw	t0, PLD_GAL_BANK(s2)

	/* NVRAM */
	li	t0, PSWAP(NVRAM_GAL_DP)
	sw	t0, NVRAM_GAL_BANK(s2)

	LA(s2, GT64240_BASE_ADR)
        /* Stop PCI automatic retries */
        lw      t1, CPU_CONFIGURATION(s2)
	or	t1, PSWAP(0x20000)
        sw      t1, CPU_CONFIGURATION(s2)

	/* Configure PCI 0 */
	/* PCI Retry Timeout */
	li	t0, PSWAP(GT_PCI0_TOR)
	sw	t0, PCI0_TIMEOUT_RETRY(s2)

	/* Configure PCI 1 */
	/* PCI Retry Timeout */
	li	t0, PSWAP(GT_PCI1_TOR)
	sw	t0, PCI1_TIMEOUT_RETRY(s2)

#if 0
	/* Set the User LED - for testing purposes only */
	LA(t0, PLD_REG(INTSET))
	li	t1, INTSET_ULED
	sb	t1, 0(t0)
#endif

#endif

	/* save the return pointer */
	move	s6, ra

#if COMMENTED
	/* SDRAM Auto Configuration */
	bal	sdramAutoConfig
#endif

#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("Cache 0!\r\n")
.set reorder
#endif
	lui a0,0xa000
	li  a1,512<<10
	move a2,zero
	bal _bfillLongs
	
	/* Initialize caches */
	bal	romCacheReset
	
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("Cache 00!\r\n")
.set reorder
#endif

	/* return */
	move	ra, s6
	j	ra

/****************************************************************************
*
* sysLedErr - display an error message
*
* Since this target can't do any more than just turn on an LED, that's all
* that is done here.
*
* RETURNS: N/A
*/
sysLedErr:
1:
	/* set the Bit LED */
	#LA(t0, PLD_REG(INTSET))
	#li	t1, INTSET_BLED
	#sb	t1, 0(t0)
	b	1b

/****************************************************************************
*
* _bfillLongs - fill a range of memory with a given value
*
* This routine fills the memory region starting at a0 and running for a1
* 32-bit values with the value specified in a2
*
* RETURNS: N/A
*/

	.ent	_bfillLongs
_bfillLongs:
	bltu	a1,8,2f

1:	sw	a2,0(a0)
	sw	a2,4(a0)
	sw	a2,8(a0)
	sw	a2,12(a0)
	sw	a2,16(a0)
	sw	a2,20(a0)
	sw	a2,24(a0)
	sw	a2,28(a0)

	subu	a1,8
	addu	a0,32
	bgeu	a1,8,1b

2:	beqz	a1,2f

1:	subu	a1,1
	sw	a2,0(a0)
	addu	a0,4
	bnez	a1,1b

2:	j	ra

	.end	_bfillLongs


/****************************************************************************
*
* _bcopyLongs - copy data from one location to another
*
* This routine copies data from the address given in a0 to the address
* given in a1, with length specified in a2 (as the number of 32-bit values).
* The routine uses 32-bit loads/stores to be more efficient.
*
* RETURNS: N/A
*/
	.ent	_bcopyLongs
_bcopyLongs:
	bltu	a2,1,2f

1:	lw	t0,0(a0)
	subu	a2,1
	sw	t0,0(a1)
	addu	a0,4
	addu	a1,4
	bgeu	a2,1,1b

2:	j	ra

	.end	_bcopyLongs

/*******************************************************************************
*
* romCacheReset - low level initialization of the RM7000 primary caches
*
* This routine initializes the RM7000 primary caches to ensure that they
* have good parity.  It must be called by the ROM before any cached locations
* are used to prevent the possibility of data with bad parity being written to
* memory.
* To initialize the instruction cache it is essential that a source of data
* with good parity is available. If the initMem argument is set, this routine
* will initialize an area of memory starting at location zero to be used as
* a source of parity; otherwise it is assumed that memory has been
* initialized and already has good parity.
*
* RETURNS: N/A
*

* void romCacheReset (initMem)

*/
	.ent	romCacheReset
romCacheReset:
	move	s5,ra
	/*
 	 * First work out the sizes
	 */
	/* The config register contains the primary cache sizes */
	mfc0	t0,C0_CONFIG
	HAZARD_CP_READ

	/* Calculate the primary instruction cache size */
	and	t1,t0,CFG_ICMASK
	srl	t1,CFG_ICSHIFT
	li	s1,0x1000
	sll	s1,t1			/* 2^(12+IC) */

	/* Calculate the primary data cache size */
	and	t1,t0,CFG_DCMASK
	srl	t1,CFG_DCSHIFT
	li	s2,0x1000
	sll	s2,t1			/* 2^(12+DC) */
#if COMMENTED
	/* Calculate external (RM7000 L3) cache size */
	move	s3,zero			/* default to no cache */

        /* Check the PLD Register */
        LA(t1, PLD_BASE_ADR)
        lb      t1, BOARD_STAT(t1)      /* Board Status Register */
        and     t1, BOARD_L3_MASK

        li      t5, BOARD_L3_NONE       /* None On board ? */
        beq     t1, t5, l3_none		/* No L3 */

	li	t5, BOARD_L3_2MB
        beq     t1, t5, l3_2mb		/* 2MB L3 */

	li	t5, BOARD_L3_4MB
        beq     t1, t5, l3_4mb		/* 4MB L3 */

	li	s3, 0x800000		/* Must be 8MB */
	b	l3_none
l3_2mb:
	li	s3, 0x200000		/* 2MB */
	b	l3_none
l3_4mb:
	li	s3, 0x400000		/* 4MB */
l3_none:
#endif
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("Cache 1!\r\n")
.set reorder
#endif
	/* move everything into a nice parameter format */
	move t2, s1  #t2:icache size
	li	 t4, 32  #t4:icache line size
	move t3, s2  #t3:dcache size
	li	 t5, 32  #t5:dcache line size
#undef L2_CACHE_SIZE
#define L2_CACHE_SIZE	(512*1024)
	li	 t6, L2_CACHE_SIZE #t6:scache size
	li	 t7, 32            #t7:scache line size
	
#if COMMENTED
	move	t6, s3
	li	t7, 32
#endif

	/* reset the caches */
	RELOC(v0, cacheLsn2eReset)
	move	a0, s0
	jal	v0
	
#if COMMENTED
	/* Enable the L3 cache */
	LA(t0, GT64240_BASE_ADR)
	lw	t1, CPU_CONFIGURATION(t0)
	or	t1, PSWAP(0x4000)
	sw	t1, CPU_CONFIGURATION(t0)
#endif
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("Cache 11!\r\n")
.set reorder
#endif
	move	ra,s5
	j	ra

	.end	romCacheReset
romCacheResetEnd:

/*******************************************************************************
*
* romClearEdac - clear error detection and correction logic
*
* This routine clears the memory and error detection logic by
* doing word writes to each DRAM location.  This only does a small
* portion of memory, enough to start code.   sdramDetect.c
* completes this process.
*/

	.ent	romClearEdac
romClearEdac:

	move s5,ra		/* save return address */
	
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("enter romClearEdac!\r\n")
.set reorder
#endif

	mfc0	v1,C0_SR	/* disable parity errors */
	HAZARD_CP_READ
	or	v0,v1,SR_DE
    mtc0    v0,C0_SR
    HAZARD_CP_WRITE
	
	/* start from minimum configuration */
    li  a0, DRAM_NONCACHE_VIRT_BASE
	li	a1, DRAM_NONCACHE_VIRT_BASE + (RAM_HIGH_ADRS & ~KSU_MASK) + ROM_SIZE
clearloop:
	sd	  zero, 0(a0)
	addu  a0, 8
	bne	  a0, a1, clearloop
done:
	mtc0	v1,C0_SR 
    HAZARD_INTERRUPT

    move ra,s5
    j    ra
	.end	romClearEdac
 

	.ent romExcHandleTlbMiss
romExcHandleTlbMiss:
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("TLBMISS!\r\n")

	PRINTSTR("\r\ns7=")
	move	a0, s7
	HAZARD_CP_READ
	bal	hexserial
	nop
	
	PRINTSTR("\r\nSTATUS=")
	mfc0	a0, C0_SR
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nCAUSE=")
	mfc0	a0, C0_CAUSE
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nCONFIG=")
	mfc0	a0, C0_CONFIG
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nERRORPC=")
	mfc0	a0, C0_ERRPC	
	HAZARD_CP_READ
	bal	hexserial
	nop
	
	PRINTSTR("\r\n")
.set reorder
#endif	
	.end romExcHandleTlbMiss
/*******************************************************************************
*
* romExcHandle - rom based exception/interrupt handler
*
* This routine is invoked on an exception or interrupt while
* the status register is using the bootstrap exception vectors.
* It saves a state frame to a known uncached location so someone
* can examine the data over the VME.  It also displays a summary of the
* error on the boards alphanumeric display.
*
* THIS ROUTIINE IS NOT CALLABLE FROM "C"
*
*/

#define	ROM_ISP_BASE	0xa0100000

	.ent	romExcHandle
romExcHandle:
	PRINTSTR("EXCPTION : romExcHandle\r\n")
#ifdef TTYDBGROM
.set noreorder
	PRINTSTR("\r\ns7=")
	move	a0, s7
	HAZARD_CP_READ
	bal	hexserial
	nop


	PRINTSTR("\r\nSTATUS=")
	mfc0	a0, C0_SR
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nCAUSE=")
	mfc0	a0, C0_CAUSE
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nCONFIG=")
	mfc0	a0, C0_CONFIG
	HAZARD_CP_READ
	bal	hexserial
	nop

	PRINTSTR("\r\nERRORPC=")
	mfc0	a0, C0_ERRPC	
	HAZARD_CP_READ
	bal	hexserial
	nop
	
	PRINTSTR("\r\n")
.set reorder
#endif
	.end    romExcHandle            /* that's all folks */

  






