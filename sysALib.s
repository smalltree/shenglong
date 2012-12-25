/* sysALib.s - cp7000 system-dependent assembly routines */

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
01m,16jul01,agf  revalidation code review results
01n,07dec01,tlc  Correct diab syntax errors.
01m,16jul01,agf  add CoE notice
01l,18jun01,agf  Integrate with MIPS BSP commonization
01k,13jun01,agf  remove unused .global decl's
01j,01jun01,agf  add IPL Set/Get functions
01i,12apr01,agf  porting extended interrupts to use T2.1 library calls
01h,07apr97,kkk  do not set SR_FR bit for R4700 in sysInit.
01g,04mar97,kkk  renamed sys[SG]etCompare() to sysCompare[SG]et().
		 renamed sys[SG]Count() to sysCount[SG]().
01f,29nov96,kkk  made sys[SG]et*() routines primitive.
01e,25nov96,kkk  disable sysClearTlbEntry for R4650.
01d,25nov96,kkk  changed ifndef R4650 to if (CPU != R4650).
01c,18apr96,rml  avoid clearing TLB for 4650
01b,06jun94,caf  changed "4*RTypeSize" to "32".
01a,27sep93,cd   created.
*/

/*
DESCRIPTION
This module contains system-dependent routines written in assembly
language.

This module must be the first specified in the \f3ld\f1 command used to
build the system.  The sysInit() routine is the system start-up code.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "config.h"
#include "asm.h"
#include "Lsn2eCpciSBC.h"

#include "esf.h"
#if 0 /*commented by wangfq*/
#define C1_ICR		$20             /* interrupt control register  */
#define C1_IPLHI	$19             /* interrupt priority level hi */
#define C1_IPLLO	$18             /* interrupt priority level lo */
#endif /*end commented*/



	/* internals */

	.globl	sysInit			/* start of system code */
	.globl	sysClearTlbEntry	/* clear entry in tlb */
	.globl	sysSetTlbEntry		/* Set entry in tlb */
	.globl	sysGetTlbEntry		/* Set entry in tlb */
	.globl	sysWbFlush		/* flush write buffers */
	.globl	sysFpaDeMux		/* which FPA int is bugging us */
	.globl	sysFpaAck		/* clear FPA interrupt */
	.globl	sysPridGet
#if 0 /*commented by wangfq*/
	.globl  sysICRSet
	.globl  sysICRGet
	.globl  sysIPLHiSet
	.globl  sysIPLHiGet
	.globl  sysIPLLoSet
	.globl  sysIPLLoGet
#endif /*end commented*/

	.globl  sysWiredSet
	.globl  sysWiredGet
	.globl  sysTlbMaskSet
	.globl  sysTlbMaskGet
	.globl  sysTlbHISet
	.globl  sysTlbHIGet
	.globl  sysTlbLO0Set
	.globl  sysTlbLO0Get
	.globl  sysTlbLO1Set
	.globl  sysTlbLO1Get
	.globl  sysTlbIdxSet
	.globl  sysTlbIdxGet
	.globl  sysTlbWriteIdx
	.globl  sysTlbWriteRand
	.globl  sysTlbReadIdx
	.globl  sysTlbProbe
	.globl  sysClearMem

	/* externals */

	.globl	usrInit		/* system initialization routine */
	.text


/*******************************************************************************
*
* sysInit - start after boot
*
* This routine is the system start-up entry point for VxWorks in RAM, the
* first code executed after booting.  It disables interrupts, sets up the
* stack, and jumps to the C routine usrInit() in usrConfig.c.
*
* The initial stack is set to grow down from the address of sysInit().  This
* stack is used only by usrInit() and is never used again.  Memory for the
* stack must be accounted for when determining the system load address.
*
* NOTE: This routine should not be called by the user.
*
* RETURNS: N/A
*
* sysInit (void)	/@ THIS IS NOT A CALLABLE ROUTINE @/
*
* NOMANUAL
*/

	.ent	sysInit
sysInit:
	la	gp,_gp			/* set global ptr from cmplr */

	/* disable all interrupts, fpa usable */
	.set noreorder

	mfc0	t1, C0_PRID
	li	t0, SR_CU1|SR_CU0|SR_IE/*|0x4400*/
	bne	t1, 0xa11, 1f
	or	t0, SR_DE		/* disable cache errors on R4200 rev1.1 */
