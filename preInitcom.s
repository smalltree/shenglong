#include "./includex/i82371eb.h"
#include "./includex/sbd.h"
#include "./includex/isapnpreg.h"
#include "./includex/i8254.h"
#include "./includex/pc97307.h"

.set noreorder

#ifdef TTYDBGROM

	bal initserial 
	nop
	
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; .set reorder; RELOC(a0,98b); .set noreorder; bal stringserial; nop
#define PRINTSTR(x) TTYDBG(x)
	
	PRINTSTR("\r\nVxWorks Bootrom Initializing. Standby...\r\n")

	PRINTSTR("\r\n s7=")
	move a0,ra
	bal hexserial
	nop
	
	PRINTSTR("\r\nPRID=")
	mfc0	a0, C0_PRID
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
	
	b .2bdone
	nop

#define COM1_BASE_VADDR	(0xbe000000)
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
	la	v0, COM1_BASE_VADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	nop
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
	nop
 	li	v1, NS16550HZ/(16*CONS_BAUD)
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
	
	.set reorder
	RELOC(v0,hexchar)
	.set noreorder
	
	addu v0, a0
	bal	tgt_putchar
	lbu	a0, 0(v0)
	
	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop

END(hexserial)

LEAF(tgt_putchar)

	la	v0, COM1_BASE_VADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
	
	j	ra
	nop	

END(tgt_putchar)

#endif

.2bdone:
		nop

.set reorder

