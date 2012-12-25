/* sysLib.c - Momentum Ocelot-G Board Support Package system-dependent routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */
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
01c,20nov02,zmm  Fix sysGtIntEnable, sysGtIntDisable and sysGtIntDemux.
                 Add auxiliary clock support.
01b,10oct02,slk  added support for wancomEnd driver
01a,17sep02,zmm  Written.
*/

/*
DESCRIPTION
This library provides board-specific routines.
The chip drivers included are:

    mipsR4kTimer.c - MIPS R4XX0 on-chip R4k timer library
    sysSerial.c    - Serial driver
    m48t37WdTimer.c" - Watchdog timer using M48T37 watchdog facilities 
    gt64240AuxTimer.c - Auxiliary clock using the GT64240 Timer 0 
    gt64240.c - GT64240 Controller Facilities 
    sysBusPci.c - Standard PCI Configuration space utilities 
    i21555.c - INTEL 21554/21555 PCI-to-PCI bridge driver 

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes */

#include "vxWorks.h"
#include "version.h"
#include "cacheLib.h"
#include "iv.h"
#include "esf.h"
#include "fppLib.h"
#include "ioLib.h"
#include "tyLib.h"
#include "sysLib.h"
#include "dosFsLib.h"
#include "config.h"
#include "string.h"
#include "intLib.h"
#include "arch/mips/fppMipsLib.h"
#ifdef INCLUDE_SN
#include "drv/netif/if_sn.h"
#endif

#ifdef INCLUDE_FD
#include "drv/Fdisk/nec765Fd.h"		/* added by yinwx */
#endif

#ifdef INCLUDE_ATA
#include "ataDrv.h"
#endif

#ifdef INCLUDE_PCI
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciAutoConfigLib.h"
#include "drv/pci/pciConfigShow.h"
#endif
#include "simpleprintf.h"
#include "Lsn2eCpciSBC.h"
#include "cs5536.h"
#include "cs5536_pci.h"
/*
 * board-specific routines shared by all MIPS BSPs but which are
 * customized by this BSP
 */

#define SYS_CLEAR_TLB
#define SYS_LOCAL_TO_BUS_ADRS
#define SYS_BUS_TO_LOCAL_ADRS
#define SYS_BUS_INT_ACK
#define SYS_BUS_INT_GEN
/*#define SYS_INT_ENABLE*/ 
/*#define SYS_INT_DISABLE*/ 

/* externals */

IMPORT void 	fpIntr (void);
IMPORT void 	sysWbFlush (void);
IMPORT int  	sysFpaAck (void);

IMPORT ULONG    taskSRInit(ULONG newSRValue);
IMPORT void     sysClearTlbEntry (int entry);
IMPORT void		sysClearTlb(void);
IMPORT int      sysCompareSet (int compareValue);
IMPORT int      sysPridGet (void);
IMPORT UINT     sysConfigGet (void);
IMPORT void     sysCauseIntClear(int); /*add by wangfq*/

#if 0 /*commented by wangfq*/
IMPORT STATUS   sysI21555Init (UINT32 pciConfAdr);
IMPORT void     sysI21555Init2 (void);
IMPORT STATUS	sysI21555MasterSet(UINT window, UINT cpciAddress, UINT size,
                                   BOOL prefetchable, UINT *cpuAddress);
IMPORT STATUS	sysI21555SlaveSet(UINT window, UINT cpciAddress, 
                                  UINT pciAddress, UINT size, BOOL prefetchable);
#endif /*end commented*/

IMPORT void     sysTlbIdxSet (UINT regVal);
IMPORT UINT     sysTlbIdxGet (void);
IMPORT void     sysTlbReadIdx (void);
IMPORT UINT     sysTlbMaskGet (void);
IMPORT void     sysTlbMaskSet (UINT regVal);
IMPORT UINT     sysTlbHIGet (void);
IMPORT void     sysTlbHISet (UINT regVal);
IMPORT UINT     sysTlbLO0Get (void);
IMPORT void     sysTlbLO0Set (UINT regVal);
IMPORT UINT     sysTlbLO1Get (void);
IMPORT void     sysTlbLO1Set (UINT regVal);
IMPORT STATUS   sysSetTlbEntry (UINT entry, ULONG virtual, ULONG physical, UINT mode);
IMPORT void     sysGetTlbEntry (UINT entry, ULONG *mask, ULONG *TLBLO0, ULONG *TLBHI);

IMPORT UINT8    ffsMsbTbl[];            /* Msb high interrupt hash table */
IMPORT UINT8    ffsLsbTbl[];            /* Lsb high interrupt hash table */


void sysBonito64Init (void);

extern void pci_isa_write_reg(int, UINT32); /* added by yinwx */
extern void pci_ide_write_reg(int, UINT32);
extern void pci_acc_write_reg(int, UINT32);
extern void pci_ohci_write_reg(int, UINT32);
extern void pci_ehci_write_reg(int, UINT32);
extern void _wrmsr(UINT32 reg, UINT32 hi, UINT32 lo);
extern void _rdmsr(UINT32 reg, UINT32 *hi, UINT32 *lo);

UINT32 Sii0680_baseaddr;
UINT32 Sii0680_ioaddr1;
UINT32 Sii0680_ioaddr2;

#if defined (INCLUDE_IPIIX4PCI)
LOCAL void sysIPIIX4Init(void);
#endif /* INCLUDE_IPIIX4PCI */

#if defined (INCLUDE_SMCFDC37M81X)
LOCAL void smcFdc37m81xInit(void);
#endif

#if defined (INCLUDE_ATA)
	/* LOCAL void sysIPIIX4ATAInit(void); */
#endif
#ifdef	INCLUDE_ATA
IMPORT VOIDFUNCPTR _func_sysAtaInit;

IMPORT STATUS usrAtaConfig (int ctrl, int drive, char *fileName);
ATA_TYPE ataTypes[ATA_MAX_CTRLS][ATA_MAX_DRIVES] =
    {
    	{
    		{761, 8, 39, 512, 0xff}
	}/* ctrl 0 drive 0 */
    };

ATA_RESOURCE ataResources[ATA_MAX_CTRLS] =
    {
    {
     {
     5, 0,
     {ATA0_IO_START0, ATA0_IO_START1}, {ATA0_IO_STOP0, ATA0_IO_STOP1}, 0,
     0, 0, 0, 0, 0
     },
     IDE_LOCAL, 1, ATA0_INT_VEC, ATA0_INT_LVL, ATA0_CONFIG,
     ATA_SEM_TIMEOUT, ATA_WDG_TIMEOUT, 0, 0
    }
    };

#endif	/* INCLUDE_ATA */

#if defined (INCLUDE_FD)			/* added by yinwx, begin */
LOCAL void sysIPIIX4FdInit(void);
#endif

#ifdef INCLUDE_FD

IMPORT STATUS usrFdConfig(int drive, int type, char *fileName);

UINT sysFdBuf = FD_DMA_BUF_ADDR;
UINT sysFdBufSize = FD_DMA_BUF_SIZE;

FD_TYPE fdTypes[] = 
{
/* No 0, 3.5" 2HD 1.44MB */
	{2880, 	/* sectors */
	 18, 	/* sectors per track */
	 2,		/* heads */
	 80,	/* cylinders */
	 2,		/* secSize, 128<<secSize 512 */
	 0x1b,	/* gap1 for rw */
	 0x6c,	/* gap2 for format 0x54*/
	 0x00,	/* dataRate 0x00 means 500KB/S(MFM) */
	 0x0c,	/* stepRate, the lower nibble 'c' means 4ms */
	 0x0f,	/* head unload time, the lower nibble 'f' means 240ms*/
	 0x05,	/* head load time, 0x05 means 10ms */
	 0x01,	/* set MFM bit high */
	 0x01,	/* set SK bit high */
	 "H1440"/* name */
	},
/* No 1, 5.25" 2HD 1.2MB */
	{2400,	/* sectors */
	 15,	/* sectors per track */
	 2,		/* heads */
	 80,	/* cylinders */
	 2,		/* secSize, 128<<secSize 512 */
	 0x1b,	/* gap1 for rw 0x24*/
	 0x54,	/* gap2 for format 0x50*/
	 0x00,	/* dataRate 0x00 means 500KB/S(MFM) */
	 0x0d,	/* stepRate, the lower nibble 'd' means 3ms */
	 0x0f,	/* head unload time, the lower nibble 'f' means 240ms*/
	 0x05,	/* head load time, 0x05 means 10ms */
	 0x01,	/* set MFM bit high */
	 0x01,	/* set SK bit high */
	 "h1200"/* name */
	}
};

#endif /* INCLUDE_FD */				/* added by yinwx, end */	

#if defined (INCLUDE_W83527)
LOCAL void w83527Init(void);
#endif

#if defined (INCLUDE_SII0680)		/* added by yinwx, 2009-03-19 */
LOCAL STATUS sysSii0680PciInit(UINT32 pciBus, UINT32 pciDevice, UINT32 pciFunc);
#endif

#include "intrCtl/i8259Pic.c"
UINT sysVectorIRQ0 = 0;		/* vector for IRQ0 */
LOCAL void sysIntInitPICLocal (void);
LOCAL void sysSpuriousInt(void);

int sysVIA686IntDemux (int vecbase);
int sysBonitoIntDemux (int vecbase);
int sysSerialprint (int vecbase);

/* globals */

int   sysMemTopDebug = 0;
UINT32 sdramSize;           /* SDRAM auto configurating result */

/*BOOL  sysCPCIEnable = FALSE;*/ /*commented by wangfq*/

UINT8 * sysHashOrder = ffsMsbTbl;       /* interrupt prio., 7 = high 0 = low */

/* forward declarations */
LOCAL int  sysSw0Ack ();
LOCAL int  sysSw1Ack ();
void sysPciIntConnect(void);
STATUS cacheLsn2eLibInit(CACHE_MODE, CACHE_MODE);

/*
 *  These tables are critical to interrupt processing.  Do not alter their
 *  contents until you understand the consequences. Refer to the Tornado
 *  for MIPS Architecture supplement for instructions on their use.
 */