1:	
	mtc0	t0, C0_SR		/* put on processor	*/
	la	sp, sysInit-32		/* set stack to grow down from code,

					    leave room for four parameters */

    .set reorder
    
	/* give us as long as possible before a clock interrupt */
	li	v0,1
	mtc0	v0,C0_COUNT
	mtc0	zero,C0_COMPARE

	 	
	jal	sysClearTlb		/* make sure tlb is invalid */
	
/*	nop
	li a0,0xc0000000
	li a1,0x20000000
	li a2,0x10000000
	li a3,TLBLO_D
	jal sysSetTlb*/
	
	
li a0,'%'
bal tgt_putchar
nop

/* for hexserial */
#ifndef RELOC
#define	RELOC(toreg,address) \
	bal	9f; \
9:; \
	la	toreg,address; \
	addu	toreg,ra; \
	la	ra,9b; \
	subu	toreg,ra
#endif
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; .set reorder; RELOC(a0,98b); .set noreorder; bal stringserial; nop
#define PRINTSTR(x) TTYDBG(x)


PRINTSTR("\r\nCAUSE=")
mfc0	a0, C0_CAUSE
HAZARD_CP_READ
bal	hexserial
nop

/*PRINTSTR("\r\nSR=")
mfc0 a0,C0_SR
HAZARD_CP_READ
bal hexserial
nop

PRINTSTR("\r\n")*/

    
	li	a0, BOOT_WARM_AUTOBOOT /* BOOT_COLD push start type arg = WARM_BOOT */
	/* PRINTSTR("in sysInit\r\n") */
	jal	usrInit			/* never returns - starts up kernel */
	li	ra, R_VEC		/* load prom reset address */
	j	ra			/* just in case */
	.end	sysInit

/*******************************************************************************
*
* sysWbFlush - flush the write buffer
*
* This routine flushes the write buffers, making certain all subsequent
* memory writes have occurred.  It is used during critical periods only, e.g.,
* after memory-mapped I/O register access.
*
* RETURNS: N/A
*
* sysWbFlush (void)
*
* NOMANUAL
*/
	.ent	sysWbFlush
sysWbFlush:
	.set noreorder
	nop
	#if 0 /*wangfq*/
        la      v0,sysWbFlush
        or      v0,DRAM_NONCACHE_VIRT_BASE
        sw      zero,0(v0)
        lw      v0,0(v0)
    #endif 
    	sync /*wangfq*/
    	nop
        j       ra
	nop
	.set reorder
	.end	sysWbFlush

/*******************************************************************************
*
* sysClearTlbEntry - clear translation lookaside buffer entry
*
* This routine clears a specified translation lookaside buffer (TLB)
* entry by writing a zero to the virtual page number and valid bit.
*
* RETURNS: N/A
*
* void sysClearTlbEntry
*     (
*     int entry
*     )
*
*/

	.ent	sysClearTlbEntry
sysClearTlbEntry:
	subu	t0, a0, TLB_ENTRIES - 1 /* how many tlb entries are there */
	bgtz	t0, invalidEntry	/* is my index bad ? */
	li	t2,DRAM_CACHE_VIRT_BASE&TLBHI_VPN2MASK
	mtc0	t2,C0_TLBHI		/* zero tlbhi entry */
	mtc0	zero,C0_TLBLO0		/* set valid bit to zero */
	mtc0	zero,C0_TLBLO1		/* set valid bit to zero */
	mtc0	a0,C0_INX		/* set up index for write     */
	tlbwi				/* write indexed tlb entry */
invalidEntry:
	j	ra
	.end	sysClearTlbEntry

/*******************************************************************************
*
* sysGetTlbEntry - Translation lookaside buffer entry
*
* RETURNS: N/A
*
* void sysGetTlbEntry
*     (
*     UINT entry,
*     UINT *mask,
*     ULONG *TLBLO[01],
*     ULONG *TLBHI		 
*     )
*/

#define TLBLO_PAGEMASK	0x01ffe000
	.ent	sysGetTlbEntry
sysGetTlbEntry:
	subu	t0, a0, TLB_ENTRIES - 1 /* how many tlb entries are there */
	bgtz	t0, badEntry2		/* is my index bad ? */
	
	/* Set the index into the TLB */
	mtc0	a0,C0_INX		

 	/* Read the TLB entries */
	tlbr				/* read entry */

	/* Get the page size mask */
	mfc0	t0, C0_PAGEMASK		
	sw	t0, 0(a1)

	/* Get the LO address */
	mfc0	t0, C0_TLBLO0		
	sw	t0, 0(a2)

	/* Get the HI address */
	mfc0	t0, C0_TLBHI		/* zero tlbhi entry */
	sw	t0, 0(a3)
	li	v0, OK
	b	goodEntry2

