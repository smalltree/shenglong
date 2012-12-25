/* Lsn2eCpciSBC.h - Loongson2E Computer header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

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
01b,21nov02,zmm  Cleanups after code review.
01a,16sep02,zmm  Written.
*/

/*
This file contains I/O addresses and related constants for the
Momentum Computer CP7000G SBC. 
*/

#ifndef	__LOONGSON2EBOX_H
#define	__LOONGSON2EBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "drv/timer/timerDev.h"
#include "drv/mem/memDev.h"
#include "gt64240.h"
#include "bonito64.h"
#include "lsn2ecpci.h"

#define LSN2ECPCISBC_DEVNUM_PIIX4 14/* modified by yinwx, original 17 */ 

#define BOARD_TYPE_UNKNOWN   (-1)    /* unknown or unsupported board type */
#ifndef	_ASMLANGUAGE
typedef struct pciBoardResource      /* PCI_BOARD_RESOURCE */
    {
    UINT32        pciBus;            /* PCI Bus number */
    UINT32        pciDevice;         /* PCI Device number */
    UINT32        pciFunc;           /* PCI Function number */

    UINT32        vendorID;          /* PCI Vendor ID */
    UINT32        deviceID;          /* PCI Device ID */
    UINT8         revisionID;        /* PCI Revision ID */
    UINT32        boardType;         /* BSP-specific board type ID */

    UINT8         irq;               /* Interrupt Request Level */
    UINT32        irqvec;            /* Interrupt Request vector */

    UINT32        bar [6];           /* PCI Base Address Registers */

    void * const  pExtended;         /* pointer to extended device info */

    } PCI_BOARD_RESOURCE;

#endif

/*added by wangfq*/
#define SYSAD_CLK_RATE 75000000 /*sysAD BUS clock rate*/
#define SYSAD_CLK_RATE_TIMES 9 /*CPU ±¶ÆµÏµÊý*/

#if 0
#define COM1_BASE_ADDR	0x3f8
#define COM2_BASE_ADDR	0x2f8
#endif

/*add by sunchao */
#define COM1_BASE_ADDR	0xbe000000
#define COM2_BASE_ADDR	0xbe000020
#define COM3_BASE_ADDR	0xbe000040
#define COM4_BASE_ADDR	0xbe000060


#define UART_REG_ADDR_INTERVAL 1 /*added by wangfq*/

#define PIC1_BASE_ADR			(0x20)
#define PIC2_BASE_ADR			(0xA0)
#define PIC_REG_ADDR_INTERVAL	1
#define NUMBER_OF_IRQS			16
/*end added*/

#if (_BYTE_ORDER == _BIG_ENDIAN)
#define PBYTESWAP(x)    (MSB(x) | (LSB(x) << 8))
#define PSWAP(x)        LONGSWAP(x)
#else /* _BYTE_ORDER == _LITTLE_ENDIAN */
#define PBYTESWAP(x)    (x)
#define PSWAP(x)        (x)
#endif

#define BUS		NONE
#define INITIAL_SR	(SR_CU1 | SR_BEV)
#define INITIAL_CFG	CFG_C_NONCOHERENT
#define CFG_TE		0x00001000

/*
 * These are the sizes (in MB) of each bank of SDRAM for a board with
 * a given amount of total memory.  Since all boards have 2 banks of SDRAM,
 * these numbers are 1/2 of the total.
 */

#define MSIZE_MB_1024   512
#define MSIZE_MB_512	256
#define MSIZE_MB_256	128
#define MSIZE_MB_128	64

/* TLB definitions */

#define TLBLO_PFNMASK           0x3fffffc0
#define TLBLO_PFNSHIFT          6
#define TLBLO_CMASK             0x00000038

#define TLBLO_NC                0x00000010 /* uncached */

#define TLBLO_NONC              0x00000018 /* cacheable non-coherent */
#define TLBLO_CEX               0x00000020 /* cacheable coherent exclusive */
#define TLBLO_CEXW              0x00000028 /* cacheable coherent exclusive write
 */
#define TLBLO_CUW               0x00000030 /* cacheable coherent update write */
#define TLBLO_D                 0x4             /* writeable */
#define TLBLO_V                 0x2             /* valid bit */
#define TLBLO_G                 0x1             /* global bit */