typedef struct 
    {
    ULONG	intCause;		/* CAUSE IP bit of int source  */
    ULONG	bsrTableOffset; 	/* index into BSR table        */
    ULONG	intMask;		/* interrupt mask              */
    ULONG	demux;			/* demultiplex arg             */
    } PRIO_TABLE;

PRIO_TABLE intPrioTable[] = 
{
	{CAUSE_SW1, (ULONG) IV_SWTRAP0_VEC,		0x000100, 0},
	{CAUSE_SW2, (ULONG) IV_SWTRAP1_VEC,		0x000200, 0},
	/*{CAUSE_IP3, (ULONG) sysVIA686IntDemux,	0x000400, IV_VIA686_VEC_BASE},*/
	{CAUSE_IP3, (ULONG) IV_COM1_VEC,	0x000400, 0},
	{CAUSE_IP4, (ULONG) IV_COM2_VEC/*sysSerialprint*/,		0x000800, 0/*43*/},
	{CAUSE_IP5, (ULONG) IV_COM3_VEC,				0x001000, 0},
	{CAUSE_IP6, (ULONG) IV_COM4_VEC,				0x002000, 0},
	{CAUSE_IP7, (ULONG) sysBonitoIntDemux,	0x004000, IV_BONITO_VEC_BASE},
	{CAUSE_IP8, (ULONG) IV_TIMER_VEC,		0x008000, 0},
};
 
#ifdef	INCLUDE_PCI                     /* BSP PCI bus & config support */
#   include "pci/pciConfigLib.c"
#   include "pci/pciIntLib.c"
#   if (defined(INCLUDE_PCI_CFGSHOW) && !defined(PRJ_BUILD))
#      include "pci/pciConfigShow.c"
#   endif /* (defined(INCLUDE_PCI_CFGSHOW) && !defined(PRJ_BUILD)) */
#if (PCI_CFG_TYPE == PCI_CFG_AUTO)
#   include "pci/pciAutoConfigLib.c"
#   include "sysBusPci.c"
#endif /* (PCI_CFG_TYPE == PCI_CFG_AUTO) */
#ifdef INCLUDE_IPIIX4PCI
#   include "pci/iPiix4Pci.c"
#endif
#ifdef INCLUDE_PC_CONSOLE
	#include "serial/pcConsole.c"
	#include "serial/i8042kbd.c"
	#include "serial/m6845Vga.c"
	#include "video/ctB69000Vga.c"
#endif
#endif	/* INCLUDE_PCI */
#ifdef INCLUDE_WINDML
#include "syswindML.c"
#endif
/* Included Generic MIPS Support code */

#include "sysMipsLib.c"

/* Included drivers */
#include "sysSerial.c" 
/*#include "m48t37WdTimer.c" */ /*commented by wangfq*/
/*#include "gt64240AuxTimer.c"*/ /*commented by wangfq*/
/*#include "i21555.c"*/ /*commented by wangfq*/
#include "timer/mipsR4kTimer.c"
#include "sysFei82557End.c"
#include "sysGei82543End.c"

