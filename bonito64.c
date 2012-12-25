/* bonito64.c - device and memory windows setup for bonito64 */

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
01b,21nov02,zmm  Fix comments, some defines, and other cleanups.
01a,15oct02,zmm  Written.
*/

/*
DESCRIPTION
This library provides utility routines for setting up the local
device addresses, PCI memory windows, and DRAM configuration
for the Bonito64 North Bridge.

INCLUDE FILES: config.h gt64240.h
*/

#include    "vxWorks.h" 
#include    "config.h"
/*#include    "gt64240.h"*/ /*commented by wangfq,replaced by the following*/
#include 	"bonito64.h"

#if 0 /*commented by wangfq*/
/******************************************************************************
*
* pciMapIOSpace - Maps PCI0 and PIC1  IO spaces for the master.
*
* This routine sets the address decodes for PCI0 and PCI1 IO spaces
*
* RETURNS: OK, or ERROR
*/
STATUS pciMapIOSpace
    (
    UINT bus,
    UINT pciIoBase,
    UINT pciIoLength
    )
    {
    UINT pci_io_low_decode_address, pci_io_high_decode_address;
    UINT pciIoTop = (UINT)(pciIoBase + pciIoLength);

    if(pciIoLength == 0)
        pciIoTop++;

    if (bus == 0)
        {
        pci_io_low_decode_address  = PCI0_IO_LOW_DECODE_ADDRESS;
        pci_io_high_decode_address = PCI0_IO_HIGH_DECODE_ADDRESS;
        }
        else if (bus ==1)
        {
        pci_io_low_decode_address  = PCI1_IO_LOW_DECODE_ADDRESS;
        pci_io_high_decode_address = PCI1_IO_HIGH_DECODE_ADDRESS;
        }
        else
            return (ERROR);

    pciIoBase = (UINT)(pciIoBase >> 20) & 0xffff;
    pciIoBase = (gtReadInternalReg(pci_io_low_decode_address) & 0xffff0000)
                    | pciIoBase;
    pciIoTop = (UINT)(((pciIoTop-1) &0x0fffffff) >> 20);
    gtWriteInternalReg(pci_io_low_decode_address, pciIoBase);
    gtWriteInternalReg(pci_io_high_decode_address, pciIoTop);
    return (OK);
    }

/******************************************************************************
*
* pciMapMemorySpace - Maps PCI0/1 memory0/memory3 spaces for the master.
*
* This routine sets the address decodes for all four Memory0-3 spaces in
* both PCI0 and PCI1.
*
* RETURNS: N/A
*/
STATUS pciMapMemorySpace
    (
    UINT bus,           /* pci bus number, 0 for PCI0, or 1 for PCI1 */
    UINT spaceNumber,   /* memory space number, 0 for Memory 0 address window, etc */
    UINT pciMemBase,
    UINT pciMemLength
    )
    {
    UINT pci_memory_low_decode_address, pci_memory_high_decode_address;
    UINT pciMemTop = pciMemBase + pciMemLength;

    if (bus == 0)
    {
        switch (spaceNumber)
        {
        case 0:
            pci_memory_low_decode_address  = PCI0_MEMORY0_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI0_MEMORY0_HIGH_DECODE_ADDRESS;
            break;
        case 1:
            pci_memory_low_decode_address  = PCI0_MEMORY1_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI0_MEMORY1_HIGH_DECODE_ADDRESS;
            break;
        case 2:
            pci_memory_low_decode_address  = PCI0_MEMORY2_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI0_MEMORY2_HIGH_DECODE_ADDRESS;
            break;
        case 3:
            pci_memory_low_decode_address  = PCI0_MEMORY3_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI0_MEMORY3_HIGH_DECODE_ADDRESS;
            break;
        default:
            return (ERROR);
        }
    }
    else if (bus == 1)
    {
        switch (spaceNumber)
        {
        case 0:
            pci_memory_low_decode_address  = PCI1_MEMORY0_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI1_MEMORY0_HIGH_DECODE_ADDRESS;
            break;
        case 1:
            pci_memory_low_decode_address  = PCI1_MEMORY1_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI1_MEMORY1_HIGH_DECODE_ADDRESS;
            break;
        case 2:
            pci_memory_low_decode_address  = PCI1_MEMORY2_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI1_MEMORY2_HIGH_DECODE_ADDRESS;
            break;
        case 3:
            pci_memory_low_decode_address  = PCI1_MEMORY3_LOW_DECODE_ADDRESS;
            pci_memory_high_decode_address = PCI1_MEMORY3_HIGH_DECODE_ADDRESS;
            break;
        default:
            return (ERROR);
        }
    }
    else
        return (ERROR);

    if(pciMemLength == 0)
        pciMemTop++;

    pciMemBase = (pciMemBase >> 20) & 0xffff;
    pciMemBase = (gtReadInternalReg(pci_memory_low_decode_address) & 0xffff0000)
                    | pciMemBase;
    pciMemTop  = ((pciMemTop-1) & 0x0fffffff) >> 20;
    gtWriteInternalReg(pci_memory_low_decode_address, pciMemBase);
    gtWriteInternalReg(pci_memory_high_decode_address, pciMemTop);

    return (OK);
    }