#define TLBHI_VPN2MASK          0xffffe000
#define TLBHI_VPN2SHIFT         13
#define TLBHI_PIDMASK           0xff
#define TLBHI_PIDSHIFT          0
#define TLBHI_NPID              256
#define TLBINX_PROBE            0x80000000
#define TLBINX_INXMASK          0x0000003f
#define TLBINX_INXSHIFT         0
#define TLBRAND_RANDMASK        0x0000003f
#define TLBRAND_RANDSHIFT       0
#define TLBCTXT_BASEMASK        0xff800000
#define TLBCTXT_BASESHIFT       23
#define TLBCTXT_VPN2MASK        0x007ffff0
#define TLBCTXT_VPN2SHIFT       4

/* Cacheable, noncoherent, write-through, no write allocate TLB */

#define TLBLO_NONC_WT	0x00000000	

/* Cacheable, noncoherent, write-through, write allocate TLB */

#define TLBLO_NONC_WTA	0x00000008	

#define TLBLO_PAGEMASK		0x01ffe000
#define TLBLO_MODE		(TLBLO_CMASK | TLBLO_D | TLBLO_V | TLBLO_G)
#define TLBLO_PAGEMASK_SIZE	(TLBLO_PAGEMASK | TLBLO_MODE | 0x1000)

/* Constants for figuring out CPU clock frequency */

/*commented by wangfq*/
#if 0
/* Constants for figuring out CPU clock frequency */

#define C0_CAUSE_IC_MASK	0x003F0000
#define C0_CAUSE_IC_SHIFT	16
#define CF_7_EP         (0xf << 24)      /* System Clock Ratio */
#define CF_7_EP_AL      24              /* Shift to align */
#define CF_7_EC         (3 << 28)       /* System Clock Ratio */
#define CF_7_EC_AL      28              /* Shift to align */
#define BUS_CLK_RATE    125000000
#define COUNTER_VAL_US  100             /* In uSeconds */
#endif
/*end commented*/

#define KSU_MASK	0xE0000000	/* Memory window selection bits */

#undef TLB_ENTRIES	
#define TLB_ENTRIES	64
#define L2_CACHE_SIZE	(512*1024) /*modified by wangfq, from 256 to 512*/
#define L2_CACHE_LINE_SIZE 32      /*added by wangfq*/
#define L1_DCACHE_LINE_SIZE 32
#define L1_ICACHE_LINE_SIZE 32

#define ICR_AND_IMASK0  0xFFFF00FF
#define ICR_OR_IMASK0   0x0000FF00

/* task default status register */

#define DEFAULT_SR (SR_CU1 | SR_CU0 | SR_IMASK0 | SR_IE)

#ifndef _ASMLANGUAGE
IMPORT UINT sysCpuClockRate();
#endif

#define CPU_CLOCK_RATE 		792000000/2/*(sysCpuClockRate())*/ /*modified by wangfq,original: (sysCpuClockRate()/2)*/

#define N_SIO_CHANNELS		1/* modified by yinwx original 2*/		/* No. serial I/O channels *//*modified by wangfq,original is 1*/
#define UART_CLK_RATE 		3686400 /*modified by wangfq, the original XTAL freq.= 20000000 */

#define NUM_PCI_DEVS		4		/* No. PCI devices */ /*modified by wangfq[SouthBridge,8139,VGA,USB],original 6,*/
#define PCI_DEV_IDSEL_MAX       (31-10)         /* max # of idsel lines */  /*modified by wangfq, originally (31-22)*/

#if 0 /*commented by wangfq*/
/* Default, power-up address values for the GT64240.  Do not change this */

#define GT64240_DEF_BASE_ADR   	(K1BASE | 0x14000000)
#define GT64240_BOOTCS_DEF_BASE	(K1BASE | 0x1fc00000)
#endif /*end commented*/

/* Address location of DRAM.  This should not be changed. */

#define DRAM_CACHE_VIRT_BASE		K0BASE
#define DRAM_NONCACHE_VIRT_BASE		K1BASE

#if 0 /*commented by wangfq*/
#define GT64240_RELOCATE
#ifdef GT64240_RELOCATE
#define IO_SPACE_VIRT_BASE	KSBASE
#define IO_SPACE_PHYS_BASE	KSBASE
#else
#define IO_SPACE_VIRT_BASE	K1BASE
#define IO_SPACE_PHYS_BASE	0
#endif
#endif
#if 1
#define IO_SPACE_TOTAL_SIZE	(KSSIZE + K3SIZE)	
#define IO_SPACE_PAGE_SIZE	(16*1024*1024)	/* Must be 4K, 16K, 64K, */
                                                /* 256K, 1MB, 4MB, or 16 MB */
/* Noncachable, blocking, valid, dirty, global */