#include "bonito64.c"
/******************************************************************/
int sysSerialprint
( 
	int vecbase
)
{
	return vecbase;
}  

		     
/***************************************************************************
*
* sysVIA686IntDemux - Decode interrupts which are managed by the i8259A in VIA686B,
* Reads the interrupt request register in the super io chip and 
* determines the highest priority interrupt.
*
* This routine determines the source of interrupts which are routed through
* the i8259A in VIA686B and returns a unique interrupt vector number for each
* interrupt source.
*
* RETURNS: An interrupt vector number
*/
int sysVIA686IntDemux
(
    int vecbase
)
{
	/*UINT hi,lo;*/
    int requestReg;
    int level;
    int oldLevel;
    int retry;
	int bbb;
	int maskReg=0x7;
	
	/*_rdmsr(DIVIL_MSR_REG(PIC_XIRR_STS_LOW),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_XIRR_STS_HIGH),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	
	tempdata=read_c0_status;
	printstr(" C0_status=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=read_c0_cause;
	printstr(" C0_cause=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTISR;
	printstr(" bonito_intisr=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTEN;
	printstr(" bonito_inten=0x");
	printnum(tempdata);
	tempdata=BONITO_INTPOL;
	printstr(" bonito_intpol=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTEDGE;
	printstr(" bonito_intedge=0x");
	printnum(tempdata);
	printstr("\r\n======================================\r\n");*/
    oldLevel = intLock ();
	
    for (retry=0; retry < 1; retry ++)
	{
		maskReg = sysInByte(PIC_IMASK(PIC1_BASE_ADR)); /* 0x21 OCW1 */
        sysOutByte (PIC_port1 (PIC1_BASE_ADR), 0x0a); /* 0x20 set OCW3 */
		bbb = sysInByte (PIC_port1 (PIC1_BASE_ADR)); /* 020 read IRQ Reg */
        requestReg = bbb & ~maskReg;

/*printstr(" first: bbb=0x");printnum(bbb);printstr("\r\n");
printstr(" maskReg=0x");printnum(maskReg);printstr("\r\n");*/

		for (level=0; level < 8; level++)
	    if ((requestReg & 1) && (level != 2)) /* 2 is for cascade!! */
	        goto sysIntLevelExit;
	    else
	        requestReg >>= 1;

		maskReg = sysInByte(PIC_IMASK(PIC2_BASE_ADR));
        sysOutByte (PIC_port1 (PIC2_BASE_ADR), 0x0a);
		bbb = sysInByte (PIC_port1 (PIC2_BASE_ADR));
        requestReg = bbb & ~maskReg;

/*printstr(" second: bbb=0x");printnum(bbb);printstr("\r\n");
printstr(" maskReg=0x");printnum(maskReg);printstr("\r\n");*/	
        for (level=8; level < 16; level++)
	    if (requestReg & 1)
	        goto sysIntLevelExit;
	    else
	        requestReg >>= 1;
	}

    sysIntLevelExit:

	#if 0
	printstr(" level=0x");
	printnum(level);
	printstr("\r\n");
	tempdata=read_c0_status;
	printstr(" C0_status=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=read_c0_cause;
	printstr(" C0_cause=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTISR;
	printstr(" bonito_intisr=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTEN;
	printstr(" bonito_inten=0x");
	printnum(tempdata);
	tempdata=BONITO_INTPOL;
	printstr(" bonito_intpol=0x");
	printnum(tempdata);
	printstr("\r\n");
	tempdata=BONITO_INTEDGE;
	printstr(" bonito_intedge=0x");
	printnum(tempdata);
	printstr("\r\n======================================\r\n");
	/*tempdata=level;
	printstr(" 8259 level=0x");
	printnum(tempdata);*/		
	#endif
	
    intUnlock (oldLevel);
	
    return (vecbase + level);
}
 
/*************************************************************************
*
* sysBonitoIntDemux - Decode interrupts which are managed by the Bonito64 
*
* This routine determines the source of interrupts which are routed through
* the Bonito64 and returns a unique interrupt vector number for each
* interrupt source.
*
* RETURNS: An interrupt vector number
*/

int sysBonitoIntDemux
    (
    	int vecbase
    )
{
    UINT cause, mask;

    cause = BONITO_INTISR;
    mask =  BONITO_INTEN;

    /* set cause so only enabled interrupts are visible */
    cause &= mask;
	#if 0
	printstr("BONITO_INTEN = ");
	printnum(mask);
	printstr("BONITO_INTISR = ");
	printnum(cause);
	printstr("\r\n");
	#endif
    /* determine interrupt vector for source */
	#if 0
	for(i = 0; i < IV_BONITO_VEC_NUM; i++)
	{
		if (cause & 0x01) return 40;/*(vecbase + i + 20);*//* note:24-4=20 */
		cause >>= 1;
	}
	#endif
	
	/*printstr("~~~>");*/
	#if 0
	if(cause & 0x10)	
	{
		/* return IV_PATA_IDE0; */
		#if 1
		intLine = (*(volatile unsigned char *)(0xbfd0418e));
		printstr("^");
		printnum(intLine);
		if((intLine) == 0x00)
		{
			return geiPciResources[0].irq;
			}
		else
		{
			return IV_PATA_IDE0;
			}
		#endif
	}
	if(cause & 0x20)	return geiPciResources[1].irq;
	if(cause & 0x40)	return geiPciResources[2].irq;
	if(cause & 0x80)	return geiPciResources[3].irq;
	#endif
	if(cause &0xF0)	return IV_PCI_BUS;
    return (0);    /* no interrupt found */
}

/******************************************************************************
*
* sysCacheInit - initialize cache
*
* This routine sets the six global cache size variables indicating the sizes
* and line sizes of the primary data, primary instruction, and secondary
* caches, and then calls the cacheInit routine.  This is called by
* cacheLibInit() in bootConfig.c
*
* RETURNS: N/A
*
*/

STATUS sysCacheInit
    (
    CACHE_MODE instMode,
    CACHE_MODE dataMode
    )
    {
    
    return (cacheLsn2eLibInit (instMode, dataMode));
    }

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string board vendor
*/

char *sysModel (void)
    {
    return ("Loongson2f 7010A Single Board Computer");
    }

/******************************************************************************
*
* sysBspRev - return the bsp version with the revision eg 1.1/<x>
*
* This function returns a pointer to a bsp version with the revision.
* for eg. 1.1/<x>. BSP_REV is concatanated to BSP_VERSION to form the
* BSP identification string.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION BSP_REV);
    }

/*******************************************************************************
*
* sysHwInit - initialize the CPU board hardware
*
* This routine initializes various features of the CP7000G Ocelot.
* It is called from usrInit() in usrConfig.c.
*
* NOTE:
* This routine should not be called directly by the user.
* 
* RETURNS: N/A
*/

void sysHwInit (void)
    {

	/* *(volatile unsigned char*)0xbfe0016a=0xff;  
		PCI Arbitration Config for rude dev */
	
	sdramSize = LOCAL_MEM_SIZE;
	BONITO_INTPOL|=(1<<11)|(1<<12)|(1<<13)|(1<<14)/*|(1<<12)*/;

    /* init status register but leave interrupts disabled */
	taskSRInit (DEFAULT_SR & 0xfffffcff); /* Modified!! original 0xffffa4ff */
    intSRSet(DEFAULT_SR & ~SR_IE & 0xfffffcff);
 
	
    sysBootLine = (char *) BOOT_LINE_ADRS;       /* address of boot line */
    sysExcMsg   = (char *) EXC_MSG_ADRS;         /* catastrophic message area */

    /* initialize floating pt unit */
    if (fppProbe () == OK)
	{
		fppInitialize ();
		intVecSet ((FUNCPTR *)INUM_TO_IVEC (IV_FPE_VEC), (FUNCPTR) fpIntr);
	}
/*added by wangfq*/
    /* initialize the Bonito64<North bridge> for proper pci io space access */
    sysBonito64Init();
/*end added*/

    
    /* Initialize the PCI Bus devices we care about */
    #if 0 /*commented by wangfq, placed after north bridge and south bridge initialized*/
    #ifndef INCLUDE_TYCODRV_5_2
    sysSerialHwInit();
	#endif /* INCLUDE_TYCODRV_5_2 */
	#endif /*end commented*/   
		
#ifdef INCLUDE_PCI
    pciConfigLibInit (PCI_MECHANISM_0, (ULONG)sysPciConfigRead, (ULONG)sysPciConfigWrite, NONE); 

	#if (PCI_CFG_TYPE == PCI_CFG_AUTO)  /*added by wangfq with the conditional complilation*/
	sysPciAutoConfig ();
	#endif /* (PCI_CFG_TYPE == PCI_CFG_AUTO) */

	#ifdef  INCLUDE_PC_CONSOLE
	vga_bios_init();
    /*ctB69000VgaInit();*/
	#endif
 
    /*sysGei82543PciInit();*/ /*commented by wangfq,replaced by the following*/
	#ifdef INCLUDE_FEI_END
	sys557PciInit(0,6,0,INTEL_PCI_VENDOR_ID,FEI_DEVICEID_i82559ER,9);
	#endif
  
	#ifdef INCLUDE_GEI8254X_END
	#if 0/*may be unnecessary*/
	sysPciConfigWrite(0, 14, 0, 0x30, 4, 0x1ad60000);
	sysPciConfigWrite(0, 14, 1, 0x30, 4, 0x1adc0000);
	#endif
	/*
	printstr("before sys543PciInit\r\n");
	sysPciConfigRead(0, 12, 0, 0x00, 4, &tmp1);
	printnum(tmp1);
	*/
	sys543PciInit(0,12,0,INTEL_PCI_VENDOR_ID,PRO1000_546_PCI_DEVICE_ID_XT,0x01);
	sys543PciInit(0,13,0,INTEL_PCI_VENDOR_ID,PRO1000_546_PCI_DEVICE_ID_XT,0x01);
	sys543PciInit(0,14,0,INTEL_PCI_VENDOR_ID,PRO1000_546_PCI_DEVICE_ID_XT,0x01);
	sys543PciInit(0,15,0,INTEL_PCI_VENDOR_ID,PRO1000_546_PCI_DEVICE_ID_XT,0x01);
  	
	/* printstr("================end gei\r\n"); */
	#endif

	#ifdef INCLUDE_SII0680
	sysSii0680PciInit(0,16,0); /* added by yinwx, 2009-03-19 */
	#endif
	
#endif
	
#if defined (INCLUDE_IPIIX4PCI)
    sysIPIIX4Init();
	#if defined (INCLUDE_FD)
		sysIPIIX4FdInit();		/* added by yinwx */
	#endif
    #if defined (INCLUDE_ATA)
		sysIPIIX4ATAInit();
	#endif
	#if defined (INCLUDE_SUPERIO)
		sysSuperioInit();
	#endif
#elif defined (INCLUDE_VT686BPCI)
		sysVT686BInit();
#endif
   
	sysIntInitPICLocal();

#ifdef INCLUDE_SMCFDC37M81X
    /* enable com1, com2, lpt1, floppy, keyboard and mouse ports */ 
    smcFdc37m81xInit();
#endif

#ifdef INCLUDE_W83527
	/* enable keyboard and mouse ports, added by yinwx, 2009-03-06 */
	w83527Init();
#endif

#ifndef INCLUDE_TYCODRV_5_2
    sysSerialHwInit();
#endif /* INCLUDE_TYCODRV_5_2 */

#ifdef INCLUDE_UART
	sysUartHwInit();
#endif
    }

/******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.
*
* RETURNS: N/A
*
*/

void sysHwInit2 (void)
    {
#if 0 /*commented by wangfq*/
#ifdef INCLUDE_RM7K_EXT_INT
    /* enable RM7000 extended interrupts */
    mipsExtndIntEnable();
#endif
#endif /*end commented*/

    /* Connect sys clock interrupts */

    (void) intConnect (INUM_TO_IVEC(IV_TIMER_VEC), sysClkInt, 0); 
    /* connect spurious interrupt handler. */
	#if 0
    (void) intConnect (INUM_TO_IVEC(IV_VIA686_SPURIOUSIRQ_VEC), sysSpuriousInt, 0);
	#endif

 #if 0 /*commented by wangfq*/
    /*
     * Call the I21555 second chip initialization routine.  Can't do this
     * in sysHwInit() becuase the full kernel has not started and 
     * the memory allocation library has not been initialized 
     */

#ifdef INCLUDE_PCI_PCI 
    if (sysCPCIEnable == TRUE)
     sysI21555Init2();
#endif
#endif /*end commented*/

/*added by wangfq*/
#ifdef INCLUDE_PCI
	
    	pciIntLibInit();
  
   
	sysPciIntConnect(); 

#endif /* INCLUDE_PCI */
/*end added*/

#ifndef INCLUDE_TYCODRV_5_2 
    sysSerialHwInit2();

#endif /* INCLUDE_TYCODRV_5_2 */

#ifdef INCLUDE_UART
	sysUartHwInit2();
#endif

#ifdef	INCLUDE_PC_CONSOLE    /* connect keyboard Controller 8042 chip interrupt */    
(void) intConnect (INUM_TO_IVEC (IV_KBD_VEC), kbdIntr, 0);
#endif	/* INCLUDE_PC_CONSOLE */
    }
 
/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of memory
*
* This routine returns the address of the first missing byte of memory, which
* indicates the top of memory.  This should be considered a virtual memory
* address.
*
* NOTE: Do not adjust LOCAL_MEM_SIZE to reserve memory for application
* use.  See sysMemTop() for more information on reserving memory.
*
* RETURNS: The address of the top of memory.
*/

char *sysPhysMemTop (void)
    {
#ifdef LOCAL_MEM_AUTOSIZE
    return (char *)((char *)LOCAL_MEM_LOCAL_ADRS + sdramSize);
#else /*LOCAL_MEM_AUTOSIZE*/
    return (char *)((char *)LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
#endif /*LOCAL_MEM_AUTOSIZE*/
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of logical memory
*
* This routine returns the address of the first unusable byte of memory.
* VxWorks will not use any memory at or above this address.
*
* The user can reserve local memory from the board by declaring the
* macro USER_RESERVED_MEM with the amount of memory to reserve. This
* routine will return a pointer to the first byte of the reserved memory
* area.
*
* RETURNS: The address of the top of usable memory.
*/

char *sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
        {
        memTop = sysPhysMemTop () - USER_RESERVED_MEM;
        }
    return memTop;
    }

/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  Normally, it is called
* only by reboot()--which services ^X--and bus errors at interrupt level.
* However, in some circumstances, the user may wish to introduce a
* <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType    /* parameter passed to ROM to tell it how to boot */
    )
    {
    FUNCPTR pRom = (FUNCPTR) (ROM_TEXT_ADRS + 8);

    /* Flush and Invalidate the caches */
    cacheFlush(DATA_CACHE, (void *)ENTIRE_CACHE, 0);

    (* pRom) (startType);

    return (OK);    /* in case we ever continue from rom monitor */
    }

/*******************************************************************************
*
* sysAutoAck - acknowledge the RM7000 interrupt condition 
*
* This routine acknowledges an RM7000 interrupt for a specified interrupt
* vector.
*
* NOTE:
* This routine must be provided on all RM7000 board support packages.
* Most interrupts are automatically acknowledged in the interrupt service
* routine.
*
* RETURNS: The result of the interrupt acknowledge cycle.
*/
int sysAutoAck
    (
    int vecNum		/* vector num of interrupt that bugs us */
    )
    {
    int result;
	
	int key = intLock();
	
    result = 0;
    switch (vecNum)
	{
        case IV_TIMER_VEC:
	    sysCompareSet (0);
	    break;
	case IV_SWTRAP0_VEC:		/* software trap 0 */
	    return(result = sysSw0Ack ());
	    break;
	case IV_COM1_VEC:		/* COM1 */
		sysIntDisablePIC(IV_VIA686_COM1_OFFSET);
		
	    break;
	case IV_COM2_VEC:		/* COM2 */
		sysIntDisablePIC(IV_VIA686_COM2_OFFSET);
		
		break;
	case IV_SWTRAP1_VEC:		/* software trap 1 */
	    return(result = sysSw1Ack ());
	    break;
	case IV_FPA_UNIMP_VEC:		/* unimplemented FPA oper*/
	case IV_FPA_INV_VEC:		/* invalid FPA operation*/
	case IV_FPA_DIV0_VEC:		/* FPA div by zero */
	case IV_FPA_OVF_VEC:		/* FPA overflow exception */
	case IV_FPA_UFL_VEC:		/* FPA underflow exception */
	case IV_FPA_PREC_VEC:		/* FPA inexact operation */
	    return (result = sysFpaAck ());
	    break;
	default:
	    return (-1);
	    break;
	}
	intUnlock(key);
    return (result);
    }

/*******************************************************************************
*
* sysDisplayTlb - Display the contents of the TLB
*
* RETURNS: N/A
*
*/

void sysDisplayTlb (void)
    {
    UINT tlbEntry, tlbMask, tlbHi, tlbLo0, tlbLo1;

    printf("   INDEX\t    MASK\t      HI\t     LO0\t     LO1\n");
    for (tlbEntry = 0; tlbEntry < TLB_ENTRIES; tlbEntry ++)
        {
	/* Set TLB entry */
	sysTlbIdxSet(tlbEntry);

	/* Read the index entry */
	sysTlbReadIdx();

	/* Read the TLB mask register */
	tlbMask = sysTlbMaskGet();

	/* Read the TLB HI register */
	tlbHi = sysTlbHIGet();

	/* Read the TLB LO0 register */
	tlbLo0 = sysTlbLO0Get();

	/* Read the TLB LO1 register */
	tlbLo1 = sysTlbLO1Get();

        printf("%8x\t%8x\t%8x\t%8x\t%8x\n", tlbEntry, tlbMask, tlbHi, tlbLo0, tlbLo1);
        }
    }

/*******************************************************************************
*
* sysSetTlb - Programs the available TLB for translations 
*
* Programs the onboard TLB table.  Entries 0 to IO_SPACE_NUM_TLBS are
* reserved.  <size> should be a multiple of 8K, 32K, 128K, 512K, 4M,
* 16M, or 32M.  <mode> is the bitwise OR of the one of the first 5
* macros and a combination of one or more of the last three:
*
* TLBLO_NONC_WT - Cacheable, noncoherent, write-through, no write allocate
* TLBLO_NONC_WTA - Cacheable, noncoherent, write-through, write allocate
* TLBLO_NC - Uncacheable, blocking 
* TLBLO_NONC - Cacheable non-coherent, write-back
* TLBLO_CUW - Uncacheable, non-blocking
* TLBLO_D   - TLB writeable
* TLBLO_V   - TLB valid
* TLBLO_G   - TLB global
*
* RETURNS: OK or ERROR if size is invalid
*
*/

STATUS sysSetTlb 
    (
    ULONG virtAddr,
    ULONG physAddr,
    ULONG size,		
    UINT mode
    )
    {
    #if 1 /*commented by wangfq*/
    UINT tlbEntry;
    UINT pagesize=32*1024*1024, numTlbs;

    /* Pick the largest page size that is aligned with the address */
    do
        {
        /* Is it aligned correctly */
        if ((size & (pagesize - 1)) == 0)
            break;

        pagesize >>= 2;
        } while (pagesize >= 8*1024);

    /* Invalid <size> parameter */
    if (pagesize < 8*1024)
        {
        printf("sysSetTbl: <size> must be a multiple of 8K,32K,128K,512K,4M,16M, or 32MB\n");
        return (ERROR);
        }
 
    pagesize /= 2;		/* Per TLB */
    numTlbs = size/pagesize;
    mode &= TLBLO_MODE;

    /* Not enough TLBS ? */
    if (numTlbs > 2*(TLB_ENTRIES - IO_SPACE_NUM_TLBS))
        {
        printf("sysSetTbl: Not enough TLBs to map requested size (0x%x max)\n", 32*1024*1024*(TLB_ENTRIES-IO_SPACE_NUM_TLBS));
        return (ERROR);
        }
 
    for (tlbEntry=IO_SPACE_NUM_TLBS; tlbEntry < TLB_ENTRIES/*, numTlbs>0*/; tlbEntry ++)
        {
        if (sysSetTlbEntry(tlbEntry,virtAddr,physAddr,(pagesize|mode)) == ERROR)
            {
            printf("sysSetTbl: Got error on TBL %d !!!!\n", tlbEntry);
            sysClearTlb();
            return (ERROR);
            }
 		virtAddr += 2*pagesize;	
 		physAddr += 2*pagesize;
		
        }
	#endif /*end commented*/

    return (OK);
    }

/*******************************************************************************
*
* sysClearTlb - clear the translation lookaside buffer
*
* This routine clears the entries in the translation lookaside buffer (TLB)
* for the MIPS RM7000 CPU.
*
* RETURNS: N/A
*
*/

void sysClearTlb (void)
    {
    FAST int tlbEntry;printf("!!!!!!!!!!!!!!!!!!!!!\n");
    for (tlbEntry = 0 /*modified by wangfq,original*/; tlbEntry < TLB_ENTRIES; tlbEntry ++)
        sysClearTlbEntry (tlbEntry);
    }
#if 0 /*commented by wangfq*/
#ifdef INCLUDE_PCI
/*******************************************************************************
*
* sysCPCISlaveMap - Enable mapping to local DRAM from Compact PCI bus
*
* This routine maps an address from Compact PCI bus to local DRAM such
* that access to <cpciAddress> accesses memory starting at 0 and the
* window size is <memSize>.  <prefetchable> determine if the memory     
* window is pre-fetchable.  <cpciAddress> and <memSize> should meet
* the following criteria: <cpciAddress> & (memSize - 1) != 0.  If
* this criteria is not satisfied, BRIDGE_INVALID_BASE_SIZE is returned
* and the mapping is not done.
* <memSize> should be a power of 2.
* If <cpciAddress> is 0xFFFFFFFF then the base address is not set.  This
* allows a Compact PCI master to set the base during probing.
*
* RETURNS: OK if successful
*          BRIDGE_INVALID_SIZE - if <memSize> is not a power of 2
*          BRIDGE_INVALID_BASE_SIZE - Invalid <cpciAddress>, <memSize> combo
*/
STATUS sysCPCISlaveMap 
    (
    UINT cpciAddress,
    UINT memSize,
    BOOL prefetchable
    )
    {
     return (sysI21555SlaveSet(I21555_MEM_WINDOW1, cpciAddress, PCI2DRAM_BASE_ADRS, memSize, prefetchable));
    }

/*******************************************************************************
*
* sysCPCIMasterMap - Enable mapping to Compact PCI from the CPU
*
* This routine sets up translation to Compact PCI bus from local CPU
* that access to <cpciAddress>.  window size is <cpciSize>.  
* <prefetchable> determine if the memory window is pre-fetchable.  
* <cpciSize> should be a power of 2.
* <cpciAddress> and <cpciSize> should meet the following criteria: 
* <cpciAddress> & (cpciSize - 1) != 0.  If
* this criteria is not satisfied, BRIDGE_INVALID_BASE_SIZE is returned
* and the mapping is not done.
*
* RETURNS: CPU address if successful
*          BRIDGE_WIN_NOT_ENABLED - if window cannot be setup becuase a
*                                   Compact PCI host must first setup the
*                                   window.
*          BRIDGE_WIN_SIZE_TOO_LARGE - if <cpciSize> larger than can be mapped
*          BRIDGE_INVALID_SIZE - if <cpciSize> is not a power of 2
*          BRIDGE_INVALID_BASE_SIZE - Invalid <cpciAddress>, <cpciSize> combo
*/
UINT sysCPCIMasterMap 
    (
    UINT cpciAddress,
    UINT cpciSize,
    BOOL prefetchable
    )
    {
    STATUS stat;
    UINT cpuAddr;

    if ((stat = sysI21555MasterSet(I21555_MEM_WINDOW1, cpciAddress, cpciSize, prefetchable, &cpuAddr)) != OK)
        {
        printf("sysCPCIMasterMap: Failed with status 0x%x\n", stat);
        cpuAddr = -1;
        }
     
    return (cpuAddr);
    }
#endif /* INCLUDE_PCI */
#endif /*end commented*/
/*******************************************************************************
*
* sysCpuClockRate - Get the CPU clock rate (in Hz)
*
* RETURNS: CPU clock rate (in Hz)
*
*/
UINT sysCpuClockRate (void)
    {
    /*commented by wangfq*/
    #if 0
    UINT c0Reg = sysConfigGet();

    c0Reg = ((c0Reg & CF_7_EC) >> CF_7_EC_AL) + 2;
 
    return (c0Reg * BUS_CLK_RATE);
	#endif
	/*end commented*/
	return((UINT)(SYSAD_CLK_RATE * SYSAD_CLK_RATE_TIMES)); /*added by wangfq*/
    }
#if 0 /*commented by wangfq*/
#ifdef INCLUDE_PCI
/******************************************************************************
*
* sysLocalToBusAdrs - convert a local address to a Compact PCI bus address
*
* This routine gets the Compact PCI bus address that accesses a specified 
* local memory address.  See routine sysI21555BusToLocalAdrs() for more 
* information on the parameters.
*
* RETURNS: OK or ERROR if translation is not possible
*
* SEE ALSO: sysBusToLocalAdrs(), sysI21555LocalToBusAdrs()
*/

STATUS sysLocalToBusAdrs
    (
    int  adrsSpace,     /* bus address space in which busAdrs resides,  */
    char *localAdrs,    /* local address to convert                     */
    char **pBusAdrs     /* where to return bus address                  */
    )
    {
    return (sysI21555LocalToBusAdrs (adrsSpace, (UINT)localAdrs, (UINT *)*pBusAdrs));
    }

/******************************************************************************
*
* sysBusToLocalAdrs - convert a Compact PCI bus address to a local address
*
* This routine gets the local address that accesses a specified Compact PCI 
* bus memory address. See routine sysI21555BusToLocalAdrs() for more 
* information on the parameters.
*
* RETURNS: OK or ERROR if translation is not possible
*
* SEE ALSO: sysLocalToBusAdrs(), sysI21555BusToLocalAdrs
*/
 
STATUS sysBusToLocalAdrs
    (
    int  adrsSpace,     /* bus address space in which busAdrs resides,  */
    char *busAdrs,      /* bus address to convert                       */
    char **pLocalAdrs   /* where to return local address                */
    )
    {
    return (sysI21555BusToLocalAdrs (adrsSpace, (UINT)busAdrs, (UINT *)*pLocalAdrs));
    }
#endif /* INCLUDE_PCI */

/******************************************************************************
*
* sysBusIntAck - acknowledge (clears) a Compact PCI bus interrupt
*
* This routine acknowledges a specified bus interrupt.  <intLevel>
* should be INTSET_INTA, INTSET_INTB, INTSET_INTC, or INTSET_INTD.
*
* RETURNS: OK or ERROR if <intLevel> is invalid.
*/

int sysBusIntAck
    (
    int intLevel        /* interrupt level to acknowledge */
    )
    {
    if ((intLevel != INTSET_INTA) && (intLevel != INTSET_INTB) &&
        (intLevel != INTSET_INTC) && (intLevel != INTSET_INTD))
     return ((int)ERROR);

    /* Clear our local asserted Compact PCI Interrupt */
    *(volatile UCHAR *)PLD_REG(INTCLR) = intLevel;

    return ((int)OK);
    }

/******************************************************************************
*
* sysBusIntGen - generate a Compact PCI bus interrupt
*
* This routine generates a bus interrupt for a specified level, given
* by <level>.  <level> should be INTSET_INTA, INTSET_INTB, INTSET_INTC,
* or INTSET_INTD.  <vector> is unused.
*
* RETURNS: OK or ERROR if <level> is invalid.
*/

STATUS sysBusIntGen
    (
    int level,          /* bus interrupt level to generate          */
    int vector          /* interrupt vector to return (0-255)       */
    )
    {
    if ((level != INTSET_INTA) && (level != INTSET_INTB) &&
        (level != INTSET_INTC) && (level != INTSET_INTD))
     return (ERROR);

    *(volatile UCHAR *)PLD_REG(INTSET) = level;
    return (OK);
    }

#ifdef INCLUDE_PCI
/******************************************************************************
* sysMailboxConnect - connect a routine to a mailbox interrupt
*
* This routine specifies the interrupt service routine to be called at each
* mailbox interrupt.  By default it connects to mailbox 1.  See 
* sysI21555MailboxConnect() for more information.
*
* RETURNS: OK or ERROR
*
* SEE ALSO: sysMailboxEnable(), sysI21555MailboxConnect()
*/

STATUS sysMailboxConnect
    (
    FUNCPTR routine,    /* routine called at each mailbox interrupt */
    int     arg         /* argument with which to call routine      */
    )
    {
    return (sysI21555MailboxConnect(routine,1,arg));
    }

/******************************************************************************
*
* sysMailboxEnable - enable the mailbox interrupt
*
* This routine enables the mailbox interrupt.  See sysI21555MailboxEnable()
* for more information.
*
* RETURNS: OK or ERROR
*
* SEE ALSO: sysMailboxConnect(), sysI21555MailboxEnable()
*/

STATUS sysMailboxEnable
    (
    INT8 *mailboxAdrs           /* mailbox address */
    )
    {
    return (sysI21555MailboxEnable(mailboxAdrs));
    }
#endif /* INCLUDE_PCI */

#endif /*end commented*/

#if 0 /*commented by wangfq*/
/*******************************************************************************
*
* sysIntDisable - disable a Compact PCI bus interrupt level
*
* This routine disables a specified bus interrupt level.  Disabling
* the level results is the board not receiving the interrupt.  <intLevel> 
* should be INTSET_INTA, INTSET_INTB, INTSET_INTC, or INTSET_INTD.  
*
* RETURNS: OK or ERROR if <intLevel> is invalid.
*
* SEE ALSO: sysIntEnable()
*/
 
STATUS sysIntDisable
    (
    int intLevel       
    )
    {
    if ((intLevel != INTSET_INTA) && (intLevel != INTSET_INTB) &&
        (intLevel != INTSET_INTC) && (intLevel != INTSET_INTD))
     return (ERROR);

    *(volatile UCHAR *)PLD_REG(INTMASK) |= intLevel;
    return (OK);
    }

/*******************************************************************************
*
* sysIntEnable - enable a Compact PCI bus interrupt level
*
* This routine enables a specified bus interrupt level.  Enabling
* the level results is the board receiving the interrupt if either
* an external source asserts the interrupt or sysBusIntGen() is
* called with the same interrupt level.  <intLevel> 
* should be INTSET_INTA, INTSET_INTB, INTSET_INTC, or INTSET_INTD.  
*
* RETURNS: OK or ERROR if <intLevel> is invalid.
*
* SEE ALSO: sysIntDisable()
*/
 
STATUS sysIntEnable
    (
    int intLevel       
    )
    {
    if ((intLevel != INTSET_INTA) && (intLevel != INTSET_INTB) &&
        (intLevel != INTSET_INTC) && (intLevel != INTSET_INTD))
     return (ERROR);

    *(volatile UCHAR *)PLD_REG(INTMASK) &= ~intLevel;
    return (OK);
    }

#endif /*end commented*/
#if 0 /*commented by wangfq*/
/*******************************************************************************
*
* sysGtIntDisable - disable a GT64240 interrupt device
*
* This routine disables a specific device from generating interrupts in the
* GT64240A.  Which interrupt to disable is determined by the supplied
* interrupt vector number which is used to determine the 64240 register
* to access and bit to clear to disable interrupts from this source.
*
* RETURNS: OK or ERROR if <intVector> is invalid.
*
* SEE ALSO: sysGtIntEnable()
*/

STATUS sysGtIntDisable (int vector)
    {
    UINT32    mask;
    int       retval = OK;

    /* determine which register needs to be accessed */

    if (vector >= IV_ETHERNET0_VEC && vector <= IV_COMM_VEC)
        {
        /* modify vector so it gives the bit number in the reg to clr */

        vector -= IV_ETHERNET0_VEC;
        /* read mask register and clr bit for this source */

        mask = gtReadInternalReg ((UINT)GT_PCI0_INT_MASK_HIGH);
        mask &= ~(GT_INT_ETH0 << vector);
        gtWriteInternalReg ((UINT)GT_PCI0_INT_MASK_HIGH, mask);
        }
    else if (vector >= IV_DEV_BUS_VEC && vector <= IV_PCI1_IN_H_VEC)
        {
        /* modify vector so it gives the bit number in the reg to clr */

        vector -= IV_DEV_BUS_VEC;
        /* read mask register and clr bit for this source */

        mask = gtReadInternalReg ((UINT)GT_PCI1_INT_MASK_LOW);
        mask &= ~(GT_INT_DEV << vector);
        gtWriteInternalReg ((UINT)GT_PCI1_INT_MASK_LOW, mask);
        }
    else
        retval = ERROR;

    return (retval);
    }
 
/*******************************************************************************
*
* sysGtIntEnable - enable a GT64240 interrupt device
*
* This routine enables a specific device for generating interrupts in the
* GT64240A.  Which interrupt to enable is determined by the supplied
* interrupt vector number which is used to determine the 64240 register
* to access and bit to set to enable interrupts from this source.
*
* RETURNS: OK or ERROR if <intVector> is invalid.
*
* SEE ALSO: sysGtIntDisable()
*/

STATUS sysGtIntEnable (int vector)
    {
    UINT32    mask;
    int       retval = OK;

    /* determine which register needs to be accessed */

    if (vector >= IV_ETHERNET0_VEC && vector <= IV_COMM_VEC)
        {
        /* modify vector so it gives the bit number in the reg to set */

        vector -= IV_ETHERNET0_VEC;
        /* read mask register and set mask bit for this source */

        mask = gtReadInternalReg ((UINT)GT_PCI0_INT_MASK_HIGH);
        mask |= (GT_INT_ETH0 << vector);
        gtWriteInternalReg ((UINT)GT_PCI0_INT_MASK_HIGH, mask);
        }
    else if (vector >= IV_DEV_BUS_VEC && vector <= IV_PCI1_IN_H_VEC)
        {
        /* modify vector so it gives the bit number in the reg to set */

        vector -= IV_DEV_BUS_VEC;
        /* read mask register and set mask bit for this source */

        mask = gtReadInternalReg ((UINT)GT_PCI1_INT_MASK_LOW);
        mask |= (GT_INT_DEV << vector);
        gtWriteInternalReg ((UINT)GT_PCI1_INT_MASK_LOW, mask);
        }
    else
        retval = ERROR;

    return (retval);
    }


/******************************************************************************
*
* sysNvRamGet - get the contents of non-volatile RAM
*
* This routine copies the contents of non-volatile memory into a specified
* string.  The string will be terminated with an EOS.
*
* RETURNS: OK, or ERROR if access is outside the non-volatile RAM range.
*
* SEE ALSO: sysNvRamSet()
*/
STATUS sysNvRamGet
    (
    char *string,    /* where to copy non-volatile RAM    */
    int strLen,      /* maximum number of bytes to copy   */
    int offset       /* byte offset into non-volatile RAM */
    )
    {
    volatile char *     pNvRam;
    int   ix;

    offset += NV_BOOT_OFFSET;   /* boot line begins at <offset> = 0 */

    if ((offset < 0) || (strLen < 0) || ((offset + strLen) > NV_RAM_SIZE))
        return (ERROR);

    pNvRam  = (volatile char *) (BBRAM_ADRS + offset);

    if (strLen == 0)
        {
        *string = EOS;
        return (OK);
        }
 
    for (ix = 0; ix < strLen; ix ++)
        string[ix] = pNvRam[ix]; 
 
    string [ix] = EOS;  
 
    return (OK);
    }


/*******************************************************************************
*
* sysNvRamSet - write to non-volatile RAM
*
* This routine copies a specified string into non-volatile RAM.
*
* RETURNS: OK, or ERROR if access is outside the non-volatile RAM range.
*
* SEE ALSO: sysNvRamGet()
*/
STATUS sysNvRamSet
    (
    char *string,     /* string to be copied into non-volatile RAM */
    int strLen,       /* maximum number of bytes to copy           */
    int offset        /* byte offset into non-volatile RAM         */
    )
    {
    volatile char *pNvRam;
    int   ix;

    offset += NV_BOOT_OFFSET;   /* boot line begins at <offset> = 0 */

    if ((offset < 0) || (strLen < 0) || ((offset + strLen) > NV_RAM_SIZE))
        return (ERROR);

    pNvRam = (volatile char *) (BBRAM_ADRS + offset);

    for (ix = 0; ix < strLen; ix ++)
        {
        *pNvRam = (char) *string;
        string ++;
        pNvRam ++;
        }
 
    return (OK);
    }

/***************************************************************************
*
* sysEnetAddrSet - write MAC address to NVRAM
*
* this routine writes the MAX address to the NVRAM
*
* RETURNS: OK, or ERROR.
*
* SEE ALSO: sysEnetAddrGet()
*/

void sysEnetAddrSet 
    (
    UINT8 byte0, 
    UINT8 byte1, 
    UINT8 byte2, 
    UINT8 byte3, 
    UINT8 byte4, 
    UINT8 byte5
    )
    {
    UINT8 macAddr[6] ;

    macAddr[0] = byte0 ;
    macAddr[1] = byte1 ;
    macAddr[2] = byte2 ;
    macAddr[3] = byte3 ;
    macAddr[4] = byte4 ;
    macAddr[5] = byte5 ;

    }

/***************************************************************************
*
* sysEnetAddrGet - get MAC address from NVRAM
*
* get the MAX address from the NVRAM
*
* RETURNS: OK, or ERROR.
*
* SEE ALSO: sysEnetAddrSet()
*/

void sysEnetAddrGet
    (
    UINT8 *adrs
    )
    {
    int     ret;
    
    /* get the MAC address */
    ret = sysNvRamGet(adrs, 6,
                (int) ((ULONG) BB_ENET - (ULONG) BBRAM_ADRS -
                       (int) NV_BOOT_OFFSET));

    /* if error in get, use default MAC */

    if (ret != OK)
        {
        bfill ((char *)adrs, 6, 0xff);
        adrs [0] = LNMSB (ENET_DEFAULT);
        adrs [1] = LNLSB (ENET_DEFAULT);
        adrs [2] = LLSB (ENET_DEFAULT);
        }
        
        adrs [0] = LNMSB (ENET_DEFAULT);
        adrs [1] = LNLSB (ENET_DEFAULT);
        adrs [2] = LLSB (ENET_DEFAULT);


    }
#endif /*end commented*/

/************************************************************************
*
* sysDelay - provide a momentary delay via a task switch
*
* RETURNS: N/A
*/

void sysDelay()
    {
    /* Force context switch */
    taskDelay(0);
    }

/***************************************************************************
*
* sysBonito64Init - Initialize the Bonito64 North Bridge for the Longson2eBox Computer.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysBonito64Init (void)
    {UINT inten=BONITO_INTEN;
    
    	BONITO_INTENCLR = 0xffffffff;
		inten=BONITO_INTEN;
   
    }

#if defined (INCLUDE_IPIIX4PCI)
LOCAL void sysIPIIX4Init(void)
    {
    UINT8 data8;
    UINT32 data32;
#if 0    
    /* enable interrupt routing on PIIX4 for PIRQ[A:D] */
    /* A,B on IRQ10, C,D on IRQ11 */

    /* NOTE: DO NOT use iPiix4PciIntrRoute() or iPiix4PciGetIntr().
       Those library functions assume that PCI interrupts are wired
       to IRQ 9, 10, 11, 12, but Malta only uses 10 (for A and B) and 11
       (for C and D). */
    pciConfigOutLong(0, MALTA_DEVNUM_PIIX4, 0, PIIX4_PCI_PIRQRC,
	     (PIIX4_PIRQRC_PCIA_IR_IRQ10 << PIIX4_PIRQRC_PCIA_IR_IRQA_SHF) |
	     (PIIX4_PIRQRC_PCIB_IR_IRQ10 << PIIX4_PIRQRC_PCIB_IR_IRQB_SHF) |
	     (PIIX4_PIRQRC_PCIC_IR_IRQ11 << PIIX4_PIRQRC_PCIC_IR_IRQC_SHF) |
	     (PIIX4_PIRQRC_PCID_IR_IRQ11 << PIIX4_PIRQRC_PCID_IR_IRQD_SHF));
#endif
    /* Level trigger on IRQ3 and IRQ4 */
    sysOutByte(IPIIX4_PCI_ELCR1,
	       (PIIX4_ELCR1_IRQ3LEVEL_BIT /*| PIIX4_ELCR1_IRQ4LEVEL_BIT*/));

    /* Level trigger on IRQ10 and IRQ11 */
    /*sysOutByte(IPIIX4_PCI_ELCR2,
	       (PIIX4_ELCR2_IRQ10LEVEL_BIT | PIIX4_ELCR2_IRQ11LEVEL_BIT));*/
	       
    /* enable serial IRQ */ 
    /* Here must be commentted, or IPIIX4 makes system suspended!! by yinwx */
    pciConfigInLong (PCI_BUS_LOCAL,
	     LSN2ECPCISBC_DEVNUM_PIIX4,
	     PIIX4_PCI_FUNCTION_BRIDGE,
	     PIIX4_PCI_GENCFG,
	     &data32);

    pciConfigOutLong(PCI_BUS_LOCAL,
	     LSN2ECPCISBC_DEVNUM_PIIX4,
	     PIIX4_PCI_FUNCTION_BRIDGE,
	     PIIX4_PCI_GENCFG,
	     data32 | PIIX4_GENCFG_SERIRQ_BIT);
		 0x3bfec001);

    pciConfigInByte (PCI_BUS_LOCAL,
	     LSN2ECPCISBC_DEVNUM_PIIX4,
	     PIIX4_PCI_FUNCTION_BRIDGE,
	     PIIX4_PCI_SERIRQC,
	     &data8);

    pciConfigOutByte(PCI_BUS_LOCAL,
	     LSN2ECPCISBC_DEVNUM_PIIX4,
	     PIIX4_PCI_FUNCTION_BRIDGE,
	     PIIX4_PCI_SERIRQC,
	     data8 | PIIX4_SERIRQC_ENABLE_BIT | PIIX4_SERIRQC_CONT_BIT);
	
    pciConfigInByte (PCI_BUS_LOCAL,
		     LSN2ECPCISBC_DEVNUM_PIIX4, 
		     PIIX4_PCI_FUNCTION_BRIDGE,
		     PIIX4_PCI_RTC,
		     &data8);
	
     pciConfigOutByte(PCI_BUS_LOCAL,
		     LSN2ECPCISBC_DEVNUM_PIIX4,
		     PIIX4_PCI_FUNCTION_BRIDGE,
		     PIIX4_PCI_RTC,
		     data8 | PIIX4_RTC_URAM_ENABLE_BIT);
	 
	/*pciConfigOutLong(PCI_BUS_LOCAL,
	     LSN2ECPCISBC_DEVNUM_PIIX4,
	     PIIX4_PCI_FUNCTION_POWER,
	     0x90, 
	     0x3fc1); SM BUS base address, func 3 */
		    
}

#if defined (INCLUDE_ATA)
LOCAL void sysIPIIX4ATAInit(void)
    {
    /* configure ATA controller *(volatile char *)0xbfd002f8='<';*/
    /*pciConfigOutWord (PCI_BUS_LOCAL,
		      LSN2ECPCISBC_DEVNUM_PIIX4,
              IPIIX4_PCI_FUNC1,
		      IPIIX4_PCI_PCICMD,
              (IPIIX4_PCI_PCICMD_BME | IPIIX4_PCI_PCICMD_IOSE));
    
    pciConfigOutByte (PCI_BUS_LOCAL,
		      LSN2ECPCISBC_DEVNUM_PIIX4,
                      IPIIX4_PCI_FUNC1,
		      IPIIX4_PCI_MLT,
		      IPIIX4_PCI_MLT_MLTCV);
        
    pciConfigOutLong (PCI_BUS_LOCAL,
		      LSN2ECPCISBC_DEVNUM_PIIX4,
              IPIIX4_PCI_FUNC1,
		      IPIIX4_PCI_IDETIM,
		      IPIIX4_PCI_IDETIM_VAL);*/
    }
#endif /* INCLUDE_ATA */

#if defined (INCLUDE_FD)		/* added by yinwx, begin */
LOCAL void sysIPIIX4FdInit(void)
	{
	UINT8 data8;
	UINT32 data32;
	
	/* configure DMA controller */
	sysOutByte(IPIIX4_PCI_FD_DCM, 
				(IPIIX4_PCI_FD_DCM_CASCADE | IPIIX4_PCI_FD_DCM_AUTOINIT));
	sysOutByte(IPIIX4_PCI_FD_RWAMB, IPIIX4_PCI_FD_RWAMB_MASKALL);

	sysOutByte(0x0B, 0x52);
	sysOutByte(0x0F, 0x00);

	/* configure floppy disk controller */
	pciConfigInByte(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			(IPIIX4_PCI_DEVRESD + 1),
			&data8);
	pciConfigOutByte(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			(IPIIX4_PCI_DEVRESD + 1),
			(data8 | IPIIX4_PCI_RESENDEV5));
	pciConfigInByte(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			(IPIIX4_PCI_DEVRESD + 1),
			&data8);
	pciConfigOutByte(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			(IPIIX4_PCI_DEVRESD + 1),
			(data8 | IPIIX4_PCI_FDCMONEN));
	pciConfigInLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			IPIIX4_PCI_DEVRESB,
			&data32);
	pciConfigOutLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			IPIIX4_PCI_DEVRESB,
			(data32 | IPIIX4_PCI_EIOENDEV5));