badEntry2:
	li	v0, ERROR
goodEntry2:

	j	ra
	.end	sysGetTlbEntry

/*******************************************************************************
*
* sysSetTlbEntry - Translation lookaside buffer entry
*
* RETURNS: N/A
*
* STATUS sysSetTlbEntry
*     (
*     UINT entry,
*     ULONG virtual,
*     ULONG physical,
*     UINT mode		 Bits 31:12 size, bits 5:0 is cache mode/enable
*     )
*/

	.ent	sysSetTlbEntry
sysSetTlbEntry:
	subu	t0, a0, TLB_ENTRIES - 1 /* how many tlb entries are there */
	bgtz	t0, badEntry		/* is my index bad ? */

	and	t1, a3, ~TLBLO_PAGEMASK_SIZE/* Check for invalid size */
	bgtz	t1, badEntry		/* Invalid size parameter ? */
	and	t0, a3, (TLBLO_PAGEMASK_SIZE & ~TLBLO_MODE)
	sll	t0, 1
	sub	t0, 1

	/* Set the page size mask */
	and	t0, TLBLO_PAGEMASK
	mtc0	t0, C0_PAGEMASK		

	/* Set the virtual address */
	and	t0, a1, TLBHI_VPN2MASK	/* ASID not used   */
	dmtc0	t0, C0_TLBHI		/* zero tlbhi entry */
	
	/* Set physical address, LO0 register */
	srl	t0, a2, 6			/* Physical address */
	and	t0, TLBLO_PFNMASK
	move	t1, a3			/* Caching characteristic */
	and	t1, TLBLO_MODE
	or	t0, t0, t1
	dmtc0	t0, C0_TLBLO0		/* set valid bit to zero */

	/* Set physical address, LO1 register */
	add	t0, a2, a3		/* Next physical address */
	srl	t0, 6
	and	t0, TLBLO_PFNMASK
	or	t0, t0, t1
	dmtc0	t0, C0_TLBLO1		/* set valid bit to zero */

	/* Set the index into the TLB */
	dmtc0	a0,C0_INX		

 	/* Write the TLB entries */
	tlbwi				/* write indexed tlb entry */
	li	v0, OK
	b	goodEntry

badEntry:
	li	v0, ERROR
goodEntry:
	j	ra
	.end	sysSetTlbEntry


/*******************************************************************************
*
* sysFpaDeMux - determine which FPA exception is pending
*
* This routine reads the floating point unit (FPU) status to determine which
* FPU exception generated an interrupt to the processor.  It returns an
* index to the vector table.
*
* This routine is loaded into the static interrupt priority table.
* It is called by jumping to the address in this table, not by
* user calls.
*
* RETURNS: An interrupt vector.
*
* int sysFpaDeMux
*     (
*     int vecbase	/@ base location of FPA vectors in excBsrTbl @/
*     )
*
*/

	.ent	sysFpaDeMux
sysFpaDeMux:
	.set	noreorder
	cfc1	v0, C1_SR			/* grab FPA status	*/
	nop
	.set	reorder
	li	a2, FP_EXC_MASK			/* load cause mask	*/
	and	a2, v0				/* look at cause only	*/
	srl	a2, FP_EXC_SHIFT		/* place cause in lsb	*/
	li	a1, FP_ENABLE_MASK		/* load enable mask	*/
	and	a1, a1, v0			/* look at enable only	*/
	srl	a1, FP_ENABLE_SHIFT		/* place enabled in lsb	*/
	li	a3, (FP_EXC_E>>FP_EXC_SHIFT)	/* ld unimp op bit	*/
	or	a1, a3				/* no mask bit for this	*/
	and	a2, a1				/* look at just enabled	*/
	and	v1, v0, ~FP_EXC_MASK		/* clear the exceptions */
	lbu	v0, ffsLsbTbl(a2)		/* lkup first set bit	*/
	addu	v0, a0				/* increment io vector	*/
	ctc1	v1, C1_SR			/* clear fp condition	*/
	j	ra				/* return to caller	*/
	.end	sysFpaDeMux

/*******************************************************************************
*
* sysFpaAck - acknowledge a floating point unit interrupt
*
* This routine writes the floating point unit (FPU) status register to
* acknowledge the appropriate FPU interrupt.  It returns an index to the vector
* table.
*
* RETURNS: An interrupt vector.
*
* int sysFpaAck (void)
*
*/

	.ent	sysFpaAck