#define IO_SPACE_MODE		(TLBLO_NC | TLBLO_V | TLBLO_D | TLBLO_G)
#define IO_SPACE_NUM_TLBS	(IO_SPACE_TOTAL_SIZE/(2*IO_SPACE_PAGE_SIZE))
#endif
/* Address for the Galileo devices */
#if 0
#define GAL_INTERNAL_REG_BASE	((IO_SPACE_PHYS_BASE + 3*(IO_SPACE_TOTAL_SIZE/4)) + 0x04000000)
#define GAL_DEV_BANK0_BASE	((IO_SPACE_PHYS_BASE + 3*(IO_SPACE_TOTAL_SIZE/4)) + 0x0c000000)
#define GAL_DEV_BANK0_SIZE	0x00800000
#define GAL_DEV_BANK1_BASE	(GAL_DEV_BANK0_BASE + GAL_DEV_BANK0_SIZE)
#define GAL_DEV_BANK1_SIZE	0x00800000
#define GAL_DEV_BANK2_BASE	(GAL_DEV_BANK1_BASE + GAL_DEV_BANK1_SIZE)
#define GAL_DEV_BANK2_SIZE	0x01000000
#define GAL_DEV_BANKS20_SIZE	0x03000000	
#define GAL_DEV_BANK3_BASE	(GAL_DEV_BANK0_BASE + GAL_DEV_BANKS20_SIZE)
#define GAL_DEV_BANK3_SIZE	0x00c00000
#define GAL_BOOTCS_BASE		(GAL_DEV_BANK3_BASE + GAL_DEV_BANK3_SIZE)
#define GAL_BOOTCS_SIZE		0x00400000
#define GAL_BANK3BOOTCS_SIZE	(GAL_DEV_BANK3_SIZE + GAL_BOOTCS_SIZE)

#ifdef _ASMLANGUAGE
#define LA(reg,val32)   	la	reg, val32
#define GT_PCI_SWAP_CONTROL	0x01000000
#endif

#define IO_TO_CPU(x)		(IO_SPACE_VIRT_BASE | x)
#define GT_ADDR_DECODE_MASK	0x0fff

#define PCI_IO_TO_CPU		IO_TO_CPU
#define PCI_MEM_TO_CPU		IO_TO_CPU
#define PCI_PF_MEM_TO_CPU	IO_TO_CPU

/* memory map as seen on the PCI bus */

#define PCI0_MEM0_ADRS		(IO_SPACE_PHYS_BASE)
#define PCI0_MEM0_SIZE		(IO_SPACE_TOTAL_SIZE/8) 
#define PCI0_MEM1_ADRS		(PCI0_MEM0_ADRS + PCI0_MEM0_SIZE)
#define PCI0_MEM1_SIZE		(IO_SPACE_TOTAL_SIZE/8)
#define PCI1_MEM0_ADRS		(PCI0_MEM1_ADRS + PCI0_MEM1_SIZE)
#define PCI1_MEM0_SIZE		(IO_SPACE_TOTAL_SIZE/8)
#define PCI1_MEM1_ADRS		(PCI1_MEM0_ADRS + PCI1_MEM0_SIZE)
#define PCI1_MEM1_SIZE		(IO_SPACE_TOTAL_SIZE/8)
#define PCI0_IO_ADRS		(PCI1_MEM1_ADRS + PCI1_MEM1_SIZE)
#define PCI0_IO_SIZE		(IO_SPACE_TOTAL_SIZE/8) 
#define PCI1_IO_ADRS		(PCI0_IO_ADRS + PCI0_IO_SIZE)
#define PCI1_IO_SIZE		(IO_SPACE_TOTAL_SIZE/8) 

/* memory map as seen from CPU */

#define CPU_PCI0_IO_ADRS	IO_TO_CPU(PCI0_IO_ADRS) /* PCI 0 I/O addr */
#define CPU_PCI0_IO_SIZE	PCI0_IO_SIZE		  

#define CPU_PCI0_MEM0_ADRS	IO_TO_CPU(PCI0_MEM0_ADRS)	/* PCI mem 0 addr */
#define CPU_PCI0_MEM0_SIZE	PCI0_MEM0_SIZE		
#define CPU_PCI0_MEM1_ADRS	IO_TO_CPU(PCI0_MEM1_ADRS)	/* PCI mem 1 addr */
#define CPU_PCI0_MEM1_SIZE	PCI0_MEM1_SIZE		

/* Default translatations for device setup reference */

