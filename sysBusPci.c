/* sysBusPci.c - PCI Autoconfig support */

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
01b,02dec02,zmm  Cleanups after code review.
01a,26sep02,zmm  written
*/

/*
DESCRIPTION

This module contains the "non-generic" or "board specific" PCI
initialization code, including initializing the PCI Host bridge,
and initiating PCI auto-config.
*/

/* includes */

#include "vxWorks.h"
#include "logLib.h"
#include "taskLib.h"
#include "config.h"
#include "drv/pci/pciLocalBus.h" 
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciAutoConfigLib.h"
#include "Lsn2eCpciSBC.h"
#include "simpleprintf.h"
/* forward declarations */
LOCAL UCHAR sysPciAutoConfigIntAsgn ( PCI_SYSTEM * pSys, PCI_LOC * pFunc,
    UCHAR intPin );
LOCAL STATUS sysPciAutoConfigInclude ( PCI_SYSTEM *pSys, PCI_LOC *pciLoc,
    UINT devVend );
/* LOCAL void sysHostPciInit(void); */


/* externals */
IMPORT void bonitoWriteInternalReg (UINT regOffset, UINT regData);
IMPORT UINT bonitogtReadInternalReg (UINT regOffset);


/*PCI IRQ ROUTING TABLE ON Lsn2cCpciSBC
device         -- Sigal   --GPIN0-6 of Bonito   Bitno.[from 0]

82559/6254/vga -- P0_INTA --gpio[0]             16
82546(0)/audio -- P0_INTB --gpio[1]             17
82546(1)       -- P0_INTC --gpio[2]             18
null           -- P0_INTD --gpio[3]             19
                  PIRQA   --gpio[4]             20
                  PIRQB   --gpio[5]             21
                  PIRQC   --gpio[6]             22
usb            -- PIRQD   --gpio[7]             23
                PX4_NMI   --gpin[0]             25  
                PX4_INTR  --gpin[1]             26
   
               -- CPUENUM/--gpin[3]             28 
com2           -- irq3    --gpin[4]             29
com1           -- irq4    --gpin[5]             30
*/

/*PCI IRQ ROUTING TABLE ON Lsn2eCpciSBC
device         -- Sigal   --GPIN0-6 of Bonito   Bitno.[from 0]

82559/6254/vga -- P0_INTA --gpio[0]             16
82546(0)/audio -- P0_INTB --gpio[1]             17
82546(1)       -- P0_INTC --gpio[2]             18
null           -- P0_INTD --gpio[3]             19
                  PIRQA   --gpio[4]             20
                  PIRQB   --gpio[5]             21
                  PIRQC   --gpio[6]             22
usb            -- PIRQD   --gpio[7]             23
                PX4_NMI   --gpin[0]             25  
                PX4_INTR  --gpin[1]             26
             NB_UART_INT  --gpin[2]             27   
               -- CPUENUM/--gpin[3]             28 
com2           -- irq3    --gpin[4]             29
com1           -- irq4    --gpin[5]             30

*/

/*82c59 Interrup Controller in 82371eb
 irq0   system clock
 irq1   keyboard
 irq2   PIC cascade
 irq3   com2
 irq4   com1
 irq5   reserved
 irq6   floppy
 irq7   null
 irq8   cmos
 irq9   PIC cascade
 irq10  null
 irq11  LAN interface
 irq12  PS/2 mouse
 irq13  Numeric data processor
 irq14  primary IDE
 irq15  secondary IDE
*/

