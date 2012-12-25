/* sysMipsALib.s - MIPS system-dependent routines */

/* Copyright 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

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
01f,07jun02,jmt  Fix typo with SYS_CONFIG_SET and SYS_PRID_GET macros
01e,29nov01,agf  fix sysConfigSet to load from a0 instead of v0
01e,29nov01,pes  Correct parameter passed to sysConfigSet()
01d,16jul01,tlc  Add CofE copyright.
01c,27jun01,tlc  General cleanup.
01b,21jun01,agf  fix typos in comments
01a,15jun01,tlc  Use HAZARD_VR5400 macro.
*/

/*
DESCRIPTION
This library provides board-specific routines that are shared by all MIPS-based
BSPs.  MIPS BSPs utilize this file by creating a symbolic link from their
directory to target/config/mipsCommon/sysMipsALib.s and include the file at the 
*bottom* of sysALib.s using

	#include "sysMipsALib.s"
	
A list of provided routines follows.  If a BSP requires a specialized routine,
then #define the appropriate MACRO corresponding to the routine to be
specialized in the BSPs sysALib.s file.

	 ROUTINE		  MACRO
	------------------------------------------------------
	sysGpInit		SYS_GP_INIT
	sysCompareSet		SYS_COMPARE_SET
	sysCompareGet		SYS_COMPARE_GET
	sysCountSet		SYS_COUNT_SET
	sysCountGet		SYS_COUNT_GET
	sysPridGet		SYS_PRID_GET
	sysConfigGet		SYS_CONFIG_GET
	sysConfigSet		SYS_CONFIG_SET
*/
	
	.globl	sysGpInit
	.globl  sysCompareSet
	.globl  sysCompareGet
	.globl  sysCountSet
	.globl	sysCountGet
	.globl	sysPridGet
	.globl	sysConfigGet
	.globl	sysConfigSet
	
	.text

#ifndef SYS_GP_INIT
/*******************************************************************************
*
* sysGpInit - initialize the MIPS global pointer
*
* The purpose of this routine is to initialize the global pointer (gp).
* It is required in order support compressed ROMs.
*
* RETURNS: N/A
*
* NOMANUAL
*/

	.ent	sysGpInit
sysGpInit:
	la	gp, _gp			/* set global pointer from compiler */
	j	ra
	.end	sysGpInit

#endif

#ifndef SYS_COMPARE_SET	
/******************************************************************************
*
* sysCompareSet - set the MIPS timer compare register
*
* RETURNS: N/A

* void sysCompareSet
*     (
*     int compareValue
*     )

* NOMANUAL
*/

	.ent	sysCompareSet
sysCompareSet:
	HAZARD_VR5400
	mtc0	a0,C0_COMPARE
	j	ra
	.end	sysCompareSet
#endif

#ifndef SYS_COMPARE_GET	
/******************************************************************************
*
* sysCompareGet - get the MIPS timer compare register
*
* RETURNS: The MIPS timer compare register value

* int sysCompareGet (void)

* NOMANUAL	
*/

	.ent	sysCompareGet
sysCompareGet:
	HAZARD_VR5400
	mfc0	v0,C0_COMPARE
	j	ra
	.end	sysCompareGet
#endif

#ifndef SYS_COUNT_SET		
/******************************************************************************
*
* sysCountSet - set the MIPS timer count register
*
* RETURNS: N/A

* void sysCountSet
*     (
*     int countValue
*     )

* NOMANUAL	
*/

	.ent	sysCountSet
sysCountSet:
	HAZARD_VR5400
	mtc0	a0,C0_COUNT
	j	ra
	.end	sysCountSet
#endif

#ifndef SYS_COUNT_GET
/******************************************************************************
*
* sysCountGet - get the MIPS timer count register
*
* RETURNS: The MIPS timer count register value
*
* int sysCountGet (void)
*
* NOMANUAL	
*/

	.ent	sysCountGet
sysCountGet:
	HAZARD_VR5400
	mfc0	v0,C0_COUNT
	j	ra
	.end	sysCountGet
#endif
	
#ifndef SYS_PRID_GET
/******************************************************************************
*
* sysPridGet - get the MIPS processor ID register
*
* RETURNS: N/A

* int sysPridGet (void)

*/
	.ent	sysPridGet
sysPridGet:
	HAZARD_VR5400
	mfc0	v0,C0_PRID
	j	ra
	.end	sysPridGet
#endif

#ifndef SYS_CONFIG_GET				
/******************************************************************************
*
* sysConfigGet - get the MIPS processor CONFIG register
*
* RETURNS: N/A

* int sysConfigGet (void)

*/
	.ent	sysConfigGet
sysConfigGet:
	HAZARD_VR5400
	mfc0	v0,C0_CONFIG
	j	ra
	.end	sysConfigGet
#endif

#ifndef SYS_CONFIG_SET
/******************************************************************************
*
* sysConfigSet - set the MIPS processor CONFIG register
*
* RETURNS: N/A

* int sysConfigSet (void)

*/
	.ent	sysConfigSet
sysConfigSet:
	HAZARD_VR5400
	mtc0	a0,C0_CONFIG
	j	ra
	.end	sysConfigSet
#endif

/*********************************************************************
 *********************************************************************
 *********************************************************************/
#define TTYDBGRAM

#ifdef TTYDBGRAM
#include "./includex/i82371eb.h"
#include "./includex/sbd.h"
#include "./includex/isapnpreg.h"
#include "./includex/i8254.h"
#include "./includex/pc97307.h"

#include "sdcfg.h"