#define CPU_PCI_IO_ADRS		CPU_PCI0_IO_ADRS
#define CPU_PCI_MEM_ADRS	CPU_PCI0_MEM0_ADRS

/* PCI vendor - device IDs */

#define PCI_ID_GAL		0x462011AB	/* Id for GT64240 */
#define PCI_ID_21555		0xB5558086	/* Id for I21555 */
#define PCI_ID_LN_GEI		0x10018086	/* Id for I82543GC */

/* PCI Device (IDSEL) numbers - Do not change */

#define PCI_DEV_GAL		0x00000000	/* Dev for GT64240 */
#define PCI_DEV_LN_GEI1		0x00000001	/* Dev for I82543GC */
#define PCI_DEV_LN_GEI2		0x00000002	/* Dev for I82543GC */
#define PCI_DEV_21555		0x00000003	/* Dev for I21555 */
#define PCI_DEV_PMC		0x00000004	/* Dev for PMC */

/* I21554/I21555 Window addresses */

#define PCI_BRIDGE_IO_ADRS	(PCI0_IO_ADRS + PCI0_IO_SIZE/2)
#define PCI_BRIDGE_IO_SIZE	(PCI0_IO_SIZE - PCI0_IO_SIZE/2)
#define CPU_PCI_BRIDGE_IO_ADRS	IO_TO_CPU(PCI_BRIDGE_IO_ADRS)

/* Memory address window */

#define PCI_BRIDGE_MEM_ADRS	(PCI0_MEM1_ADRS)
#define PCI_BRIDGE_MEM_SIZE	(PCI0_MEM1_SIZE/2)
#define CPU_PCI_BRIDGE_MEM_ADRS	IO_TO_CPU(PCI_BRIDGE_MEM_ADRS)

/* Pre-fetchable Memory address window */

#define PCI_BRIDGE_PF_MEM_ADRS	(PCI_BRIDGE_MEM_ADRS + PCI_BRIDGE_MEM_SIZE)
#define PCI_BRIDGE_PF_MEM_SIZE	(PCI_BRIDGE_MEM_SIZE)
#define CPU_PCI_BRIDGE_PF_MEM_ADRS	IO_TO_CPU(PCI_BRIDGE_PF_MEM_ADRS)
#define COMM_UNIT_ARBITER_CONTROL    0xf300

/* Given a PCI DRAM address, compute local CPU bus address */

#define PCI2DRAM_BASE_ADRS	0x00000000	/* memory seen from PCI bus */
#define PCI2CPU_TRANSLATE(x)	(LOCAL_MEM_LOCAL_ADRS + (x - PCI2DRAM_BASE_ADRS))

/* serial ports (COM1) */

#ifndef COM1_BASE_ADR
#define COM1_BASE_ADR		IO_TO_CPU(GAL_DEV_BANK2_BASE | 0x20)	/* serial port 1 */

/* debugging support (ns16550Serial.c) */

#define UART0_BASE_ADR COM1_BASE_ADR
#define BAUD_CLK_FREQ  UART_CLK_RATE
#endif

#define UART_REG_ADDR_INTERVAL  4	

/* non volatile ram defines */

#define	BBRAM_ADRS		IO_TO_CPU(GAL_DEV_BANK1_BASE)	/* base address is in */
						/* memory space */
#define	BBRAM_SIZE		0x7ff0		/* 32KB-16 total */

/* factory Ethernet address */

#define	BB_ENET			((volatile char *)(BBRAM_ADRS + 0x7cf2))
#define	ENET_DEFAULT		0x0003cc

/* MK48T37 Timekeeper */

#define	TOD_CLOCK 		((volatile char *)(BBRAM_ADRS + BBRAM_SIZE))

/* Flash Base Address */

#define DOC_BASE_ADR		IO_TO_CPU(GAL_DEV_BANK3_BASE)
#define DOC_SIZE		(0x00800000)	/* 8MB Flash */
#define	FDISK_BASE_ADR		DOC_BASE_ADR

/* Device timings definitions for the Galileo CS, BootCS */

#define GT64240_BASE_ADR      	IO_TO_CPU(GAL_INTERNAL_REG_BASE)
#define GT_SDRAM_BANK_CONFIG   	0x000f8000
#define GT_SDRAM_BANK_TIMING   	0x0000072a
#define GT_SDRAM_CONFIG    	0x04c20200