/*	pciConfigInLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			IPIIX4_PCI_DEVRESB,
			&data32);
	pciConfigOutLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			IPIIX4_PCI_DEVRESB,
			(data32 & IPIIX4_PCI_FDCDECSEL));*/
/*	pciConfigInLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			0x67,
			&data8);*/
/*	pciConfigOutLong(PCI_BUS_LOCAL,
			LSN2ECPCISBC_DEVNUM_PIIX4,
			IPIIX4_PCI_FUNC3,
			0x67,
			0x98);  This is for serial port enable, added by yinwx */
	}
#endif /* INCLUDE_FD */			/* added by yinwx, end */

#ifdef INCLUDE_SMCFDC37M81X
/***************************************************************************
*
* smcFdc37m81xInit() - Initialize the Super I/O chip for normal operation
*
* This routine enables the serial, parallel, Floppy disk, Keyboard and
* mouse ports on the Fdc37m81x Super I/O chip.
*
* NOTE:
* This routine should not be called directly by the user.
* 
* RETURNS: N/A
*/

LOCAL void smcFdc37m81xInit(void)
    {
    
    /* enter config mode */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_KEY_START);
	sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_KEY_START);

#if 1 /*if 1, serial output messily*/
    /* serial port com 2 (tty1) */
    /* select device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DEVNUM);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_DEVNUM_COM2);

    /* set base address */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASEHI);
    sysOutByte(SMSC_DATA_OFS, (MALTA_SMSC_COM2_ADR >> 8) & 0xff);
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASELO);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_COM2_ADR & 0xff);

    /* Select IRQ */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_TTY1);

    /* mode register (enable high speed) */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_MODE);
    sysOutByte(SMSC_DATA_OFS, 0/*SMSC_CONFIG_MODE_HIGHSPEED_BIT*/); /* modified by yinwx */

    /* activate device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_ACTIVATE);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_ACTIVATE_ENABLE_BIT);

    
    /* serial port com 1 (tty0) */
    /* select device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DEVNUM);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_DEVNUM_COM1);

    /* set base address */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASEHI);
    sysOutByte(SMSC_DATA_OFS, (MALTA_SMSC_COM1_ADR >> 8) & 0xff);
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASELO);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_COM1_ADR & 0xff);

    /* Select IRQ */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_TTY0);

    /* mode register (enable high speed) */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_MODE);
    sysOutByte(SMSC_DATA_OFS, 0/*SMSC_CONFIG_MODE_HIGHSPEED_BIT*/); /* modified by yinwx */

    /* activate device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_ACTIVATE);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_ACTIVATE_ENABLE_BIT);
#endif
    /* parallel port */
    /* select device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DEVNUM);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_DEVNUM_PARALLEL);

    /* Set base address */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASEHI);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_1284_ADR >> 8);
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASELO);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_1284_ADR & 0xff);
    
    /* Select IRQ */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_1284);

    /* Activate device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_ACTIVATE);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_ACTIVATE_ENABLE_BIT);

    /* Floppy disk */#if 0
    /* select device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DEVNUM);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_DEVNUM_FDD);

	/* Set base address */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASEHI);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_FDD_ADR >> 8);
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_BASELO);
    sysOutByte(SMSC_DATA_OFS, MALTA_SMSC_FDD_ADR & 0xff);

	/* Select IRQ for floppy */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_FDD);

	/* Select DMA Channel for floppy */
	sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DRQ);
    sysOutByte(SMSC_DATA_OFS, 0x02);
    
    /* Activate device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_ACTIVATE);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_ACTIVATE_ENABLE_BIT);
	#endif
      
    /* Keyboard, Mouse */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_DEVNUM);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_DEVNUM_KYBD);
    
    /* select IRQ for keyboard */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_KYBD);

    /* select IRQ for mouse */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_IRQ2);
    sysOutByte(SMSC_DATA_OFS, MALTA_INTLINE_MOUSE);

    /* activate device */
    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_ACTIVATE);
    sysOutByte(SMSC_DATA_OFS, SMSC_CONFIG_ACTIVATE_ENABLE_BIT);

    sysOutByte(SMSC_CONFIG_OFS, SMSC_CONFIG_KEY_EXIT);
    }