LOCAL UCHAR pci_irq_table[/*19*/][2/*8*/]=
{
#if 0
	{0,		0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 0,  AD11*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 1,  AD12*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 2,  AD13*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 3,  AD14*/
	{0,	    0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 4,  AD15*/
	{0, 	0, 		0, 	    0, 	    0, 		0, 0,0}, /*pci device 5,  AD16*/
	{0,     0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 6,  AD17*/
	{0,	    0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 7,  AD18*/
	{0,     0, 	    0,	    0, 		0, 		0, 0,0}  /*pci device 8,  AD19*/
	{0,		0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 9,  AD20*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 10, AD21*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 11, AD22*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 12, AD23*/
#endif

#if 1 /* for 3U CPCI, added by yinwx, 08-02-24 */
	/* for bus=0 */
	{27,	0},  /* 0, pci device 7,  AD18 for vga */
	{24,   	25}, /* 1, pci device 8,  AD19 for 82546 GEI */
	{25,   	0},  /* 2, pci device 9,  AD20 for 6254Brige */
	{26,   	0},  /* 3, pci device 10, AD21 for SiI0680A IDE Controller */
	{0,   	0},  /* 4, pci device 11, AD22 for null */
	{0,   	0},  /* 5, pci device 12, AD23 for null */
	{0,   	0},  /* 6, pci device 13, AD24 for null */
	{0,   	0},  /* 7, pci device 14, AD25 for CS5536 SB */
	/* for bus=1, added by yinwx, 2009-05-25 */
	{27,   	0},  /* 8, pci device 15, AD31 for secondary board 1 */
	{24,   	0},  /* 9, pci device 14, AD30 secondary board 2 */
	{25,   	0},  /* 10, pci device 13, AD29 secondary board 3 */
	{26,   	0}   /* 11, pci device 12, AD28 secondary board 4 */
#endif

#if 0 /* for Lsn2f ETX */
	{0,		0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 5, AD11 IDSEL0 for 82371eb*/
	{24,   	0/*,    0, 	    0, 	    0, 		0, 0,0*/}, /*pci device 6, AD4  IDSEL1 for 82559*/
	{26,   	0/*,    0, 	    0, 	    0, 		0, 0,0*/}  /*pci device 7, AD18 IDSEL2 for vga*/
#endif

#if 0
	{26,	0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 13, AD24 IDSEL0 for 82559*/
	{27,   28/*,    0, 	    0, 	    0, 		0, 0,0*/}, /*pci device 14, AD25 IDSEL1 for 82546*/
	{26,    0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 15, AD26 IDSEL2 for 6254Brige*/
	{27,	0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 16, AD27 IDSEL5 for audio*/
	{0,     0/*,    0,	    0, 		0, 		0, 0,0*/}, /*pci device 17, AD28 IDSEL4 for 82371eb*/
    {26,	0/*, 	0, 		0, 		0, 		0, 0,0*/}  /*pci device 18, AD29 IDSEL3 for vga*/
#endif
};
#if 0 /*for Lsn2cCpciSBC*/
LOCAL UCHAR pci_irq_table[6/*19*/][2/*8*/]=
{
#if 0
	{0,		0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 0,  AD11*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 1,  AD12*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 2,  AD13*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 3,  AD14*/
	{0,	    0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 4,  AD15*/
	{0, 	0, 		0, 	    0, 	    0, 		0, 0,0}, /*pci device 5,  AD16*/
	{0,     0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 6,  AD17*/
	{0,	    0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 7,  AD18*/
	{0,     0, 	    0,	    0, 		0, 		0, 0,0}  /*pci device 8,  AD19*/
	{0,		0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 9,  AD20*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 10, AD21*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 11, AD22*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 12, AD23*/
#endif
	{16+16,	0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 13, AD24 IDSEL0 for 82559*/
	{16+17, 16+18/*,0, 	    0, 	    0, 		0, 0,0*/}, /*pci device 14, AD25 IDSEL1 for 82546*/
	{16+16, 0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 15, AD26 IDSEL2 for 6254Brige*/
	{16+17,	0/*, 	0, 		0, 		0, 		0, 0,0*/}, /*pci device 16, AD27 IDSEL5 for audio*/
	{0,     0/*,0,	    0, 		0, 		0, 0,0*/}, /*pci device 17, AD28 IDSEL4 for 82371eb*/
    {16+16,	0/*, 	0, 		0, 		0, 		0, 0,0*/}  /*pci device 18, AD29 IDSEL3 for vga*/
};
#endif
#if 0 /*for lm2e-box*/
/*PCI IRQ ROUTING TABLE ON lm2e-box*/
LOCAL UCHAR pci_irq_table[][8]=
{
	{0,		0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 0, AD11*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 1, AD12*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 2, AD13*/
	{0, 	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 3, AD14*/
	{16+26,	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 4, AD15  for notebook*/
	{0, 	0, 		10, 	11, 	0, 		9, 0,0}, /*pci device 5, AD16  for via686b*/
	{16+26, 0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 6, AD17  for raden 7000 */
	{16+27,	0, 		0, 		0, 		0, 		0, 0,0}, /*pci device 7, AD18  for rtl8139*/
	{16+26, 16+27, 	16+28,	0, 		0, 		0, 0,0}  /*pci device 8, AD19  for usb*/
};
#endif
/* subroutines */

/***************************************************************************
*
* sysInByte - pciConfigLib and PCI mapped I/O support
*
* RETURNS: value input
*
* NOMANUAL
*/

UCHAR sysInByte
    (
    ULONG	address
    )
    {
    UCHAR retval;
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    retval = *(volatile UCHAR *)address;
    return (retval);
    }

/***************************************************************************
*
* sysOutByte - pciConfigLib & PCI mapped I/O support
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysOutByte
    (
    ULONG	address,
    UCHAR	data
    )
    {
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    *(volatile UCHAR *)address = data;
    }

/***************************************************************************
*
* sysInWord - pciConfigLib & pci mapped I/O support
*
* RETURNS: value input
*
* NOMANUAL
*/

USHORT sysInWord
    (
    ULONG  address
    )
    {
    USHORT retval;
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    retval = *(USHORT *)(address);
    retval = PBYTESWAP (retval);
    return (retval);
    }

/***************************************************************************
*
* sysOutWord - pciConfigLib support
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysOutWord
    (
    ULONG	address,        /* I/O address to write the byte to */
    USHORT	data            /* byte to write */
    )
    {
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    *(USHORT *)address = PBYTESWAP ((USHORT)data);
    }

/***************************************************************************
*
* sysInLong - pciConfigLib support
*
* RETURNS: value input
*
* NOMANUAL
*/

ULONG sysInLong
    (
    ULONG       address         /* I/O address to read the byte from */
    )
    {
    ULONG retval;
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    retval = PSWAP (*(ULONG *)(address));
    return (retval);
    }

/***************************************************************************
*
* sysOutLong - pciConfigLib support
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysOutLong
    (
    ULONG       address,        /* I/O address to write the byte to */
    ULONG       data            /* byte to write */
    )
    {    
    /* force addresses <64k into PCI I/O space */
    if (!(address & 0xffff0000))
	address += BONITO_PCIIO_BASE_VA;
    
    *(ULONG *)address = PSWAP (data);
    }

/***************************************************************************
* sysOutLongString - output long string to I/O space
*
* returns: N/A
*
* void sysOutLongString (port, address, count)
*     ULONG port;	/@ I/O port address @/
*     ULONG *address;	/@ address of data written to the port @/
*     int count;	/@ count @/
*
*/
void sysOutLongString(ULONG port, ULONG *address, int count)
    {
    FAST ULONG data;

    /* force addresses <64k into PCI I/O space */
    if (!(port & 0xffff0000))
        port += BONITO_PCIIO_BASE_VA;

    while (count--)
	    {
        data = *(address++);
	    *(ULONG *)port = PSWAP(data);
	    }
    }
 
/***************************************************************************
* sysInLongString - input long string from I/O space
*
* returns: N/A
*
* void sysInLongString (port, address, count)
*     ULONG port;	/@ I/O port address @/
*     ULONG *address;	/@ address of data read from the port @/
*     int count;	/@ count @/
*
*/
void sysInLongString(ULONG port, ULONG *address, int count)
    {
    /* force addresses <64k into PCI I/O space */
    if (!(port & 0xffff0000))
        port += BONITO_PCIIO_BASE_VA;

    while (count--)
	    {
	    *(address++) = PSWAP (*(ULONG *)(port));
	    }
    }


/*****************************************************************************
*
* sysOutWordString - write multiple words to PCI I/O space
*
* This function writes consecutive 2-byte words to a word-sized register
* in PCI I/O space.  The register address is an offset in PCI I/O space.
* It is not necessary to do any translations to the address here, 
* the address can be  used as is.  Any translations of the address to 
* account for PCI aperture address mapping to a different PCI address 
* is handled by DynEneAttach().
*
* RETURNS: N/A
*/
void sysOutWordString
    ( 
    ULONG addr, 		/* PCI I/O address */
    UINT16 * src, 		/* word buffer to write (short words) */
    int length			/* number of short words */
    )
    {
    FAST USHORT data;

    /* force addresses <64k into PCI I/O space */
    if (!(addr & 0xffff0000))
        addr += BONITO_PCIIO_BASE_VA;

    /* Do a series of 16 bit writes until we have written all data. */
    while (length--) 
        {
        data = (USHORT)*src++;
        *(USHORT *)addr = PBYTESWAP (data);
	    }
    }

/*****************************************************************************
*
* sysInWordString - read multiple words from PCI I/O space
*
* This function performs multiple reads of a word-sized (2-byte) register 
* in PCI I/O space and stores the results in the buffer at dest.
* It is not necessary to do any translations to the address here, the
* address can be used as is. 
* Any translations of the address to account for PCI aperture address mapping
* to a different
* PCI address is handled by DynEneAttach().
*
* RETURNS: N/A
*/

void sysInWordString
    ( 
    ULONG addr, 	/* PCI I/O address */
    UINT16 * dest, 	/* word buffer to fill (short words) */
    int   length	/* number of short words */
    )
    {
    /* force addresses <64k into PCI I/O space */
    if (!(addr & 0xffff0000))
        addr += BONITO_PCIIO_BASE_VA;

    while (length--) 
        {
	    *dest++ = PBYTESWAP (*(USHORT *)(addr));
	    }
    }


/***************************************************************************
* sysInWordStringRev - Byte swapping version of sysInWordString()
*
* returns: N/A
*
* NOTE: byte swapping is performed by sysInWord().
*
* void sysInWordStringRev (port, address, count)
*     ULONG port;	/@ I/O port address @/
*     USHORT *address;	/@ address of data read from the port @/
*     int count;	/@ count @/
*
*/
void sysInWordStringRev(ULONG port, USHORT *address, int count)
    {
    /* force addresses <64k into PCI I/O space */
    if (!(port & 0xffff0000))
        port += BONITO_PCIIO_BASE_VA;

    while (count--)
	    {
	    *(address++) = PBYTESWAP (*(USHORT *)(port));
	    }
    }

/******************************************************************************
*
* sysPciAutoConfigInclude - Determine if function is to be autoConfigured
*
* This function is called with PCI bus, device, function, and vendor 
* information.  It returns an indication of whether or not the particular
* function should be included in the automatic configuration process.
* This capability is useful if it is desired that a particular function
* NOT be automatically configured.  Of course, if the device is not
* included in automatic configuration, it will be unusable unless the
* user's code made provisions to configure the function outside of the
* the automatic process.
*
* RETURNS: TRUE if function is to be included in automatic configuration,
* FALSE otherwise.
*/
 
LOCAL STATUS sysPciAutoConfigInclude
    (
    PCI_SYSTEM *pSys,		/* input: AutoConfig system information */
    PCI_LOC *pciLoc,		/* input: PCI address of this function */
    UINT     devVend		/* input: Device/vendor ID number      */
    )
    {
    BOOL retVal = OK;
    
    /* If it's the host bridge then exclude it */

    if ((pciLoc->bus == 0) && (pciLoc->device == 0) && (pciLoc->function == 0))
	return ERROR;


    switch(devVend)
	{

	/* TODO - add any excluded devices by device/vendor ID here */

	default:
	    retVal = OK;
	    break;
	}

    return retVal;
    }

/******************************************************************************
*
* sysPciAutoConfigIntAssign - Assign the "interrupt line" value
*
* RETURNS: "interrupt line" value.
*
*/

LOCAL UCHAR sysPciAutoConfigIntAsgn
    ( 
    PCI_SYSTEM * pSys,		/* input: AutoConfig system information */
    PCI_LOC * pFunc,
    UCHAR intPin 		/* input: interrupt pin number */
    )
    {
    UCHAR irqValue = 0xff;    /* Calculated value */

    if (intPin == 0) 
	return irqValue;
	if(pFunc->bus==0)
	/*added by wangfq*/
		irqValue = IV_VIA686_VEC_BASE + pci_irq_table[pFunc->device-7/*13*/][pFunc->function];
	/*end added by wangfq*/
	
	if(pFunc->bus==1)
	/* added by yinwx, 2009-05-25 */
		irqValue = IV_VIA686_VEC_BASE + pci_irq_table[23-pFunc->device][pFunc->function];

    PCI_AUTO_DEBUG_MSG("intAssign called for device [%d %d %d] IRQ: %d\n",
		pFunc->bus, pFunc->device, pFunc->function,
		irqValue, 0, 0 );

    return (irqValue);
    }

/*******************************************************************************
*
* sysPciAutoConfig - PCI autoConfig support routine
*
* This routine instantiates the PCI_SYSTEM structure needed to configure
* the system. This consists of assigning address ranges to each category
* of PCI system resource: Prefetchable and Non-Prefetchable 32-bit Memory, and
* 16- and 32-bit I/O. Global values for the Cache Line Size and Maximum
* Latency are also specified. Finally, the four supplemental routines for 
* device inclusion/exclusion, interrupt assignment, and pre- and
* post-enumeration bridge initialization are specified. 
*
* RETURNS: N/A
*/

void sysPciAutoConfig (void)
    {
    LOCAL PCI_SYSTEM sysParams;
#if 0
	/*BONITO_PCISTATUS = PCI_CMD_FBTB_ENABLE|PCI_CMD_WC_ENABLE;*/
	BONITO_PCICMD = PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE|PCI_CMD_MASTER_ENABLE;

	 BONITO_PCI_REG(0x40)=0x80000000; /*base0's mask register*/
	 BONITO_PCI_REG(0x44)=0xf0000000; /*base1's mask register*/

	BONITO_PCIMAP = 0x46140; /*0x42040;*/ /*Thus,From CPU Access PCI address space*/	

	/*PCI to local mem mapping:[2G,4G]->BONITO[2G,4G]->DDR[0,2G]*/
	/*Bonito PCI CSR PBR*/
	BONITO_PCIBASE0 = 0x80000000; 
	BONITO_PCI_REG(0x40) = 0x80000000;/*base0's mask register*/

	BONITO_PCIBASE1 = 0x0;               /*added in 2008.10.28 */
	BONITO_PCI_REG(0x50) = 0x8000000c;
	BONITO_PCI_REG(0x54) = 0xffffffff;

	 BONITO_PCIMEMBASECFG = 0x80000000;
#endif
	BONITO_PCICMD = PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE|PCI_CMD_MASTER_ENABLE;
	BONITO_PCILTIMER = 0x000000FF;
	BONITO_PCIBASE0 = 0x80000000; 
	BONITO_PCI_REG(0x40) = 0x80000000;/*base0's mask register*/
	BONITO_PCIBASE1 = 0x0;
	BONITO_PCI_REG(0x44)=0xf0000000; /*base1's mask register*/
	BONITO_PCIBASE2 = 0x00800000;
	BONITO_PCIBASE3 = 0x00;

	BONITO_PCICACHECTRL = 0x8000000c;
	BONITO_PCICACHETAG = 0xFFFFFFFF;
	BONITO_PCIBADADDR = 0xFF80000c;
	BONITO_PCIMSTAT = 0xFFFFFFFF;
	*(volatile unsigned int *)(0xbfe00160) = 0x0E;
	*(volatile unsigned int *)(0xbfe00164) = 0x80000000;
	
	BONITO_PCI_REG(0x50) = 0x8000000c;
	BONITO_PCI_REG(0x54) = 0xffffffff;	

	BONITO_PCIMAP = 0x46140;
	BONITO_PCIMEMBASECFG = 0x80000000;
	/*end added*/
	/* 32-bit PCI I/O Space 
	sysParams.pciIo32 = 0x1fd00000;
	sysParams.pciIo32Size = 0x00100000;*/
	
    sysParams.pciIo16 = PCI_IO_SPACE_BASE + 0x0004000;
    sysParams.pciIo16Size = BONITO_PCIIO_SIZE - 0x0004000;
	
    sysParams.pciMemIo32 = PCI_MEM_SPACE_PCI_BASE + 0x04000000;
    sysParams.pciMemIo32Size = BONITO_PCILO_SIZE - 0x04000000;
	
    sysParams.cacheSize = PCI_CACHE_LINE_SIZE;
    sysParams.maxLatency = PCI_LATENCY_TIMER;

	sysParams.maxBus = 2;
    sysParams.autoIntRouting = FALSE;

    sysParams.includeRtn = sysPciAutoConfigInclude ; 
    sysParams.intAssignRtn = sysPciAutoConfigIntAsgn;
	
    sysParams.pciRollcallRtn = NULL;
    pciAutoConfig(&sysParams);

    return;
    }
#if 0
/******************************************************************************
*
* sysHostPciInit - Host/PCI bridge Initialization
*
* This routine initialize Host/PCI bridge Status and Command reisters.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysHostPciInit ()
{
	BONITO_PCISTATUS = PCI_CMD_FBTB_ENABLE|PCI_CMD_WC_ENABLE;
	BONITO_PCICMD = PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE|PCI_CMD_MASTER_ENABLE;
}
#endif
#if 0
/***************************************************************************
*
* sysPciConfigRead - read data from a device PCI configuration register
*
* Read from a device PCI configuration register.
*
* RETURNS: OK on success, ERROR on failure
*/

STATUS sysPciConfigRead
    (
    int bus,
    int dev,
    int func,
    int reg,
    int size,
    void* data
    )
    {
    UINT32      addr, dat;
	
    int         key = intLock();
	
    if ((reg & (size-1)) || reg < 0 || reg > 0x000000ff || size > 4 || size < 1) 
	return (ERROR);
  
    if (bus == 0) 
    {/* Type 0 configuration on onboard PCI bus */
		if (dev > 20 || func > 7)
	    	return (ERROR);		/* device out of range */
		addr = (1 << (dev+11)) | (func << 8) | (reg);
		BONITO_PCIMAP_CFG = (addr >> 16) & 0x0000fffc;
		
    }
    else 
	{/* Type 1 configuration on offboard PCI bus */
		if (bus > 255 || dev > 31 || func > 7)
	    	return (ERROR);	/* device out of range */
		addr = (dev << 11) | (func << 8) | (reg);

		BONITO_PCIMAP_CFG = 0x10001;
    }

    /* clear aborts */
    BONITO_PCICMD |= CSR_RCV_MST_ABRT | CSR_RCV_TGT_ABRT;

	dat = PCI_IN_LONG(PHYS_TO_K1(BONITO_PCICFG_BASE | (addr & 0xfffc)));

	/* move data to correct position */
    dat = dat >> ((addr & 3) << 3);

	switch(size)
	{
		case 1:
			*(UINT8 *)data  = (UINT8)dat;
			break;
		case 2:
			*(UINT16 *)data = (UINT16)dat;
			break;
		case 4:
			*(UINT32 *)data = (UINT32)dat;
			break;
	}

    intUnlock(key);
	
    return OK;
    }

/***************************************************************************
*
* sysPciConfigWrite - write data to a device PCI configuration register
*
* Write to a device PCI configuration register.
*
* RETURNS: OK on success, ERROR on failure
*/

STATUS sysPciConfigWrite
    (
    int bus,
    int dev,
    int func,
    int reg,
    int size,
    UINT32 data
    )
    {
	    UINT32      addr,  dat;
		
	    int         key = intLock();
    	if ((reg & (size-1)) || reg < 0 || reg > 0x000000ff || size > 4 || size < 1) 
			return (ERROR);
	   	if (bus == 0) 
	   	{/* Type 0 configuration on onboard PCI bus */
			if (dev > 20 || func > 7)
		    	return (ERROR);	/* device out of range */
			addr = (1 << (dev+11)) | (func << 8) | (reg);	
			BONITO_PCIMAP_CFG = (addr >> 16) & 0x0000fffc;
	    }
	    else 
	    {/* Type 1 configuration on offboard PCI bus */
		if (bus > 255 || dev > 31 || func > 7)
		    	return (ERROR);	/* device out of range */
		addr = (dev << 11) | (func << 8) | (reg);
		BONITO_PCIMAP_CFG = 0x10001;
	    }

	    /* clear aborts */
	    BONITO_PCICMD |= CSR_RCV_MST_ABRT | CSR_RCV_TGT_ABRT;

	    dat = PCI_IN_LONG(PHYS_TO_K1(BONITO_PCICFG_BASE | (addr & 0xfffc)));

	    data = data << ((addr & 3) << 3);


		if(size ==1 )
			data = dat & (~(0xff << ((addr & 3) << 3))) | data;
		
		if(size ==2 )
			data = dat & (~(0xffff << ((addr & 3) << 3))) | data;
		
		PCI_OUT_LONG(PHYS_TO_K1(BONITO_PCICFG_BASE | (addr & 0xfffc)), data);
		
	    intUnlock(key);
		
	    return OK;
    }
#else
#define PCI_STATUS_MASTER_ABORT CSR_RCV_MST_ABRT
#define PCI_STATUS_MASTER_TARGET_ABORT CSR_RCV_TGT_ABRT

#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define BONITO_PCICMD_MABORT PCI_STATUS_MASTER_ABORT
#define BONITO_PCICMD_MTABORT PCI_STATUS_MASTER_TARGET_ABORT
#define KSEG1ADDR(x) ((x)|0xa0000000)
#define PCIBIOS_SUCCESSFUL              0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED      0x81
#define PCIBIOS_BAD_VENDOR_ID           0x83
#define PCIBIOS_DEVICE_NOT_FOUND        0x86
#define PCIBIOS_BAD_REGISTER_NUMBER     0x87
#define PCIBIOS_SET_FAILED              0x88
#define PCIBIOS_BUFFER_TOO_SMALL        0x89

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1
	
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;


static  void bflush (void)
{
	/* flush Bonito register writes */
	(void) BONITO_PCICMD;
}

static int
p6032_pcibios_config_access(unsigned char access_type,int bus, int device, int function,
			    unsigned char where, UINT32 *data)
{
	
	
	UINT32 addr, type;
	void *addrp;
	
	int reg = where & ~3;
	

	if (bus == 0) 
	{
	        /* Type 0 configuration on onboard PCI bus */
		if (device > 20 || function > 7) {
			*data = -1;	/* device out of range */
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		addr = (1 << (device+11)) | (function << 8) | reg;
		type = 0;
	} else {
	        /* Type 1 configuration on offboard PCI bus */
	        if (bus > 255 || device > 31 || function > 7) {
			*data = -1;	/* device out of range */
		        return PCIBIOS_DEVICE_NOT_FOUND;
		}
		addr = (bus << 16) | (device << 11) | (function << 8) | reg;
		type = 0x10000;
	}

	/* clear aborts */
	BONITO_PCICMD |= BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT;
	
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	bflush (); /*<----------- */
	addrp = (void *)KSEG1ADDR(BONITO_PCICFG_BASE | (addr & 0xffff));
	if (access_type == PCI_ACCESS_WRITE)
		*(volatile unsigned int *)addrp = cpu_to_le32(*data);
	else
		*data = le32_to_cpu(*(volatile unsigned int *)addrp);
	
/****************==========Here!!==============*************/
#if 0
prom_printf ("pci_config: (%d,%d,%d)/%x 0x%02x %s 0x%x\n", bus, device, function, addr,
	     reg,
	     access_type == PCI_ACCESS_WRITE ? "<-" : "->",
	     *data);
#endif

	if (BONITO_PCICMD & (BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT)) {
	    BONITO_PCICMD |= BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT;
	    *data = -1;
	    return PCIBIOS_DEVICE_NOT_FOUND;
	}

	return PCIBIOS_SUCCESSFUL;
}


static int
p6032_pcibios_read_config_byte(int bus, int device, int function, int where, u8 *val)
{
    UINT32 data;
	int status;
	status = p6032_pcibios_config_access(PCI_ACCESS_READ,  bus,  device,  function, where, &data);
	*val = (data >> ((where & 3) << 3)) & 0xff;

	return status;
}


static int
p6032_pcibios_read_config_word (int bus, int device, int function, int where, u16 *val)
{
    UINT32 data;
	int status;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	status = p6032_pcibios_config_access(PCI_ACCESS_READ,  bus,  device, function, where, &data);
	*val = (data >> ((where & 3) << 3)) & 0xffff;

	return status;
}

static int
p6032_pcibios_read_config_dword (int bus, int device, int function, int where, u32 *val)
{
	UINT32 data;
	int status;

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	
	status = p6032_pcibios_config_access(PCI_ACCESS_READ, bus, device, function, where, &data);
	*val = data;

	return status;
}


static int
p6032_pcibios_write_config_byte (int bus, int device, int function, int where, u8 val)
{
	UINT32 data;
	int status;
	status = p6032_pcibios_config_access(PCI_ACCESS_READ, bus, device, function, where, &data);

	if (status != PCIBIOS_SUCCESSFUL)
		return status;

	data = (data & ~(0xff << ((where & 3) << 3))) |
	       (val << ((where & 3) << 3));

	status = p6032_pcibios_config_access(PCI_ACCESS_WRITE,  bus,  device,  function, where, &data);
	return status;
}

static int
p6032_pcibios_write_config_word (int bus, int device, int function, int where, u16 val)
{
    UINT32 data;
	int status;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
        status = p6032_pcibios_config_access(PCI_ACCESS_READ,  bus,  device,  function, where, &data);

	if (status != PCIBIOS_SUCCESSFUL)
		return status;

	data = (data & ~(0xffff << ((where & 3) << 3))) | 
	       (val << ((where & 3) << 3));

	status = p6032_pcibios_config_access(PCI_ACCESS_WRITE,  bus,  device,  function, where, &data);
	return status;
}

static int
p6032_pcibios_write_config_dword( int bus, int device, int function, int where, u32 val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return p6032_pcibios_config_access(PCI_ACCESS_WRITE,  bus,  device,  function, where, &val);
}

int sysPciConfigRead(int bus, int device, int function, int reg, int width, int *data)
{
	int stat = 0;

    switch(width)
    {
	    case 1:
			stat = p6032_pcibios_read_config_byte( bus,  device,  function,reg,(u8 *)data);return stat;
	    case 2:
			stat = p6032_pcibios_read_config_word( bus,  device,  function,reg,(u16 *)data);return stat;
	    case 4:
			stat = p6032_pcibios_read_config_dword( bus,  device,  function,reg,data);return stat;
    }
    return -1;
}

int sysPciConfigWrite(int bus, int device, int function, int reg, int width, int data)
{
	int stat = 0;

	switch(width)
       {
			case 1:  
				stat = p6032_pcibios_write_config_byte( bus,  device,  function,reg,data);return stat;
			case 2:  
				stat = p6032_pcibios_write_config_word( bus,  device,  function,reg,data);return stat;
			case 4:  
				stat = p6032_pcibios_write_config_dword( bus,  device,  function,reg,data);return stat;
       }
       return -1;
}

#endif