#define PLD_GAL_BANK		DEVICE_BANK0_PARAMETERS
#define PLD_GAL_DP		((0x2 <<  0) | (0x8 <<  3) | (0x8 <<  7) | \
				 (0x3 << 11) | (0x3 << 14) | (0x5 << 17) | \
				 (0x0 << 20) | (0x3 << 30)) 

#define NVRAM_GAL_BANK		DEVICE_BANK1_PARAMETERS
#define NVRAM_GAL_DP		((0x2 <<  0) | (0xd <<  3) | (0xd <<  7) | \
				 (0x5 << 11) | (0x7 << 14) | (0x5 << 17) | \
				 (0x0 << 20) | (0x3 << 30)) 

#define UART_GAL_BANK		DEVICE_BANK2_PARAMETERS
#define UART_GAL_DP		((0x2 <<  0) | (0xf <<  3) | (0xf <<  7) | \
				 (0x5 << 11) | (0x0 << 14) | (0x5 << 17) | \
				 (0x0 << 20) | (0x1 << 26) | (0x3 << 30)) 

#define DOC_GAL_BANK		DEVICE_BANK3_PARAMETERS
#define DOC_GAL_DP		((0x2 <<  0) | (0x1 <<  3) | (0x1 <<  7) | \
				 (0x5 << 11) | (0x7 << 14) | (0x5 << 17) | \
				 (0x1 << 23) | (0x1 << 24) | (0x0 << 20) | \
				 (0x3 << 30)) 

#define FLASH_GAL_BANK		BOOT_DEVICE_PARAMETERS
#define FLASH_GAL_DP		((0x2 <<  0) | (0xf <<  3) | (0xf <<  7) | \
				 (0x5 << 11) | (0x7 << 14) | (0x5 << 17) | \
				 (0x0 << 20) | (0x3 << 30)) 

#define GT_PCI0_TOR		(0xffff)
#define GT_PCI1_TOR		(0xffff)
#endif /*end commented by wangfq*/

/*
 * Interrupt Levels: Used in intEnable() and intDisable() 
 * Do not change these.  They are hardware specific
 * Note that from INT_LVL_GAL_P1 to INT_LVL_PERF are handled with
 * sysICRGet and sysICRSet
 */ 
#if 0 /*commented by wangfq*/
#define INT_LVL_PERF		SR_IBIT6	/* Performance Counter */
#define INT_LVL_TIMER7000	SR_IBIT5	/* R7000 timer (changed) */
#define INT_LVL_TESTPOINT	SR_IBIT4	/* Testpoint Interrupt */
#define INT_LVL_CPCI		SR_IBIT3	/* CPCI INTA/INTB/INTC/INTD */
#define INT_LVL_PMC			SR_IBIT2	/* PMC 1 */
#define INT_LVL_GAL_P1		SR_IBIT1 	/* 64240 interrupts */
#define INT_LVL_TIMER		SR_IBIT8 	/* Internal Timer (default) */
#define INT_LVL_GAL_P0		SR_IBIT7 	/* 64240 interrupts */
#define INT_LVL_BRIDGE		SR_IBIT6	/* I21555 Interrupts */
#define INT_LVL_COM1		SR_IBIT5	/* COM 1 */
#define INT_LVL_GEI2		SR_IBIT4	/* I82543 */
#define INT_LVL_GEI1		SR_IBIT3	/* I82543 */
#endif /*end commented,replaced by the following*/
/*added by wangfq*/
#define INT_LVL_TIMER		SR_IBIT8 	/* Internal Timer (default) Cause IP7*/
#define INT_LVL_PERF		SR_IBIT7	/* Performance Counter Cause IP6*/
#define INT_LVL_VIA686      SR_IBIT3    /* from VIA686b interrupt,cpu int3 hw line,Cause IP5, modified IP3*/
#define INT_LVL_BONITO		SR_IBIT7    /* from Bonito64 NorthBridge interrupt,cpu int0 hw line,Cause IP2, modified IP7*/
#define INT_LVL_SW1	        SR_IBIT2	/* sw interrupt 1 (fixed) */
#define INT_LVL_SW0	   		SR_IBIT1	/* sw interrupt 0 (fixed) */
/*end added by wangfq*/

/* Interrupt vectors: For intPrioTable[] in sysLib.c and intConnect() */