sysFpaAck:
	cfc1	v0, C1_SR		/* read control/status reg	*/
	and	t0, v0, ~FP_EXC_MASK	/* zero bits		*/
	ctc1	t0, C1_SR		/* acknowledge interrupt	*/
	j	ra			/* return to caller		*/
	.end	sysFpaAck

#if 0 /*commented by wangfq*/
/*******************************************************************************
*
* sysICRSet - set the RM7000 CP1 Interrupt Control Register
*
* RETURNS: N/A
*
* void sysICRSet (void)
*/

	.ent	sysICRSet
sysICRSet:
	ctc0	a0,C1_ICR
	j	ra
	.end	sysICRSet

/*******************************************************************************
*
* sysICRGet - get the RM7000 CP1 Interrupt Control Register
*
* RETURNS: N/A
*
* UINT sysICRGet (void)
*/

	.ent	sysICRGet
sysICRGet:
	cfc0	v0,C1_ICR
	j	ra
	.end	sysICRGet

/*******************************************************************************
*
* sysIPLHiSet - set the RM7000 CP1 Interrupt Priority Hi Register
*
* RETURNS: N/A
*
* void sysIPLHiSet (void)
*/

	.ent	sysIPLHiSet
sysIPLHiSet:
	ctc0	a0,C1_IPLHI
	j	ra
	.end	sysIPLHiSet

/*******************************************************************************
*
* sysIPLHiGet - get the RM7000 CP1 Interrupt Priority Hi Register
*
* RETURNS: N/A
*
* UINT sysIPLHiGet (void)
*/

	.ent	sysIPLHiGet
sysIPLHiGet:
	cfc0	v0,C1_IPLHI
	j	ra
	.end	sysIPLHiGet

/*******************************************************************************
*
* sysIPLLoSet - set the RM7000 CP1 Interrupt Priority Lo Register
*
* RETURNS: N/A
*
* void sysIPLLoSet (void)
*/

	.ent	sysIPLLoSet
sysIPLLoSet:
	ctc0	a0,C1_IPLLO
	j	ra
	.end	sysIPLLoSet

/*******************************************************************************
*
* sysIPLLoGet - get the RM7000 CP1 Interrupt Priority Lo Register
*
* RETURNS: N/A
*
* UINT sysIPLLoGet (void)
*/

	.ent	sysIPLLoGet
sysIPLLoGet:
	cfc0	v0,C1_IPLLO
	j	ra
	.end	sysIPLLoGet

#endif /*end commented by wangfq*/

/*******************************************************************************
*
* sysWiredSet - set the R4000 CP1 TLB Wired Register
*
* RETURNS: N/A
*
* void sysWiredSet (void)
*/

	.ent	sysWiredSet
sysWiredSet:
	mtc0	a0,C0_WIRED
	j	ra
	.end	sysWiredSet

/*******************************************************************************
*
* sysWiredGet - get the R4000 CP0 TLB Wired Register
*
* RETURNS: N/A
*
* UINT sysWiredGet (void)
*/

	.ent	sysWiredGet
sysWiredGet:
	mfc0	v0,C0_WIRED
	j	ra
	.end	sysWiredGet

/*******************************************************************************
*
* sysTlbMaskSet - set the R4000 CP1 TLB TlbMask Register
*
* RETURNS: N/A
*
* void sysTlbMaskSet (UINT regVal)
*/

	.ent	sysTlbMaskSet
sysTlbMaskSet:
	mtc0	a0,C0_PAGEMASK
	j	ra
	.end	sysTlbMaskSet

/*******************************************************************************
*
* sysTlbMaskGet - get the R4000 CP0 TLB TlbMask Register
*
* RETURNS: N/A
*
* UINT sysTlbMaskGet (void)
*/

	.ent	sysTlbMaskGet
sysTlbMaskGet:
	mfc0	v0,C0_PAGEMASK
	j	ra
	.end	sysTlbMaskGet

/*******************************************************************************
*
* sysTlbHISet - set the R4000 CP1 TLB TlbHI Register
*
* RETURNS: N/A
*
* void sysTlbHISet (UINT regVal)
*/

	.ent	sysTlbHISet
sysTlbHISet:
	mtc0	a0,C0_TLBHI
	j	ra
	.end	sysTlbHISet