.set noreorder
	#define COM1_BASE_VADDR	0xbe000000/*(0xbfd00000+COM1_BASE_ADDR)*/
	#define COM2_BASE_VADDR	(0xbfd00000+COM2_BASE_ADDR)
#ifdef HAVE_NB_SERIAL
	#define COM3_BASE_VADDR	(0xbfd00000+COM3_BASE_ADDR)
#endif

#define CONS_BAUD 115200
#define	NS16550HZ 3686400

#define	NS16550_DATA	0
#define	NS16550_IER	    1
#define	NS16550_IIR	    2
#define	NS16550_FIFO	2
#define	NS16550_CFCR	3
#define	NS16550_MCR	    4
#define	NS16550_LSR	    5
#define	NS16550_MSR	    6	
#define	NS16550_SCR	    7

/* fifo control register */
#define	FIFO_ENABLE	0x01	/* enable fifo */
#define	FIFO_RCV_RST	0x02	/* reset receive fifo */
#define	FIFO_XMT_RST	0x04	/* reset transmit fifo */
#define	FIFO_DMA_MODE	0x08	/* enable dma mode */
#define	FIFO_TRIGGER_1	0x00	/* trigger at 1 char */
#define	FIFO_TRIGGER_4	0x40	/* trigger at 4 chars */
#define	FIFO_TRIGGER_8	0x80	/* trigger at 8 chars */
#define	FIFO_TRIGGER_14	0xc0	/* trigger at 14 chars */

/* character format control register */
#define	CFCR_DLAB	0x80	/* divisor latch */
#define	CFCR_SBREAK	0x40	/* send break */
#define	CFCR_PZERO	0x30	/* zero parity */
#define	CFCR_PONE	0x20	/* one parity */
#define	CFCR_PEVEN	0x10	/* even parity */
#define	CFCR_PODD	0x00	/* odd parity */
#define	CFCR_PENAB	0x08	/* parity enable */
#define	CFCR_STOPB	0x04	/* 2 stop bits */
#define	CFCR_8BITS	0x03	/* 8 data bits */
#define	CFCR_7BITS	0x02	/* 7 data bits */
#define	CFCR_6BITS	0x01	/* 6 data bits */
#define	CFCR_5BITS	0x00	/* 5 data bits */

/* modem control register */
#define	MCR_LOOPBACK	0x10	/* loopback */
#define	MCR_IENABLE	0x08	/* output 2 = int enable */
#define	MCR_DRS		0x04	/* output 1 = xxx */
#define	MCR_RTS		0x02	/* enable RTS */
#define	MCR_DTR		0x01	/* enable DTR */

/* line status register */
#define	LSR_RCV_FIFO	0x80	/* error in receive fifo */
#define	LSR_TSRE	0x40	/* transmitter empty */
#define	LSR_TXRDY	0x20	/* transmitter ready */
#define	LSR_BI		0x10	/* break detected */
#define	LSR_FE		0x08	/* framing error */
#define	LSR_PE		0x04	/* parity error */
#define	LSR_OE		0x02	/* overrun error */
#define	LSR_RXRDY	0x01	/* receiver ready */
#define	LSR_RCV_MASK	0x1f

/* modem status register */
#define	MSR_DCD		0x80	/* DCD active */
#define	MSR_RI		0x40	/* RI  active */
#define	MSR_DSR		0x20	/* DSR active */
#define	MSR_CTS		0x10	/* CTS active */
#define	MSR_DDCD	0x08    /* DCD changed */
#define	MSR_TERI	0x04    /* RI  changed */
#define	MSR_DDSR	0x02    /* DSR changed */
#define	MSR_DCTS	0x01    /* CTS changed */

#ifndef NSREG
#define NSREG(x)	x
#endif

LEAF(initserial)
#ifdef HAVE_NB_SERIAL

	la	v0, COM3_BASE_VADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
 	li	v1, NS16550HZ/(16*CONS_BAUD)
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	li	v1, MCR_DTR|MCR_RTS
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
#endif
	la	v0, COM1_BASE_VADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	nop
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
	nop
 	li	v1, NS16550HZ/2/(16*CONS_BAUD)
	sb	v1, NSREG(NS16550_DATA)(v0)
	nop
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	nop
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	nop
	li	v1, MCR_DTR|MCR_RTS
	sb	v1, NSREG(NS16550_MCR)(v0)
	nop
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
	nop

	j	ra
	nop

END(initserial)


	.rdata
hexchar:
	.ascii	"0123456789abcdef"

	.text
	.align	2
/*
 * Simple character printing routine used before full initialization
 */
.global stringserial
.global hexserial
.global tgt_putchar

LEAF(stringserial)

	move	a2, ra
	move	a1, a0
	lbu	    a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal	tgt_putchar
	addiu	a1, 1
	b	1b
	lbu	a0, 0(a1)

2:
	j	a2
	nop

END(stringserial)


LEAF(hexserial)

	move a2, ra
	move a1, a0
	li	 a3, 7
1:
	rol	a0, a1, 4
	move	a1, a0
	and	a0, 0xf
	
	la v0,hexchar
	
	addu v0, a0
	bal	tgt_putchar
	lbu	a0, 0(v0)
	
	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop

END(hexserial)

LEAF(tgt_putchar)

	la	v0, COM1_BASE_VADDR /**** for 2F BOX, COM1_BASE_VADDR *****/
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
	
#ifdef HAVE_NB_SERIAL
	move	v1, v0
	la	v0, COM3_BASE_VADDR
	bne	v0, v1, 1b
	nop
#endif

	j	ra
	nop	

END(tgt_putchar)

.set reorder

#endif