/* Interrupt vectors for actual MIPS interrupts */
#if 0 /*commented by wangfq*/
#define IV_PERF_VEC             64      /* Performance Counter */
#define IV_TIMER7000_VEC        65      /* R7000 timer (changed) */
#define IV_CPCI_VEC             66      /* CPCI INTA/INTB/INTC/INTD */
#define IV_TESTPOINT_VEC        67      /* Testpoint */
#define IV_PMC_VEC              68      /* PMC 1 */
#define IV_TIMER_VEC            69      /* R7000 timer (fixed) */
#define IV_GAL_P0_VEC           70      /* Galileo Interrupts */
#define IV_GAL_P1_VEC           71      /* Galileo Interrupts */
#define IV_BRIDGE_VEC           72      /* I21555 Interrupts */
#define IV_COM1_VEC             73      /* COM1 */
#define IV_GEI1_VEC             74      /* I82543GC */
#define IV_GEI2_VEC             75      /* I82543GC */
#endif /*end commented,replace by the following*/
/*added by wangfq*/
     
#define IV_TIMER_VEC		38 	/* Internal Timer (default) Cause IP7*/
#define IV_PERF_VEC			39	/* Performance Counter Cause IP6*/
#define IV_VIA686_VEC_BASE  40/*40   from VIA686b interrupt,cpu int3 hw line,Cause IP5*/
	#define IV_VIA686_VEC_NUM 16
	#define IV_VIA686_SPURIOUSIRQ_OFFSET 16
		#define IV_VIA686_SPURIOUSIRQ_VEC (IV_VIA686_VEC_BASE + IV_VIA686_SPURIOUSIRQ_OFFSET)
	#define IV_VIA686_COM1_OFFSET 4
		#define IV_VIA686_COM1_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM1_OFFSET)
	#define IV_VIA686_COM2_OFFSET 3
		#define IV_VIA686_COM2_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM2_OFFSET)
	/* add by sunchao */
	#define IV_VIA686_COM3_OFFSET 2
	#define IV_VIA686_COM4_OFFSET 1
	/* add end */
#define IV_BONITO_VEC_BASE	IV_VIA686_VEC_BASE /*(IV_VIA686_VEC_BASE + IV_VIA686_VEC_NUM + 1)*/	 /* from Bonito64 NorthBridge interrupt,cpu int0 hw line,Cause IP2*/
	#define IV_BONITO_PCIIRQ26_VEC_OFFSET 26
		#define IV_BONITO_PCIIRQ26_VEC (IV_BONITO_VEC_BASE + IV_BONITO_PCIIRQ26_VEC_OFFSET)
	#define IV_BONITO_PCIIRQ27_VEC_OFFSET 27
		#define IV_BONITO_PCIIRQ27_VEC (IV_BONITO_VEC_BASE + IV_BONITO_PCIIRQ27_VEC_OFFSET)
	#define IV_BONITO_PCIIRQ28_VEC_OFFSET 28
		#define IV_BONITO_PCIIRQ28_VEC (IV_BONITO_VEC_BASE + IV_BONITO_PCIIRQ28_VEC_OFFSET)	
	#define IV_BONITO_VEC_NUM 32

#define INT_NUM_IRQ0  (IV_VIA686_VEC_BASE+10)

#define IV_COM1_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM1_OFFSET)
#define IV_COM2_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM2_OFFSET)
/* add by sunchao */
#define IV_COM3_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM3_OFFSET)
#define IV_COM4_VEC (IV_VIA686_VEC_BASE + IV_VIA686_COM4_OFFSET)

#define IV_PCI_BUS	  (INT_NUM_IRQ0)
#define IV_PATA_IDE0 (IV_PCI_BUS)
#define IV_END0_VEC  (INT_NUM_IRQ0 + 0)
/* add end */
#define IV_KBD_VEC  (IV_VIA686_VEC_BASE + KBD_INT_LVL)

/*end added by wangfq*/

#if 0 /*commented by wangfq*/
/*
 * Interrupt vectors for interrupts demuxed by the PLDs.  Do not change
 * these values without consulting sysPLDIntDemux() in sysLib.c
 */

#define IV_CPCI_INTA_VEC	76	/* CPCI INTA interrupt line */
#define IV_CPCI_INTB_VEC	77	/* CPCI INTB interrupt line */
#define IV_CPCI_INTC_VEC	78	/* CPCI INTC interrupt line */
#define IV_CPCI_INTD_VEC	79	/* CPCI INTD interrupt line */
#define IV_21555_VEC		80	/* Intel 21555 Secondary INTA */
#define IV_CPCI_ENUM_VEC	81	/* CPCI ENUM# interrupt */
#define IV_CPCI_DEG_VEC		82	/* CPCI DEG# interrupt */
#define IV_CPCI_FAL_VEC		83	/* CPCI FAL# interrupt */
#endif /*end commented*/