#endif
#endif /* INCLUDE_IPIIX4PCI */

#ifdef INCLUDE_SII0680
/***************************************************************************
*
* sysSii0680PciInit() - Initialize the PCI-IDE Controller chip
* added by yinwx, 2009-03-20
*
* This routine initiallizes related regs in SiI0680 
*
* NOTE:
* This routine should not be called directly by the user.
* 
* RETURNS: OK/ERROR
*/

LOCAL STATUS sysSii0680PciInit
	(
	UINT32 pciBus,
	UINT32 pciDevice,
	UINT32 pciFunc
	)
    {
#if 0
    UINT32 class_rev   = 0;
    UINT8 tmpbyte  = 0;
    UINT8 BA5_EN   = 0;
#endif
	UINT32 BAR0;
	UINT32 BAR1;
#if 0
    pciConfigInLong(pciBus, pciDevice, pciFunc,
					PCI_CFG_REVISION, &class_rev);
    class_rev &= 0xff;
    pciConfigOutByte(pciBus, pciDevice, pciFunc,
					PCI_CFG_CACHE_LINE_SIZE, (class_rev) ? 1 : 255);

    pciConfigInByte(pciBus, pciDevice, pciFunc,
					0x8A, &BA5_EN);

    pciConfigOutByte(pciBus, pciDevice, pciFunc,
					0x80, 0x00); /* Transfer mode- 00:PIO without IORDY */
    pciConfigOutByte(pciBus, pciDevice, pciFunc,
					0x84, 0x00); /* Transfer mode- 00:PIO without IORDY */
    pciConfigInByte(pciBus, pciDevice, pciFunc,
					0x8A, &tmpbyte);
    switch(tmpbyte & 0x30) {
        case 0x00:
            /* 133 clock attempt to force it on */
            pciConfigOutByte(pciBus, pciDevice, pciFunc,
            				0x8A, tmpbyte|0x10);
        case 0x30:
            /* if clocking is disabled */
            /* 133 clock attempt to force it on */
            pciConfigOutByte(pciBus, pciDevice, pciFunc,
            				0x8A, tmpbyte & ~0x20);
        case 0x10:
            /* 133 already */
            break;
        case 0x20:
            /* BIOS set PCI x2 clocking */
            break;
    }
	pciConfigInByte(pciBus, pciDevice, pciFunc,
					0x8A, &tmpbyte);

    pciConfigOutByte(pciBus, pciDevice, pciFunc,
					0xA1, 0x72);
    pciConfigOutWord(pciBus, pciDevice, pciFunc,
					0xA2, 0x328A);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xA4, 0x62DD62DD/*0x328a328a*/);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xA8, 0x43924392);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xAC, 0x40094009);
    pciConfigOutByte(pciBus, pciDevice, pciFunc,
					0xB1, 0x72);
    pciConfigOutWord(pciBus, pciDevice, pciFunc,
					0xB2, 0x328A);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xB4, 0x62DD62DD/*0x328a328a*/);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xB8, 0x43924392);
    pciConfigOutLong(pciBus, pciDevice, pciFunc,
					0xBC, 0x40094009);