/******************************************************************************
*
* pci0MapMemoryBank0 - Maps PCI0 memory bank 0 for the slave.
*
* This routine maps memory bank 0 as viewed from the PCI bus 0
*
* RETURNS N/A
*/
void pci0MapMemoryBank0
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(0,0,0, PCI_CFG_BASE_ADDRESS_0, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI0_SCS0_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(0,0,0,PCI_CFG_BASE_ADDRESS_0, 4, pci0DramBase);
    }

/******************************************************************************
*
* pci0MapMemoryBank1 - Maps PCI0 memory bank 1 for the slave.
*
* This routine maps memory bank 1 as viewed from the PCI bus 0
*
* RETURNS N/A
*/
void pci0MapMemoryBank1
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(0,0,0, PCI_CFG_BASE_ADDRESS_1, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI0_SCS1_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(0,0,0,PCI_CFG_BASE_ADDRESS_1, 4, pci0DramBase);
    }


/******************************************************************************
*
* pci0MapMemoryBank2 - Maps PCI0 memory bank 2 for the slave.
*
* This routine maps memory bank 2 as viewed from the PCI bus 0
*
* RETURNS N/A
*/
void pci0MapMemoryBank2
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(0,0,0, PCI_CFG_BASE_ADDRESS_2, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI0_SCS2_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(0,0,0,PCI_CFG_BASE_ADDRESS_2, 4, pci0DramBase);
    }

/******************************************************************************
*
* pci0MapMemoryBank3 - Maps PCI0 memory bank 3 for the slave.
*
* This routine maps memory bank 3 as viewed from the PCI bus 0
*
* RETURNS N/A
*/
void pci0MapMemoryBank3
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(0,0,0, PCI_CFG_BASE_ADDRESS_3, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI0_SCS3_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(0,0,0,PCI_CFG_BASE_ADDRESS_3, 4, pci0DramBase);
    }


/******************************************************************************
*
* pci1MapMemoryBank0 - Maps PCI1 memory bank 0 for the slave.
*
* This routine maps memory bank 0 as viewed from the PCI bus 1
*
* RETURNS N/A
*/
void pci1MapMemoryBank0
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(1,0,0, PCI_CFG_BASE_ADDRESS_0, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI1_SCS0_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(1,0,0,PCI_CFG_BASE_ADDRESS_0, 4, pci0DramBase);
    }

/******************************************************************************
*
* pci1MapMemoryBank1 - Maps PCI1 memory bank 1 for the slave.
*
* This routine maps memory bank 1 as viewed from the PCI bus 1
*
* RETURNS N/A
*/
void pci1MapMemoryBank1
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(1,0,0, PCI_CFG_BASE_ADDRESS_1, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI1_SCS1_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(1,0,0,PCI_CFG_BASE_ADDRESS_1, 4, pci0DramBase);
    }