/*
 * Interrupt vectors for interrupts from the GT-64240 part.  Do not
 * change these values without consulting sysGtIntDemux()
 * These values relate to bit-positions in status registers.
 */

#define IV_DEV_BUS_VEC		84
#define IV_DMA_ERROR_VEC	85
#define IV_CPU_BUS_VEC		86
#define IV_IDMA0_1_VEC		87
#define IV_IDMA2_3_VEC		88
#define IV_IDMA4_5_VEC		89
#define IV_IDMA6_7_VEC		90
#define IV_TIMER0_1_VEC		91
#define IV_TIMER2_3_VEC		92
#define IV_TIMER4_5_VEC		93
#define IV_TIMER6_7_VEC		94
#define IV_PCI0_0_VEC		95
#define IV_PCI0_1_VEC		96
#define IV_PCI0_2_VEC		97
#define IV_PCI0_3_VEC		98
#define IV_ECC_VEC			99
#define IV_PCI1_0_VEC		100
#define IV_PCI1_1_VEC		101
#define IV_PCI1_2_VEC		102
#define IV_PCI1_3_VEC		103
#define IV_PCI0_OUT_L_VEC	104
#define IV_PCI0_OUT_H_VEC	105
#define IV_PCI1_OUT_L_VEC	106
#define IV_PCI1_OUT_H_VEC	107
#define IV_PCI0_IN_L_VEC	109
#define IV_PCI0_IN_H_VEC	110
#define IV_PCI1_IN_L_VEC	111
#define IV_PCI1_IN_H_VEC	112

#define	IV_ETHERNET0_VEC	115
#define	IV_ETHERNET1_VEC	116
#define	IV_ETHERNET2_VEC	117
#define IV_SDMA_VEC		119
#define IV_I2C_VEC		120
#define IV_BRG_VEC		122
#define IV_MPSC0_VEC		123
#define IV_MPSC1_VEC		125
#define IV_COMM_VEC		126

#if 0 /*commented by wangfq*/
/* PCI Interrupt Mapping */

#define PCI_IV_UNMAPPED                 -1
#define PCI_INTR_A_VEC                  PCI_IV_UNMAPPED
#define PCI_INTR_B_VEC                  PCI_IV_UNMAPPED
#define PCI_INTR_C_VEC                  PCI_IV_UNMAPPED
#define PCI_INTR_D_VEC                  PCI_IV_UNMAPPED

/* Miscellaneous PLD registers */

#define PLD_BASE_ADR		IO_TO_CPU(GAL_DEV_BANK0_BASE)

#define REV		0x0     /* Board Assembly Revision */
#define PLD1ID          0x1     /* PLD 1 ID */
#define PLD2ID          0x2     /* PLD 2 ID */
#define RESET_STAT      0x3     /* Reset Status Register */
#define BOARD_STAT      0x4     /* Board Status Register */
#define CPCI_ID         0x5     /* Compact PCI ID Register */
#define CONTROL         0x8     /* Control Register */
#define CPU_EEPROM      0x9     /* CPU Configuration EEPROM Register */
#define INTMASK         0xA     /* Interrupt Mask Register */
#define INTSTAT         0xB     /* Interrupt Status Register */
#define INTSET          0xC     /* Interrupt Set Register */
#define INTCLR          0xD     /* Interrupt Clear Register */
 
/* Reset Status Register */

#define RESET_POWER     0x01    /* Power Up Reset */
#define RESET_BUTTON    0x02    /* Push Button Reset */
#define RESET_CPCI      0x04    /* Compact PCI Reset */
#define RESET_WDOG      0x08    /* Watchdog Reset */
#define RESET_SW        0x10    /* Software Reset */
 
/* Board Status Register */

#define BOARD_USER      0x80    /* User Jumper Installed */
#define BOARD_FWRITE    0x40    /* Flash Write Enable Jumper Installed */
#define BOARD_SYNCH     0x20    /* Set Galileo 64240 to clocking mode 6 */
#define BOARD_PHY       0x10    /* Transition Board PHYs Active */
#define BOARD_L3_NONE   0x00    /* No L3 Cache */
#define BOARD_L3_2MB    0x04    /* 2MB L3 Cache */
#define BOARD_L3_4MB    0x08    /* 4MB L3 Cache */
#define BOARD_L3_8MB    0x0c    /* 8MB L3 Cache */
#define BOARD_L3_MASK   0x0c    /* L3 cache size mask bits */