#endif
	pciConfigInLong(pciBus, pciDevice, pciFunc,
					0x10, &BAR0);
	pciConfigInLong(pciBus, pciDevice, pciFunc,
					0x14, &BAR1); 
	Sii0680_baseaddr = (BAR0 & (~0x1)) | (0xbfd00000);
	Sii0680_ioaddr1 = (BAR0 & (~0x1)) + 0x80;/*8620 IDE register address*/
	Sii0680_ioaddr2 = (BAR0 & (~0x1)) + 0x8e;
	/*
	printstr("Sii0680_ioaddr1 : ");
	printnum(Sii0680_ioaddr1);
	printstr("\r\n");
	printstr("Sii0680_ioaddr2 : ");
	printnum(Sii0680_ioaddr2);
	printstr("\r\n");
	*/
    return OK;
	}
#endif /* INCLUDE_SII0680 */

#ifdef INCLUDE_W83527
/***************************************************************************
*
* w83527Init() - Initialize the Super I/O chip for normal operation
* added by yinwx, 2009-03-06
*
* This routine enables the Keyboard and mouse ports on the 
* w83527Init Super I/O chip.
*
* NOTE:
* This routine should not be called directly by the user.
* 
* RETURNS: N/A
*/

LOCAL void w83527Init(void)
    {
    /* enter config mode */
	sysOutByte(W83527EFIR, W83527_CONFIG_MODE_START);
	sysOutByte(W83527EFIR, W83527_CONFIG_MODE_START);

	/* Keyboard, Mouse */
	sysOutByte(W83527EFIR, W83527_CHIPCTRL_DEVSEL);
    sysOutByte(W83527EFDR, W83527_CONFIG_DEVNUM_KYBD);

	/* select IRQ for keyboard */
    sysOutByte(W83527EFIR, W83527_CONFIG_IRQ);
    sysOutByte(W83527EFDR, MALTA_INTLINE_KYBD);

    /* select IRQ for mouse */
    sysOutByte(W83527EFIR, W83527_CONFIG_IRQ2);
    sysOutByte(W83527EFDR, MALTA_INTLINE_MOUSE);

	/* CR-f0 configuration */
	sysOutByte(W83527EFIR, 0xf0);
    sysOutByte(W83527EFDR, 0x80); /** Gate A20 and KBRST# software control */

    /* activate device */
    sysOutByte(W83527EFIR, W83527_CONFIG_ACTIVE);
    sysOutByte(W83527EFDR, W83527_CONFIG_ACTIVE_ENABLE);

	/* exit config mode */
	sysOutByte(W83527EFIR, W83527_CONFIG_MODE_EXIT);

	/* accesses to I/O Port 092h are passed on to the LPC bus */
	_wrmsr(DIVIL_MSR_REG(KEL_CTRL), 0, 0);
	}