/******************************************************************************
*
* pci1MapMemoryBank2 - Maps PCI1 memory bank 2 for the slave.
*
* This routine maps memory bank 2 as viewed from the PCI bus 1
*
* RETURNS N/A
*/
void pci1MapMemoryBank2
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(1,0,0, PCI_CFG_BASE_ADDRESS_2, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI1_SCS2_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(1,0,0,PCI_CFG_BASE_ADDRESS_2, 4, pci0DramBase);
    }

/******************************************************************************
*
* pci1MapMemoryBank3 - Maps PCI1 memory bank 3 for the slave.
*
* This routine maps memory bank 3 as viewed from the PCI bus 1
*
* RETURNS N/A
*/
void pci1MapMemoryBank3
    (
    UINT pci0DramBase,
    UINT pci0DramSize
    )
    {
    UINT32 regVal;

    pci0DramBase = pci0DramBase & 0xfffff000;
    sysPciConfigRead(1,0,0, PCI_CFG_BASE_ADDRESS_3, 4, &regVal);
    pci0DramBase = pci0DramBase | (regVal & 0x00000fff);
    if(pci0DramSize == 0)
        pci0DramSize ++;
    gtWriteInternalReg(PCI1_SCS3_BAR_SIZE, pci0DramSize-1);
    sysPciConfigWrite(1,0,0,PCI_CFG_BASE_ADDRESS_3, 4, pci0DramBase);
    }
#endif /*end commented by wangfq*/


#ifdef INCLUDE_SHOW
#if 0 /*commented by wangfq*/
/******************************************************************************
*
* gtDump - Dump the GT64240A chip internal registers
*
* This routine prints the internal registers of the GT-64240
*
* RETURNS: N/A
*/
void dumpGtRegs()
    {
    int i, val;

    for (i=0; i<0x13c; i+=4)
        { 
        val = gtReadInternalReg(i);
        printf("Reg 0x%x : 0x%x\n", i, val);
        } 

    for (i=0x400; i<0x494; i+=4)
        { 
        val = gtReadInternalReg(i);
        printf("Reg 0x%x : 0x%x\n", i, val);
        } 

    for (i=0x800; i<0x880; i+=4)
        { 
        val = gtReadInternalReg(i);
        printf("Reg 0x%x : 0x%x\n", i, val);
        } 

    for (i=0xc00; i<0xce8; i+=4)
        { 
        val = gtReadInternalReg(i);
        printf("Reg 0x%x : 0x%x\n", i, val);
        } 
    }
#endif /*end commented by wangfq, replaced by the following*/
/******************************************************************************
*
* dumpBonitoRegs - Dump the Bonito64 internal registers
*
* This routine prints the internal registers of the Bonito64
*
* RETURNS: N/A
*/
void dumpBonitoRegs()
{
    UINT i, val;

    for (i=0; i<(BONITO_PCI_CSR_SIZE + BONITO_INTERNAL_REG_SIZE); i+=4)
    { 
        val = bonitoReadInternalReg(i);
        printf("Reg 0x%x : 0x%x\n", i, val);
    }
}
#endif

/*******************************************************************************
*
* bonitoWriteInternalReg - write to a Bonito64 internal register.
*
* This routine writes data to a register internal to the Bonito64 part.
* This routine uses the offset within the Bonito64 to compute the address.
* The written data is not cached.
*
* Inputs:
* UINT int  reg_offset - The register location in the internal address 
*                        space segment.
* UINT reg_data        - The data to be written to register.
*
* RETURN: N/A
*/

void bonitoWriteInternalReg (UINT regOffset, UINT regData)
    { 
    	BONITO(regOffset) = PSWAP(regData);
    }
 
/*******************************************************************************
*
* bonitoReadInternalReg - read a Bonito64 internal register.
*
* This routine reads data to a register internal to the Bonito64 part.
* This routine only accepts the register offset. The read data is not cached.
*
* Inputs:
* UINT int  reg_offset - The register location in the internal address 
*                        space segment.
*
* RETURN: N/A
*/