#define BOARD_RAM_1GB   0x00    /* 1 GB  SDRAM */
#define BOARD_RAM_128MB 0x01    /* 128MB SDRAM */
#define BOARD_RAM_256MB 0x02    /* 256MB SDRAM */
#define BOARD_RAM_512MB 0x03    /* 512MB SDRAM */
#define BOARD_RAM_MASK  0x03    /* SDRAM size mask bits */

/* Compact PCI ID Register */

#define CPCI_SSLOT      0x20    /* CPCI System Slot */
#define CPCI_ADDR_MASK  0x1f    /* Mask bit for Geographical Address */

/* Control Register */

#define CTRL_I2C_CLK    0x01    /* I2C Clock Line */
#define CTRL_I2C_DAT    0x02    /* I2C Data I/O */
#define CTRL_I2C_CLK_EN 0x04    /* I2C Enable Clock Line */
#define CTRL_I2C_DAT_EN 0x08    /* I2C Enable Data I/O */
#define CTRL_I2C_ST     0x10    /* I2C Data Latch Strobe */
 
/* CPU Configuration EEPROM */

#define EEPROM_CSEL     0x01    /* EEPROM Chip Select */
#define EEPROM_CLK      0x02    /* EEPROM Clock */
#define EEPROM_DAT_OUT  0x04    /* EEPROM Data Output */
#define EEPROM_DAT_IN   0x08    /* EEPROM Data Input */
#define EEPROM_STROBE   0x10    /* EEPROM Read Strobe */
 
/* Interrupt Mask Register */

#define INTMASK_INTA    0x01    /* CompactPCI INTA# Mask */
#define INTMASK_INTB    0x02    /* CompactPCI INTB# Mask */
#define INTMASK_INTC    0x04    /* CompactPCI INTC# Mask */
#define INTMASK_INTD    0x08    /* CompactPCI INTD# Mask */
#define INTMASK_BRIDGE  0x10    /* I21554/I21555 Secondary Interrupt Mask */
#define INTMASK_ENUM    0x20    /* CompactPCI ENUM# Mask */
#define INTMASK_DEG     0x40    /* CompactPCI DEG# Mask */
#define INTMASK_FAL     0x80    /* CompactPCI FAL# Mask */
 
/* Interrupt Status Register */

#define INTSTAT_INTA    INTMASK_INTA    /* CompactPCI INTA# Status */
#define INTSTAT_INTB    INTMASK_INTB    /* CompactPCI INTB# Status */
#define INTSTAT_INTC    INTMASK_INTC    /* CompactPCI INTC# Status */
#define INTSTAT_INTD    INTMASK_INTD    /* CompactPCI INTD# Status */
#define INTSTAT_BRIDGE  INTMASK_BRIDGE  /* I21554/I21555 Secondary Interrupt Status */
#define INTSTAT_ENUM    INTMASK_ENUM    /* CompactPCI ENUM# Status */
#define INTSTAT_DEG     INTMASK_DEG     /* CompactPCI DEG# Status */
#define INTSTAT_FAL     INTMASK_FAL     /* CompactPCI FAL# Status */
 
/* Interrupt Set */

#define INTSET_INTA     INTMASK_INTA    /* CompactPCI INTA# Set */
#define INTSET_INTB     INTMASK_INTB    /* CompactPCI INTB# Set */
#define INTSET_INTC     INTMASK_INTC    /* CompactPCI INTC# Set */
#define INTSET_INTD     INTMASK_INTD    /* CompactPCI INTD# Set */
#define INTSET_WDOG     0x20            /* Enable WDOG */
#define INTSET_ULED     0x40            /* USER LED On */
#define INTSET_BLED     0x80            /* BIT LED On */
 
/* Interrupt Clear Register */

#define INTCLR_INTA     INTMASK_INTA    /* CompactPCI INTA# Clear */
#define INTCLR_INTB     INTMASK_INTB    /* CompactPCI INTB# Clear */
#define INTCLR_INTC     INTMASK_INTC    /* CompactPCI INTC# Clear */
#define INTCLR_INTD     INTMASK_INTD    /* CompactPCI INTD# Clear */
#define INTCLR_WDOG     INTSET_WDOG     /* USER LED Off */
#define INTCLR_ULED     INTSET_ULED     /* USER LED Off */
#define INTCLR_BLED     INTSET_BLED     /* BIT LED Off */

#define PLD_REG(x)       (PLD_BASE_ADR + x)

#endif /*end commented*/

#ifdef __cplusplus
}
#endif

#endif /* INCcp7000gh */