#endif /* INCLUDE_W83527 */
/***********************************************************************************/
#if 0
void sstEraseSector(unsigned int offset)
{
	*((volatile unsigned char *)(0xbfc00000+0x5555)) = 0xAA;
	*((volatile unsigned char *)(0xbfc00000+0x2aaa)) = 0x55;
	*((volatile unsigned char *)(0xbfc00000+0x5555)) = 0x80;
	*((volatile unsigned char *)(0xbfc00000+0x5555)) = 0xAA;
	*((volatile unsigned char *)(0xbfc00000+0x2aaa)) = 0x55;
	*((volatile unsigned char *)(0xbfc00000+offset)) = 0x30;
}
unsigned int sstIsBusy(unsigned int t)
{
	unsigned int i;
	for(i = 0; i < t; i ++);
}
void sstWriteByte(unsigned int offset, unsigned char dat)
{
	*((volatile unsigned char *)(0xbfc00000+0x5555)) = 0xAA;
	*((volatile unsigned char *)(0xbfc00000+0x2aaa)) = 0x55;
	*((volatile unsigned char *)(0xbfc00000+0x5555)) = 0xA0;
	*((volatile unsigned char *)(0xbfc00000+offset)) = dat;
	sstIsBusy(0x4ffff);
}
#endif