UINT bonitoReadInternalReg (UINT regOffset)
    {
    UINT regValue;
    regValue = BONITO(regOffset);
    return PSWAP(regValue);
    }


/********************************************************************
*
* writeMemWord - Write 4 bytes to address addr.
*
* The function write 4 bytes to an address, and takes care of Big/Little
* endian conversion.  Note that, as opposed to bonitoWriteInternalReg, this
* routine works on any memory location.
*
* RETURNS: N/A
*/

void writeMemWord(ULONG address, UINT data)
    { 
    *((UINT *)(address | (ULONG)K1BASE)) = PSWAP(data); /*modified by wangfq,from IO_SPACE_VIRT_BASE to K1BASE*/
    }

/********************************************************************
*
* readMemWord - Read 4 bytes from address addr
*
* The function reads 4 bytes and takes care of Big/Little endian conversion.
* Note that, as opposed to bonitoReadInternalReg, this routine works on any memory
* location.
*
* RETURNS:  data read from address addr
*/

UINT readMemWord(ULONG address)
    {
    UINT value;
    value = *(UINT *)((ULONG)K1BASE | address); /*modified by wangfq,from IO_SPACE_VIRT_BASE to K1BASE*/
    return PSWAP(value);
    }

#if 0 /*commented by wangfq*/
/******************************************************************************
*
* sdramAutoDetect - JK Detect size of DRAM
*
* This routine detects the size of onboard DRAM.  It also enables access
* to DRAM from a PCI master by seting up the appropriate translations
* to local memory.
*
* RETURNS: Size of DRAM in bytes.
*
*/
UINT32 sdramAutoDetect()
    {
    UCHAR pldval = *(volatile UCHAR *)(PLD_BASE_ADR + BOARD_STAT);
    UINT bank0Size, bank1Size, bank2Size, bank3Size; 
    ULONG totalSize;
    
    pldval &= BOARD_RAM_MASK;
    switch (pldval)
        {
        case BOARD_RAM_128MB:
            bank2Size = bank0Size = 0x04000000;
            break;
        case BOARD_RAM_256MB:
            bank2Size = bank0Size = 0x08000000;
            break;
        case BOARD_RAM_512MB:
            bank2Size = bank0Size = 0x10000000;	
            break;
        case BOARD_RAM_1GB:
            bank2Size = bank0Size = 0x20000000;
            break;
        default:
            bank2Size = bank0Size = 0;
        }

    bank1Size = bank3Size = 0;

    totalSize = (bank0Size + bank1Size + bank2Size + bank3Size);

    /* Map PCI to DRAM memory here */
    
    pci0MapMemoryBank0(PCI2DRAM_BASE_ADRS, bank0Size);
    pci0MapMemoryBank1(PCI2DRAM_BASE_ADRS + bank0Size, bank1Size); 
    pci0MapMemoryBank2(PCI2DRAM_BASE_ADRS + bank0Size + bank1Size, bank2Size);
    pci0MapMemoryBank3(PCI2DRAM_BASE_ADRS + bank0Size + bank1Size + bank2Size, bank3Size);
    pci1MapMemoryBank0(PCI2DRAM_BASE_ADRS, bank0Size);
    pci1MapMemoryBank1(PCI2DRAM_BASE_ADRS + bank0Size, bank1Size);
    pci1MapMemoryBank2(PCI2DRAM_BASE_ADRS + bank0Size + bank1Size, bank2Size);
    pci1MapMemoryBank3(PCI2DRAM_BASE_ADRS + bank0Size + bank1Size + bank2Size, bank3Size);

    /* Closed unused windows on PCI busses */

    gtWriteInternalReg(PCI0_BASE_ADDR_REGS_ENABLE, 0x1ffffdfa);
    gtWriteInternalReg(PCI1_BASE_ADDR_REGS_ENABLE, 0x1ffffdfa);
    return (totalSize);
    }
#endif /*end commented by wangfq*/