/*******************************************************************************
*
* sysTlbHIGet - get the R4000 CP0 TLB TlbHI Register
*
* RETURNS: N/A
*
* UINT sysTlbHIGet (void)
*/

	.ent	sysTlbHIGet
sysTlbHIGet:
	mfc0	v0,C0_TLBHI
	j	ra
	.end	sysTlbHIGet

/*******************************************************************************
*
* sysTlbLO0Set - set the R4000 CP1 TLB TlbLO0 Register
*
* RETURNS: N/A
*
* void sysTlbLO0Set (UINT regVal)
*/

	.ent	sysTlbLO0Set
sysTlbLO0Set:
	mtc0	a0,C0_TLBLO0
	j	ra
	.end	sysTlbLO0Set

/*******************************************************************************
*
* sysTlbLO0Get - get the R4000 CP0 TLB TlbLO0 Register
*
* RETURNS: N/A
*
* UINT sysTlbLO0Get (void)
*/

	.ent	sysTlbLO0Get
sysTlbLO0Get:
	mfc0	v0,C0_TLBLO0
	j	ra
	.end	sysTlbLO0Get

/*******************************************************************************
*
* sysTlbLO1Set - set the R4000 CP1 TLB TlbLO1 Register
*
* RETURNS: N/A
*
* void sysTlbLO1Set (void)
*/

	.ent	sysTlbLO1Set
sysTlbLO1Set:
	mtc0	a0,C0_TLBLO1
	j	ra
	.end	sysTlbLO1Set

/*******************************************************************************
*
* sysTlbLO1Get - get the R4000 CP0 TLB TlbLO1 Register
*
* RETURNS: N/A
*
* UINT sysTlbLO1Get (void)
*/

	.ent	sysTlbLO1Get
sysTlbLO1Get:
	mfc0	v0,C0_TLBLO1
	j	ra
	.end	sysTlbLO1Get

/*******************************************************************************
*
* sysTlbIdxSet - set the R4000 CP1 TLB TlbIdx Register
*
* RETURNS: N/A
*
* void sysTlbIdxSet (UINT regVal)
*/

	.ent	sysTlbIdxSet
sysTlbIdxSet:
	mtc0	a0,C0_INX
	j	ra
	.end	sysTlbIdxSet

/*******************************************************************************
*
* sysTlbIdxGet - get the R4000 CP0 TLB TlbIdx Register
*
* RETURNS: N/A
*
* UINT sysTlbIdxGet (void)
*/

	.ent	sysTlbIdxGet
sysTlbIdxGet:
	mfc0	v0,C0_INX
	j	ra
	.end	sysTlbIdxGet

/*******************************************************************************
*
* sysTlbWriteIdx - Initiate Coprocessor 0 TLB Write Index operation
*
* RETURNS: N/A
*
* void sysTlbWriteIdx (void)
*/

	.ent	sysTlbWriteIdx
sysTlbWriteIdx:
	tlbwi
	j	ra
	.end	sysTlbWriteIdx


/*******************************************************************************
*
* sysTlbWriteRand - Initiate Coprocessor 0 TLB Write Random operation
*
* RETURNS: N/A
*
* void sysTlbWriteRand (void)
*/

	.ent	sysTlbWriteRand
sysTlbWriteRand:
	tlbwr
	j	ra
	.end	sysTlbWriteRand

/*******************************************************************************
*
* sysTlbReadIdx - Initiate Coprocessor 0 TLB Read Index operation
*
* RETURNS: N/A
*
* void sysTlbReadIdx (void)
*/

	.ent	sysTlbReadIdx
sysTlbReadIdx:
	tlbr
	j	ra
	.end	sysTlbReadIdx


/*******************************************************************************
*
* sysTlbProbe - Initiate Coprocessor 0 TLB Probe operation
*
* RETURNS: N/A
*
* void sysTlbProbe (void)
*/

	.ent	sysTlbProbe
sysTlbProbe:
	tlbp
	j	ra
	.end	sysTlbProbe


/*******************************************************************************
*
* sysClearMem - 
*
* RETURNS: N/A
*
* void sysClearMem (ULONG baseAddr, UINT numDoubles)
*/
	.ent	sysClearMem
	.set	noreorder
sysClearMem:
 	move	t0, a0
	move	t1, a1
1:
	sd	zero,0(t0)
	sub	t1, 1
	bgtz	t1, 1b
	add	t0, t0, 8
	.set	reorder

	j	ra
	.end	sysClearMem


/* Include Generic MIPS support code */
#include "sysMipsALib.s"
#if 0
#include "excALib.s"
#endif