STATUS sysNvRamGet
    (
    char *string,    /* where to copy non-volatile RAM    */
    int strLen,      /* maximum number of bytes to copy   */
    int offset       /* byte offset into non-volatile RAM */
    )
    {
 #if 1
    	unsigned char *nvRamAdr = 0xbfc00000+0x7d000;
	unsigned int i;

	for(i = 0; i < strLen; i ++)
	{
		string[i] = nvRamAdr[i];
	}
	/* printf("minik get = %s\n", string); */
    	return (OK);
#endif
	return ERROR;
	}

STATUS sysNvRamSet
    (
    char *string,     /* string to be copied into non-volatile RAM */
    int strLen,       /* maximum number of bytes to copy           */
    int offset        /* byte offset into non-volatile RAM         */
    )
    {
#if 1
    	unsigned char *nvRamAdr = 0xbfc00000+0x7d000;
	unsigned int i;
	/*
	for(i = 0; i < strLen; i ++)
	{
		nvRamAdr[i] = string[i];
	}
	*/
	/* printf("minik set = %s strLen = %d\n", string, strLen); */
	/* erase sector */
	sstEraseSector(0x7d000);
	/* sstEraseChip(); */
	sstSoftDelay1(0xffffff);
	for(i = 0; i < strLen; i ++)
	{
		sstWriteByte(0x7d000+i, string[i]);
	}
    	return (OK);
#endif
	return ERROR;
	}

#if 1
void testCpci(void)
{
	volatile unsigned int data;
	int i=0;
	while(1)
	{
		data=0;
         	data=*(volatile unsigned int *)0xb5000000;
		taskDelay(60*2);
		i++;
		data = data&0x0000ffff;
		if(data!=0x8008)
		{
			printf("have read i=%d; read error data=%x\n",i,data);
		/*	break;*/
		}
		if(i%10==0&&data==0x8008)
		{
			printf("i am ok! i=%d\n",i);
		}
		
	}
}
#endif

#if 0
/*******************************************************************************
*
* viaIntDisablePIC - disable a PIC interrupt level
*
* This routine disables a specified PIC interrupt level.
*
* RETURNS: OK, always.
*
* SEE ALSO: sysIntEnablePIC()
*
* ARGSUSED0
*/

STATUS viaIntDisablePIC
    (
    int intLevel        /* interrupt level to disable */
    )
    {

    if (intLevel < 8)
	{
	sysOutByte (0x21,
	    sysInByte (0x21) | (1 << intLevel));
	}
    else
	{
	sysOutByte (0xA1,
	    sysInByte (0xA1) | (1 << (intLevel - 8)));
	}

    return (OK);
    }

/*******************************************************************************
*
* viaIntEnablePIC - enable a PIC interrupt level
*
* This routine enables a specified PIC interrupt level.
*
* RETURNS: OK, always.
*
* SEE ALSO: sysIntDisablePIC()
*
* ARGSUSED0
*/

  STATUS viaIntEnablePIC
    (
    int intLevel        /* interrupt level to enable */
    )
    {

    if (intLevel < 8)
	{
	sysOutByte (0x21,
	    sysInByte (0x21) & ~(1 << intLevel));
	}
    else
	{
	sysOutByte (0xA1,
	    sysInByte (0xA1) & ~(1 << (intLevel - 8)));
	}

    return (OK);
    }
/*******************************************************************************
*
* viaIntLock - lock out all PIC interrupts
*
* This routine saves the mask and locks out all PIC interrupts.
* It should be called in the interrupt disable state(IF bit is 0).
*
* SEE ALSO: sysIntUnlock()
*
* ARGSUSED0
*/
VOID viaIntLock (void)

    {

    viaIntMask1 = sysInByte (0x21);
    viaIntMask2 = sysInByte (0xA1);
    sysOutByte (0x21, 0xff);
    sysOutByte (0xA1, 0xff);
    }

/*******************************************************************************
*
* viaIntUnlock - unlock the PIC interrupts
*
* This routine restores the mask and unlocks the PIC interrupts
* It should be called in the interrupt disable state(IF bit is 0).
*
* SEE ALSO: sysIntLock()
*
* ARGSUSED0
*/

VOID viaIntUnlock (void)

    {

    sysOutByte (0x21, viaIntMask1);
    sysOutByte (0xA1, viaIntMask2);
    }
#endif

/*******************************************************************************
*
* sysPciIntConnect - install pciInt as the handler for PCI A, B, C and D lines
*
* RETURNS: 
*
* SEE ALSO: pciInt(), pciIntConnect()
*/
 
void sysPciIntConnect
    (
    void
    )
    {
    	intConnect(INUM_TO_IVEC(IV_PCI_BUS), pciInt, 0);

	return;
    }

/*******************************************************************************
*
* sysIntInitPICLocal - initialize the PIC
*
* This routine initializes the PIC. It is identical to the sysIntInitPIC
* function in i8259Pic.c, except that it initializes ICW4 for Auto EOI mode
* (Master PIC only).
*
*/

LOCAL void sysIntInitPICLocal (void)

    {
#if 0
UINT hi,lo;

	_rdmsr(DIVIL_MSR_REG(PIC_IRQM_PRIM),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_IRQM_LPC),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_YSEL_HIGH),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_YSEL_LOW),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_ZSEL_HIGH),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_ZSEL_LOW),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
#endif
	pci_isa_write_reg(PCI_UART1_INT_REG, 1);	/* IRQ 4 enable, COM1, IR */
	pci_isa_write_reg(PCI_UART2_INT_REG, 1);	/* IRQ 3 enable, COM2 */
	pci_isa_write_reg(PCI_ISA_FIXUP_REG, 1);
	/* pci_isa_write_reg(PCI_KEL_INT_REG, 1); ***!!!! added by yinwx, 2009-03-06 !!!!****/

	pci_ide_write_reg(PCI_IDE_CFG_REG, 0xDEADBEEF);
	pci_acc_write_reg(PCI_ACC_INT_REG, 1);
	pci_ohci_write_reg(PCI_OHCI_INT_REG, 1);
	pci_ehci_write_reg(PCI_EHCI_FLADJ_REG, 0x2000); /* Defination is NULL!!! */
#if 0	
	printstr("after modified\n\r");
    _rdmsr(DIVIL_MSR_REG(PIC_IRQM_PRIM),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_IRQM_LPC),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_YSEL_HIGH),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_YSEL_LOW),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_ZSEL_HIGH),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
	_rdmsr(DIVIL_MSR_REG(PIC_ZSEL_LOW),&hi,&lo);
	printnum(hi); printstr("\n\r"); printnum(lo); printstr("\n\r");
#endif

    /* initialize the PIC (Programmable Interrupt Controller) */
	sysOutByte (PIC_port1 (PIC1_BASE_ADR),0x11);        /* ICW1 */
    sysOutByte (PIC_port2 (PIC1_BASE_ADR),sysVectorIRQ0);	/* ICW2 */
    sysOutByte (PIC_port2 (PIC1_BASE_ADR),0x04);        /* ICW3 */
    /* auto EOI */
    sysOutByte (PIC_port2 (PIC1_BASE_ADR),0x03);        /* ICW4 */
	
    sysOutByte (PIC_port1 (PIC2_BASE_ADR),0x11);        /* ICW1 */
    sysOutByte (PIC_port2 (PIC2_BASE_ADR),sysVectorIRQ0+8); /* ICW2 */
    sysOutByte (PIC_port2 (PIC2_BASE_ADR),0x02);        /* ICW3 */
    /* auto EOI */
    sysOutByte (PIC_port2 (PIC2_BASE_ADR),0x03);        /* ICW4 */
	
    /* disable interrupts */
	sysOutByte (PIC_port1 (PIC1_BASE_ADR),0x6a);
	sysOutByte (PIC_port1 (PIC2_BASE_ADR),0x6a);
	
    sysOutByte (PIC_IMASK (PIC1_BASE_ADR),0xfb);
    sysOutByte (PIC_IMASK (PIC2_BASE_ADR),0xff);
    }


/**************************************************************************
*
* sysSpuriousInt - Handler for spurious interrupts.
*
* sysIntLevelLocal() returns '16' when the 8259 generates an interrupt
* that cannot be identified (i.e., the INTR line is asserted, but the
* Interrupt Request Register doesn't have any unmasked bits asserted).
* This handler is installed to trap this situation and "ignore" it by
* explicitly generating an EOI cycle for both PICs.
*/
int spuriousCount = 0;
int lastSpuriousLevel = -1;

LOCAL void sysSpuriousInt(void)
{
    int level;
    int serviceReg;
    int retry;
    
    spuriousCount++;
  
    /* issue eoi */
    sysOutByte(PIC_port1 (PIC1_BASE_ADR), 0x20); /* INT CNTRL-1, OCW2 */
    sysOutByte(PIC_port1 (PIC2_BASE_ADR), 0x20); /* INT CNTRL-2, OCW2 */

    for (retry=0; retry < 1; retry ++)
	{
        sysOutByte (PIC_port1 (PIC1_BASE_ADR), 0x0b); /* set OCW3 */
        serviceReg = sysInByte (PIC_port1 (PIC1_BASE_ADR)); /* read in-service Reg */
        for (level=0; level < 8; level++)
	    if ((serviceReg & 1) && (level != 2))
	        goto sysSpurIntLevelExit;
	    else
	        serviceReg >>= 1;

        sysOutByte (PIC_port1 (PIC2_BASE_ADR), 0x0b);
        serviceReg = sysInByte (PIC_port1 (PIC2_BASE_ADR));
        for (level=8; level < 16; level++)
	    if (serviceReg & 1)
	        goto sysSpurIntLevelExit;
	    else
	        serviceReg >>= 1;
	}

    sysSpurIntLevelExit:
    if (level != 16)
	{
	lastSpuriousLevel = level;
	}
printstr("Spurious!!!!!!!!!!!!!!!!\r\n");

}

/*end added by wangfq*/

