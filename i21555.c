/* i21555.c - Intel 21555/21555 PCI-to-PCI Non-Transparent Bridge library */

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
01c,20mar02,agf  revalidation code review results
01b,16jul01,agf  add CoE notice
01a,26sep00,bvn  created.
*/

/*
DESCRIPTION
Driver for the Intel 21555/21555 non-transparent PCI-to-PCI bridge.

INCLUDE FILES: i21555.h
*/

/* include files */

#include "vxWorks.h"
#include "logLib.h"
#include "lstLib.h"
#include "memLib.h"
#include "intLib.h"
#include "config.h"
#include "drv/pci/pciConfigLib.h"
#include "i21555.h"

#ifndef MB
#define MB (1048576)
#define KB (1024)
#endif

#define BUS_NO(x) ((x >> 16) & 0xff)
#define DEV_NO(x) ((x >> 11) & 0x1f)
#define FUNC_NO(x) ((x >> 8) & 0x7)

#if FALSE
#undef LOCAL
#define LOCAL 
#endif

IMPORT BOOL sysCPCIEnable;

/* Table Lookup Page Size */
LOCAL UINT tblPageLUT0[] = {
	0, 256, 512, 1*KB, 2*KB, 4*KB, 8*KB, 16*KB, 
	32*KB, 64*KB, 128*KB, 256*KB, 512*KB, 1*MB, 2*MB, 4*MB
};
LOCAL UINT tblPageLUT1[] = {0, 8*MB, 16*MB, 32*MB, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0};

typedef struct pciMemNode {
	struct pciMemList *nextPtr;
	struct pciMemList *prevPtr;
	UINT pciMemBase;
	UINT pciMemSize;
} PCI_MEM_NODE;

typedef struct pciMemLst {
 	LIST pciLst;
        UINT pciMemBase;
        UINT pciMemTotal;
} PCI_MEM_LST;

LOCAL PCI_MEM_LST pciIOList;
LOCAL PCI_MEM_LST pciMemList;
LOCAL PCI_MEM_LST pciPfMemList;

/* Forward declarations */
STATUS	sysI21555PCIConfigWrite (int busNum, int devNum, int funcNum,
                                 int regNum, int dataSize, UINT devValue,
                                 int type);
STATUS	sysI21555PCIConfigRead (int busNum, int devNum, int funcNum, int regNum,
                                int dataSize, void *devValue, int type);
STATUS	sysI21555BusToLocalAdrs (UINT space, UINT busAdr, UINT *localAdr);
STATUS	sysI21555LocalToBusAdrs (UINT space, UINT localAdr, UINT *busAdr);

/* Locals */
LOCAL	I21555_CSR *i21555GetCsr ();
LOCAL	UINT	i21555Orientation ();
LOCAL	int	interfaceOrientation = -1;
LOCAL	UINT32	pciConfigAddr = -1;

LOCAL	BOOL	sysMailboxConnected = FALSE;
LOCAL	FUNCPTR	sysMailboxRoutine[MAILBOXSIZE-1];
LOCAL	UINT	sysMailboxArg[MAILBOXSIZE-1];

#ifdef INCLUDE_PCI_SHOW
LOCAL	void	printI21555Master (UINT CPCIAdr, UINT PCIAdr, UINT size,
                                   BOOL printHdr);
LOCAL	void	printI21555Slave (UINT CPCIAdr, UINT PCIAdr, UINT size,
                                  BOOL printHdr);
#endif

LOCAL	void	i21555MemFree (UINT pciSpace, UINT pciAddr, UINT pciSize);
LOCAL	UINT	i21555MemAlloc (UINT pciSpace, UINT pciSize);
LOCAL	BOOL	isI21555AllocatedMem (UINT base, UINT size);
LOCAL	BOOL	isNot2N (UINT size);
LOCAL	void	i21555MemInit (void);
LOCAL	STATUS	i21555TblSetup (UINT *addrList, UINT *ctrlList, UINT listCnt,
                                UINT winSize);
LOCAL	STATUS	i21555TblGetSetup (UINT *addrList, UINT *ctrlList, UINT listCnt,
                                   UINT *winSize);
LOCAL	void	i21555SetupBAR (UINT regAdr, UINT mask, UINT sizeVal,
                                UINT ctrlFlag);
LOCAL	UINT	i21555SetupBARSize (UINT setupBar);
LOCAL	void	i21555EnableSetupBAR (UINT regAdr, UINT mask);
LOCAL	STATUS	i21555TranslatePrimary (UINT space, UINT localAdr,
                                        UINT *busAdr, UINT cfgHdrOffset,
                                        UINT localOffsetMem,
                                        UINT localOffsetPfMem,
                                        UINT localOffsetIO, UINT busOffsetMem,
                                        UINT busOffsetIO);
LOCAL	STATUS	i21555TranslateSecondary (UINT space, UINT busAdr,
                                          UINT *localAdr, UINT cfgHdrOffset,
                                          UINT localOffsetMem,
                                          UINT localOffsetPfMem,
                                          UINT localOffsetIO,
                                          UINT busOffsetMem, UINT busOffsetIO);
LOCAL	void	i21555VerifyLocalPrimary (UINT space, UINT busAdr,
                                          UINT *localAdr);
LOCAL	void	i21555VerifyLocalSecondary (UINT space, UINT busAdr,
                                            UINT *localAdr);
LOCAL	UINT	i21555ReadConfig (UINT regAdr);
LOCAL	STATUS	i21555WriteConfig (UINT regAdr, UINT val);

/****************************************************************************
* sysCPCIBusToLocalAdrs - Translate a CPCI bus to local address
*
* This routine translates a CompactPCI bus address to a local CPU
* address. <space> is the bus space identifier and should be one
* of the following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: ERROR if the translation cannot be performed
*
* SEE ALSO: sysCPCILocalToBusAdrs()
*/
STATUS sysCPCIBusToLocalAdrs
    (
    int space,
    char *busAdr,
    char **localAdr
    )
    {
    return (sysI21555BusToLocalAdrs((UINT)space,
                                    (ULONG)busAdr, (UINT *)localAdr));
    }

/****************************************************************************
* sysCPCILocalToBusAdrs - Translate a local address to a CPCI bus address
*
* This routine translates a DRAM or PCI local address to a CompactPCI bus 
* address.  <space> is the bus space identifier and should be one of the 
* following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: ERROR if the translation cannot be performed
*
* SEE ALSO: sysCPCIBusToLocalAdrs()
*/

STATUS sysCPCILocalToBusAdrs 
    (
    int space,
    char *localAdr,
    char **busAdr
    )
    {
    return (sysI21555LocalToBusAdrs((UINT)space, (ULONG)localAdr,
                                    (UINT *)busAdr));
    }

/****************************************************************************
* sysPrimaryMailboxHandler - Handle the mailbox 0/1 interrupt
*
* RETURNS: OK
*
* NOMANUAL:
*/

LOCAL void sysPrimaryMailboxHandler
    (
    )
    {
    USHORT intStat, vector;
    volatile USHORT *intReg;

    intReg = (USHORT *)((ULONG)&i21555GetCsr()->clrIRQ);

    /* Get the interrupt conditions */
    intStat = *intReg;
    vector = PBYTESWAP(intStat) & MAILBOX_INT_MASK;

    /* This should never happen */
    if (vector == 0)
        {
        logMsg("Spurious Mailbox 0/1 interrupt\n", 0, 0, 0, 0, 0, 0);
        return;
        }

    vector -= 1;
     
    /* call customer's service routine, if present */
    if (sysMailboxRoutine[vector] == NULL)
        logMsg("Unsolicited Mailbox %d interrupt\n", vector+1, 0, 0, 0, 0, 0);
    else
        (*sysMailboxRoutine[vector])(sysMailboxArg[vector]);

    /* Clear the interrupt conditions */
    *intReg = intStat;
    intStat = *intReg;
    }

/****************************************************************************
* sysSecondaryMailboxHandler - Handle the mailbox 0/1 interrupt
*
* RETURNS: OK
*
* NOMANUAL:
*/

LOCAL void sysSecondaryMailboxHandler
    (
    )
    {
    USHORT intStat, vector;
    volatile USHORT *intReg;

    intReg = (USHORT *)((ULONG)&i21555GetCsr()->clrIRQ + 2);

    /* Get the interrupt conditions */
    intStat = *intReg;
    vector = PBYTESWAP(intStat) & MAILBOX_INT_MASK; 

    /* This should never happen */
    if (vector == 0)
        {
        logMsg("Spurious Mailbox 0/1 interrupt\n", 0, 0, 0, 0, 0, 0);
        return;
        }

    vector -= 1;

    /* call customer's service routine, if present */
    if (sysMailboxRoutine[vector] == NULL)
        logMsg("Unsolicited Mailbox %d interrupt\n", vector+1, 0, 0, 0, 0, 0);
    else
        (*sysMailboxRoutine[vector])(sysMailboxArg[vector]);

    /* Clear the interrupt conditions */
    *intReg = intStat;
    }

/****************************************************************************
* sysMCMailboxConnect - Connect a routine to the mailbox
*
* Connect a routine to the mailbox interrupt condition.  The <mailbox> is
* between 1 and 255
*
* RETURNS: OK or ERROR if <mailbox> is out of range or mailbox handling
*          facilities could not be initialized.
*
*/

STATUS sysMCMailboxConnect
    (
    FUNCPTR routine,
    int mailbox,
    int arg
    )
    {
    if ((mailbox >= MAILBOXSIZE) || (mailbox == 0))
        return (ERROR);

    mailbox -=1;

    if (sysMailboxConnected == FALSE)
        {
        if (interfaceOrientation == IS_PRIMARY_INTERFACE)
            {
            if (intConnect(INUM_TO_IVEC(IV_21555_VEC), sysPrimaryMailboxHandler,
                                    0) == ERROR)
                return (ERROR);
            }
        else
            {
            if (intConnect(INUM_TO_IVEC(IV_21555_VEC),
                                    (VOIDFUNCPTR)sysSecondaryMailboxHandler,
                                    0) == ERROR)
                return (ERROR);
            }

        sysMailboxConnected = TRUE;
        }

    sysMailboxRoutine[mailbox] = routine;
    sysMailboxArg[mailbox] = arg;
  
    return (OK);
    }

/****************************************************************************
* sysI21555MailboxConnect - Connect a routine to the mailbox
*
* Connect a routine to the mailbox interrupt condition.  The <mailbox> is
* between 1 and 255
*
* RETURNS: OK or ERROR if <mailbox> is out of range or mailbox handling
*          facilities could not be initialized.
*
*/

STATUS sysI21555MailboxConnect
    (
    FUNCPTR routine,
    int mailbox,
    int arg
    )
    {
    return (sysMCMailboxConnect(routine,mailbox,arg));
    }

/****************************************************************************
* sysI21555MailboxEnable - Enable a mailbox
*
* Enables the mailbox corresponding to the mailbox address given by <addr>.
* The lower 8 bits of <addr> should be one of the following:
*
* 0x9C - Primary Interface Mailbox  
* 0x9E - Secondary Interface Mailbox 
*
* RETURNS: OK or ERROR if <addr> is not valid
*/

STATUS sysI21555MailboxEnable
    (
    char *addr
    )
    {
    volatile USHORT *mboxAddr;
    UINT mbox = ((ULONG)addr & 0xFF);  /* IRQ Clear Mask Registers */

    if ((mbox != 0x9C) && (mbox != 0x9E))
        return (ERROR);

    mbox -= 0x9C;
    mboxAddr = (USHORT *)((ULONG)&i21555GetCsr()->clrIRQMask + mbox);

    *mboxAddr = PBYTESWAP(MAILBOX_INT_MASK);

    return (OK);
    }

/****************************************************************************
* sysI21555MailboxDisable - Disable a mailbox
*
* Disable the mailbox corresponding to the mailbox address given by <addr>.
* The lower 8 bits of <addr> should be one of the following:
*
* 0x9C - Primary Interface Mailbox  
* 0x9E - Secondary Interface Mailbox 
*
* RETURNS: OK or ERROR if <addr> is not valid
*
*/

STATUS sysI21555MailboxDisable
    (
    char *addr
    )
    {
    UINT mbox = ((ULONG)addr & 0xFF);  /* IRQ Set Mask Registers */

    if ((mbox != 0x9C) && (mbox != 0x9E))
        return (ERROR);

    mbox -= 0x9C;
    *(volatile USHORT *)((ULONG)&i21555GetCsr()->setIRQMask + mbox) = PBYTESWAP(MAILBOX_INT_MASK);

    return (OK);
    }

/***************************************************************************
* sysI21555MailboxAck - Clear a mailbox interrupt
*
* Clears the mailbox condition by writing the IRQ clear register.  <mbox> is
* ignored:
*
* RETURNS: OK or ERROR if mbox >= MAILBOXSIZE 
*
*/

STATUS sysI21555MailboxAck
    (
    UINT mbox
    )
    {
    volatile USHORT *mboxAddr;

    if (interfaceOrientation == IS_PRIMARY_INTERFACE)
        mboxAddr = (USHORT *)((ULONG)((I21555_CSR *)(i21555GetCsr()))->clrIRQ);
    else
        mboxAddr = (USHORT *)((ULONG)((I21555_CSR *)(i21555GetCsr()))->clrIRQ + 2);

    *mboxAddr = PBYTESWAP(MAILBOX_INT_MASK);

    return (OK);
    }

/***************************************************************************
* sysI21555DisablePrimary - Disables the primary interface
*
* This routine enables Configuration access to the primary interface of
* the bridge chip.  It is only relevant when the secondary interface is
* on the local bus.  If this is not true then there is no affect.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555EnablePrimary()
*/

void sysI21555DisablePrimary (void)
    {
    UINT chipControl;

    if (interfaceOrientation == IS_PRIMARY_INTERFACE)
        return;

    chipControl = i21555ReadConfig(CHIP_CONTROL);

    /* Toggle to Primary Access Lockout bit */
    chipControl |= PRIMARY_ACCESS_LOCKOUT;

    /* Write the register */
    i21555WriteConfig(CHIP_CONTROL, chipControl);

    return;
    }

/***************************************************************************
* sysI21555EnablePrimary - Enables the primary interface
*
* This routine enables Configuration access to the primary interface of
* the bridge chip.  It is only relevant when the secondary interface is
* on the local bus.  If this is not true then there is no affect.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555DisablePrimary()
*/

void sysI21555EnablePrimary (void)
    {
    UINT chipControl;

    if (interfaceOrientation == IS_PRIMARY_INTERFACE)
        return;

    chipControl = i21555ReadConfig(CHIP_CONTROL);

    /* Toggle to Primary Access Lockout bit */
    chipControl &= ~PRIMARY_ACCESS_LOCKOUT;

    /* Write the register */
    i21555WriteConfig(CHIP_CONTROL, chipControl);

    return ;
    }

/****************************************************************************
* sysI21555BusToLocalAdrs - Translate a CPCI bus to local address
*
* This routine translates a CompactPCI bus address to a local CPU
* address. <space> is the bus space identifier and should be one
* of the following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: ERROR if the translation cannot be performed
*
* SEE ALSO: sysI21555LocalToBusAdrs()
*/

STATUS sysI21555BusToLocalAdrs 
    (
    UINT space,
    UINT busAdr,
    UINT *localAdr
    )
    {
    STATUS status = ERROR;
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        status = i21555TranslateSecondary(space, busAdr, localAdr, 0,
                                 PCI_MEM_TO_CPU(pciMemList.pciMemBase),
                                 PCI_MEM_TO_CPU(pciPfMemList.pciMemBase), 
                                 PCI_IO_TO_CPU(pciIOList.pciMemBase), 0 ,0);

        if (status == OK)
            /* Make sure we're not accessing our own translated address */
            i21555VerifyLocalPrimary(space,busAdr,localAdr);
        }
    else
        {
        status = i21555TranslatePrimary(space, busAdr, localAdr, 0, 0, 0,
                                PCI_MEM_TO_CPU(pciMemList.pciMemBase), 
                                PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase), 
                                PCI_IO_TO_CPU(pciIOList.pciMemBase));

        if (status == OK)
            /* Make sure we're not accessing our own translated address */
            i21555VerifyLocalSecondary(space,busAdr,localAdr);
        }

    return (status);
    }

/****************************************************************************
* sysI21555LocalToBusAdrs - Translate a local address to a CPCI bus address
*
* This routine translates a DRAM or PCI local address to a CompactPCI bus 
* address.  <space> is the bus space identifier and should be one of the 
* following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: ERROR if the translation cannot be performed
*
* SEE ALSO: sysI21555BusToLocalAdrs()
*/

STATUS sysI21555LocalToBusAdrs 
    (
    UINT space,
    UINT localAdr,
    UINT *busAdr
    )
    {
    STATUS status;

    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        status = i21555TranslatePrimary(space, localAdr, busAdr, 
                                SUB_HEADER_OFFSET, PCI2DRAM_BASE_ADRS,
                                PCI2DRAM_BASE_ADRS, pciIOList.pciMemBase, 0, 0);
    else
        status = i21555TranslateSecondary(space, localAdr, busAdr, 
                                SUB_HEADER_OFFSET, 0, 0, 0,
                                PCI2DRAM_BASE_ADRS, 0);

    return (status);
    }

/****************************************************************************
* sysI21555Init - I21555 Bridge chip initialization
*
* Performs PCI specific initialization for the 21555 chip.  
*
* RETURNS: OK or ERROR
*
* NOMANUAL:
*/

STATUS sysI21555Init 
    (
    UINT32 pciConfAdr
    )
    {
    int i;

    /* Store the PCI Config Address location */
    pciConfigAddr = pciConfAdr;

    /* Get the orientation of the bridge chip */
    interfaceOrientation = i21555Orientation();

    /* Clear the mailbox interrupt structure */
    for (i=0; i < MAILBOXSIZE; i++)
        {
	sysMailboxRoutine[i] = (FUNCPTR)0;
	sysMailboxArg[i] = 0;
        }

    /* For now disable mailbox 1 interrupts */
    i = PSWAP(0xFF00FFFF);  /* Disable all primary interrupts */
    *(ULONG *)(&i21555GetCsr()->setIRQMask) = i;

    /* Enable access to primary interface */
    sysI21555EnablePrimary();

    /* Enable master access on the Compact PCI interface */
    i = i21555ReadConfig(SUB_HEADER_OFFSET + PCI_CFG_COMMAND) | PCI_CMD_MASTER_ENABLE;

    /* If we're the system slot then also allow other Compact PCI */
    /* devices to access to our board */
    if (*(volatile UCHAR *)PLD_REG(CPCI_ID) & CPCI_SSLOT)
        i |= PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE;

    /* Write to the Compact PCI interface register */
    i21555WriteConfig(SUB_HEADER_OFFSET + PCI_CFG_COMMAND, i);

    return (OK);
    }
 
/***************************************************************************
* sysI21555Init2 - I21555 Bridge chip initialization, second part
*
* Performs PCI specific initialization for the 21555 chip.  
*
* RETURNS: OK or ERROR
*
* NOMANUAL:
*/

void sysI21555Init2 (void)
    {
    /* Initialize the PCI memory allocation structures */
    i21555MemInit();
    }

/**************************************************************************
* sysI21555PCIConfigWrite - write a value to a Compact PCI device.  
*
* This function writes a value to the PCI device base 
* in PCI Configuration Space. The input values should be bus number to 
* access <busNum>, device number on the bus <devNum>, function for the 
* device <funcNum>, register of the device <regNum>. The address written 
* written should be 1, 2, or 4 bytes, as specified by <dataSize>, and 
* <devValue> is the address of the register value to be written.  
*
* For Type 0 Configuration cycles <busNum> should be 0 and <devNum>
* should be set to the <IDSEL> value (1 to 0x100000).
*
* For example, to write 32 bits to bus number 0, device 13, function 0, 
* register 0x3C, and performing a Type 0 cycle, use the following command: 
*
* Example : sysI21555PCIConfigWrite 0, 13, 0, 0x3c, 4, &val, 0
*
* Note that the variable "val" should be initialized to data before the
* call to the function. Note also that this function is used only for the 
* target (CompactPCI) bus. 
*
* RETURNS: OK or ERROR if the input values are not within the range or 
*          the address can't be probed.
*
* SEE ALSO: sysI21555PCIConfigRead()
*/

STATUS sysI21555PCIConfigWrite
    (
    int	busNum,		/* bus number to access (0 - 255) */
    int	devNum,		/* device number on the bus (0 - 31) */
    int	funcNum,	/* function for the device (0-7) */
    int	regNum,		/* register of the device (0 - 0xFF) */
    int	dataSize,	/* size of each data (1, 2, or 4) */
    UINT devValue,	/* value to write to the PCI device */ 
    int	type		/* Type of config, 0 or 1 */
    ) 
    {
    UINT data, shft, mask;
    UINT ownbit;
    int cnfgAddr, cnfgData;

    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        cnfgAddr = (int)U_CONF_ADDR;
        cnfgData = (int)U_CONF_DATA;
        }
    else
        {
        cnfgAddr = (int)D_CONF_ADDR;
        cnfgData = (int)D_CONF_DATA;
        }

    /* make sure all input values is within the range */
    if (busNum < 0 || busNum >= 256)
        return (ERROR); 
    if (devNum < 0 || devNum >= (1 << 21))
        return (ERROR);
    if (funcNum < 0 || funcNum >= 8)
        return (ERROR);
    if (regNum < 0 || regNum >= 0x100)
        return (ERROR);
     
    shft = (regNum & 0x3)*8;
    switch (dataSize)
        {
        case 1: 
            mask = 0xff;
            break;
        case 2: 
            mask = 0xffff; 
            break;
        case 4: 
            mask = 0xffffffff; 
            break;
        default:
            return (ERROR);
        }

    /* Enable configuration transactions */
    if ((ownbit = i21555ReadConfig(CONFIG_OWN)) == ERROR)
        return (ERROR);

    ownbit |= 0x02020000;
    if (i21555WriteConfig(CONFIG_OWN, ownbit) == ERROR)
        return (ERROR);

    data = 0x00000000 | (busNum << 16) | (devNum << 11) | (funcNum << 8) | (regNum & 0xFC) | type;

    /* write to CFG_ADDR register */
    if (i21555WriteConfig(cnfgAddr, data) == ERROR)
        return (ERROR);

    /* read CFG_DATA */
    if ((data = i21555ReadConfig(cnfgData)) == ERROR)
        return (ERROR);

    /* Shift data to correct position */
    data = data & ~(mask << shft);
    data |= ((devValue & mask) << shft);

    /* write to CFG_DATA */
    if (i21555WriteConfig(cnfgData, data) == ERROR)
        return (ERROR);

    return (OK);
    }

/**************************************************************************
*
* sysI21555PCIConfigRead - read a value from the PCI device.  
*
* This function reads a value from the PCI device base 
* in PCI Configuration Space . The input values should be bus number to 
* access <busNum>, device number on the bus <devNum>, function for the 
* device <funcNum>, register of the device <regNum>. The address is read 
* should be 1, 2, or 4 bytes, as specified by <dataSize>.  <devValue> is the 
* address where the register value is to be returned.  
*
* For Type 0 Configuration cycles <busNum> should be 0 and <devNum>
* should be set to the <IDSEL> value (1 to 0x100000).
*
* For example, to read 32 bits from bus number 0, device 13, function 0, 
* register 0x3C, and performing a Type 0 cycle, use the following command: 
*
* Example : sysI21555PCIConfigRead 0, 13, 0, 0x3c, 4, &val, 0
*
* Note that the variable "val" should be initialized to 0 (zero) before the
* call to the function. Note also that this function is used only for the 
* target (CompactPCI) bus. 
*
* RETURNS: OK or ERROR if the input values are not within the range or 
*          the address can't be probed.
*
* SEE ALSO: sysI21555PCIConfigWrite()
*/

STATUS sysI21555PCIConfigRead
    (
    int	busNum,		/* bus number to access (0 - 255) */
    int	devNum,		/* device number on the bus (0 - 31) */
    int	funcNum,	/* function for the device (0-7) */
    int	regNum,		/* register of the device (0 - 0xFF) */
    int	dataSize,	/* size of each data (1, 2, or 4) */
    void* devValue,	/* where to return value of device or vendor ID */	
    int	type		/* Type of config, 0 or 1 */
    ) 
    {
    UINT data, mask, shft;
    UINT ownbit;
    int cnfgAddr, cnfgData;

    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        cnfgAddr = (int)U_CONF_ADDR;
        cnfgData = (int)U_CONF_DATA;
        }
    else
        {
        cnfgAddr = (int)D_CONF_ADDR;
        cnfgData = (int)D_CONF_DATA;
        }

    /* make sure all input values is within the range */
    if (busNum < 0 || busNum >= 256)
        return (ERROR); 
    if (devNum < 0 || devNum >= (1 << 21))
        return (ERROR);
    if (funcNum < 0 || funcNum >= 8)
        return (ERROR);
    if (regNum < 0 || regNum > 0xFF)
        return (ERROR);

    shft = (regNum & 0x3)*8;
    switch (dataSize)
        {
        case 1: 
            shft =0;
            mask = 0xff;
            break;
        case 2: 
            mask = 0xffff; 
            break;
        case 4: 
            mask = 0xffffffff; 
            break;
        default:
            return (ERROR);
        }

    /* Enable configuration transactions */
    if ((ownbit = i21555ReadConfig(CONFIG_OWN)) == ERROR)
        return (ERROR);

    ownbit |= 0x02020000;
    if (i21555WriteConfig(CONFIG_OWN, ownbit) == ERROR)
        return (ERROR);

    data = 0x00000000 | (busNum << 16) | (devNum << 11) | (funcNum << 8) | (regNum & 0xFC) | type;

    /* write to CFG_ADDR */
    if (i21555WriteConfig(cnfgAddr, data) == ERROR)
         return (ERROR);
     
    /* read to CFG_DATA */
   data = 0;
    if ((data = i21555ReadConfig(cnfgData)) == ERROR)
         return (ERROR);
     
    data = (data >> shft) & mask;
    switch (dataSize)
        {
        case 1: 
            *(UCHAR *)devValue = (UCHAR)data; 
            break;
        case 2: 
            *(USHORT *)devValue = (USHORT)data; 
            break;
        case 4: 
            *(UINT32 *)devValue = (UINT32)data; 
            break;
        default:
            return (ERROR);
        }

    return (OK);
    }

/****************************************************************************
* sysI21555PCIConfigProbe - probe the PCI bus for a device
*
* This function probes the PCI Configuration Space bus to locate the device
* passed in <devVenID>.  If found the address information for the device is
* passed back in the <busNum>, <devNum>. and <funcNum> fields.  <unit>
* is an input to indicate the instance of the device to locate, with
* 0 being the first device.
*
* If <busNum> is set to between 0 and 256 then the probe will begin at that
* PCI bus number else at 0.  If <devNum> is set between 0 and 31 and the bus
* number is greater than 0 then that is the first device slot probed at 
* the <busNum> else 0.  If <busNum> is zero then <devNum> can be between
* 0 and 0x100000, to support Type 0 Configurations.  Multi-function devices 
* are always probed starting at function 0.
*
* For example, if the device with Vendor ID of 0x1011 and Device ID 
* of 0x0046 is to be probed starting at bus number 5 and device 8 and
* the second instance of the device is needed, the following call can
* be used:
* 
* UINT busNo = 5, devNo = 8, funcNo;
* sysI21555PCIConfigProbe (0x00461011, &busNo, &devNo, &funcNo);
*
* RETURNS: OK or ERROR if probing failed to find the device
*
* SEE ALSO: sysI21555PCIConfigRead(), sysI21555PCIConfigWrite(), sysI21555PCIShow()
*
*/

STATUS sysI21555PCIConfigProbe
    (
    UINT devVenID,
    UINT *busID,
    UINT *devID,
    UINT *funcID,
    UINT unit
    )
    {
    UINT pciDevVen;
    UINT devNum=0, busNum=0, funcNum=0;
    UCHAR pciHdr;
    STATUS status;

    if (*busID >= 256)
        *busID = 1;
    if ((busID != 0) && (*devID > 31))
        *devID = 0;
    if (*funcID > 7)
        *funcID = 0;

    if (*busID == 0)
        {
        /* Do Type 0 Configuration probe */
        for (devNum= (*devID ? *devID : 1); devNum < (1 << 21); devNum <<= 1)
            {
            for (funcNum=*funcID; funcNum<8; funcNum++)
                {
                status = sysI21555PCIConfigRead (0, devNum, funcNum, PCI_CFG_VENDOR_ID, 4, &pciDevVen, 0);
                if ((status == OK) && (pciDevVen == devVenID))
                    {
                    if (unit == 0)
	                {
	                *busID = 0;
	                *devID = devNum;
	                *funcID = funcNum;
            	        return (OK);
	                }
                    /* Next unit instance */ 
                    unit--;

                    /* Check to see if this is a multi-function device */
                    status = sysI21555PCIConfigRead (0, devNum, funcNum, PCI_CFG_HEADER_TYPE, 1, &pciHdr, 0);
                    if (!(pciHdr & PCI_HEADER_MULTI_FUNC))
                        break;
                    }
                }
            }
        }
      
    /* Do Type 1 Configuration probe */
    for (busNum=*busID; busNum < 256; busNum++)
        for (devNum=*devID; devNum<32; devNum++)
            for (funcNum=*funcID; funcNum<8; funcNum++)
                {
                status = sysI21555PCIConfigRead (busNum, devNum, funcNum, PCI_CFG_VENDOR_ID, 4, &pciDevVen, 1);
                if ((status == OK) && (pciDevVen == devVenID))
                    {
                    printf("found device bus 0x%x, device %d, function %d\n",
                           busNum, devNum, funcNum);
                    if (unit == 0)
	                {
	                *busID = busNum;
	                *devID = devNum;
	                *funcID = funcNum;
            	        return (OK);
	                }
                    /* Next unit instance */ 
                    unit--;

                    /* Check to see if this is a multi-function device */
                    status = sysI21555PCIConfigRead (busNum, devNum, funcNum, PCI_CFG_HEADER_TYPE, 1, &pciHdr, 1);
                    if (!(pciHdr & PCI_HEADER_MULTI_FUNC))
                        break;
                    }
                }

    return (ERROR);
    }

/****************************************************************************
*
* sysI21555MasterPSet - Sets up the PCI to CPCI translations 
*
* This routine sets a window to access the Compace PCI bus.  It is assumed
* the the Primary interface of the I21555 is on the local PCI bus.  This
* routine should only be used in this case.  This is typically called by
* sysI21555MasterSet().
*
* Since we're accessing from the Primary interface, we can only set the
* base and translated addresses.  The size is set by the Secondary side.
* If the Secondary has not set the size, BRIDGE_WIN_NOT_ENABLED is returned.
* The <size> field is ignored in this function.
*
* <window> should be I21555_IO_WINDOW, I21555_MEM_WINDOW1, I21555_MEM_WINDOW2,
*           or I21555_MEM_WINDOW3.
* <cpciAddress> is the Compact PCI base address to access.
* <size> the size of the window to set.  It's not used in this case.  See below.
* <prefetchable> is a flag to indicate enabling of pre-fetchable space.  This
*           is only applicable to I21555_MEM_WINDOW1, I21555_MEM_WINDOW2, or 
*           I21555_MEM_WINDOW3.
* <cpuAddress> is the returned CPU translated address.
*
* RETURNS: OK - Success
*          BRIDGE_WIN_SIZE_TOO_LARGE - <size> is too large to set up.
*          BRIDGE_WIN_NOT_ENABLED - window is not enabled from Secondary side.
*          BRIDGE_INVALID_INTERFACE - Primary interface is not on local bus.
*                  Use sysI21555MsterSSet() instead 
*
* Note that I21555_MEM_WINDOW1 maps the CSR registers into the first
* 4KB of the memory window.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555MasterSSet(), sysI21555MasterSet() 
*
*/ 
STATUS sysI21555MasterPSet
    (
    UINT window,
    UINT cpciAddress,
    UINT size,
    BOOL prefetchable,
    UINT *cpuAddress
    )
    {
    UINT regSetup, regTran, regBar, pciAddr;
    
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        printf("sysI21555MasterPSet: Secondary interface is not on local PCI bus.  Use sysI21555MasterPSet() instead\n");
        return (BRIDGE_INVALID_INTERFACE);
        }

    switch (window)
        {
	case I21555_IO_WINDOW:
            regBar = i21555ReadConfig(D_MEM1_BAR);
            regTran = i21555ReadConfig(D_MEM1_TRAN);
            regSetup = i21555ReadConfig(D_MEM1_SETUP);

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            regSetup = i21555SetupBARSize(regSetup);

            /* If previously allocated then free up the memory */
            regBar &= REG_BAR_SIZE_MASK;
            if (isI21555AllocatedMem(regBar,regSetup))
                i21555MemFree (CPCI_IO_SPACE, regBar, regSetup);

            /* Allocate memory */
            if ((pciAddr = i21555MemAlloc(CPCI_IO_SPACE, size)) == 0xFFFFFFFF)
                return (BRIDGE_WIN_SIZE_TOO_LARGE);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
            i21555WriteConfig(D_MEM1_TRAN, regTran);

            /* Do BAR register */
            regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
            i21555WriteConfig(D_MEM1_BAR, regBar);

            /* Compute CPU address */
            *cpuAddress = PCI_IO_TO_CPU(pciIOList.pciMemBase) +
                          (pciAddr - pciIOList.pciMemBase);
	    break;

	case I21555_MEM_WINDOW1:
            regBar = i21555ReadConfig(D_MEM0_BAR);
            regTran = i21555ReadConfig(D_MEM0_TRAN);
            regSetup = i21555ReadConfig(D_MEM0_SETUP);

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            regSetup = i21555SetupBARSize(regSetup);

            /* Size too large */
            if (prefetchable == TRUE)
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_PF_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_PF_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM0_TRAN, regTran);

                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM0_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase) +
                              (pciAddr - pciPfMemList.pciMemBase);
                }
            else
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM0_TRAN, regTran);

                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM0_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_MEM_TO_CPU(pciMemList.pciMemBase) +
                              (pciAddr - pciMemList.pciMemBase);
                }
	    break;

	case I21555_MEM_WINDOW2:
            regBar = i21555ReadConfig(D_MEM2_BAR);
            regTran = i21555ReadConfig(D_MEM2_TRAN);
            regSetup = i21555ReadConfig(D_MEM2_SETUP);

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            regSetup = i21555SetupBARSize(regSetup);

            /* Size too large */
            if (prefetchable == TRUE)
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_PF_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_PF_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM2_TRAN, regTran);

                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM2_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase) +
                              (pciAddr - pciPfMemList.pciMemBase);
                }
            else
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM2_TRAN, regTran);

                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM2_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_MEM_TO_CPU(pciMemList.pciMemBase) +
                              (pciAddr - pciMemList.pciMemBase);
                }
	    break;
	case I21555_MEM_WINDOW3:
            regBar = i21555ReadConfig(D_MEM3_BAR);
            regTran = i21555ReadConfig(D_MEM3_TRAN);
            regSetup = i21555ReadConfig(D_MEM3_SETUP);

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            regSetup = i21555SetupBARSize(regSetup);

            /* Size too large */
            if (prefetchable == TRUE)
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_PF_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_PF_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM3_TRAN, regTran);
 
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM3_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase) +
                              (pciAddr - pciPfMemList.pciMemBase);
                }
            else
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM3_TRAN, regTran);
 
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(D_MEM3_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_MEM_TO_CPU(pciMemList.pciMemBase) +
                              (pciAddr - pciMemList.pciMemBase);
                }
	    break;
        default:
            printf("sysI21555MasterPSet: Invalid window parameter. Aborting\n");
            return (BRIDGE_INVALID_WINDOW);
            break;
        }
    return (OK);
    }

/****************************************************************************
*
* sysI21555MasterSSet - Sets up the PCI to CPCI translations 
*
* This routine sets a window to access the Compace PCI bus.  It is assumed
* the the Secondary interface of the I21555 is on the local PCI bus.  This
* routine should only be used in this case.  This is typically called by
* sysI21555MasterSet().
*
* <window> should be I21555_IO_WINDOW, I21555_MEM_WINDOW2,
*          or I21555_MEM_WINDOW3.
* <cpciAddress> is the Compact PCI base address to access.
* <size> the size of the window to set.  Should be a power of 2.
* <prefetchable> is a flag to indicate enabling of pre-fetchable space.  This
*           is only applicable to I21555_MEM_WINDOW2 or I21555_MEM_WINDOW3.
* <cpuAddress> is the returned CPU translated address.
*
* RETURNS: OK - Success
*          BRIDGE_WIN_SIZE_TOO_LARGE - <size> is too large to set up.
*          BRIDGE_INVALID_INTERFACE - Secondary interface is not on local bus.
*                  Use sysI21555MasterPSet() instead 
*          BRIDGE_INVALID_SIZE - If size is not a power of 2.
*
* SEE ALSO: sysI21555MasterPSet(), sysI21555MasterSet()
*
*/ 
STATUS sysI21555MasterSSet
    (
    UINT window,
    UINT cpciAddress,
    UINT size,
    BOOL prefetchable,
    UINT *cpuAddress
    )
    {
    UINT regSetup, regTran, regBar, pciAddr;
   
    if (interfaceOrientation == IS_PRIMARY_INTERFACE)
        {
        printf("sysI21555MasterSSet: Secondary interface is not on local PCI bus.  Use sysI21555MasterPSet() instead\n");
        return (BRIDGE_INVALID_INTERFACE);
        }

    if (isNot2N(size))
        return (BRIDGE_INVALID_SIZE);

    switch (window)
        {
	case I21555_IO_WINDOW:
            regBar = i21555ReadConfig(U_MEM0_BAR);
            regTran = i21555ReadConfig(U_MEM0_TRAN);
            regSetup = i21555ReadConfig(U_MEM0_SETUP);

            regSetup = i21555SetupBARSize(regSetup);

            /* If previously allocated then free up the memory */
            regBar &= REG_BAR_SIZE_MASK;
            if (isI21555AllocatedMem(regBar,regSetup))
                i21555MemFree (CPCI_IO_SPACE, regBar, regSetup);

            /* Size too large */
            if ((pciAddr = i21555MemAlloc(CPCI_IO_SPACE, size)) == 0xFFFFFFFF)
                return (BRIDGE_WIN_SIZE_TOO_LARGE);

            /* Do setup register */
            i21555SetupBAR(U_MEM0_SETUP, REG_SETUP_RESERVED, size, REG_SETUP_IO_SPACE);
            i21555EnableSetupBAR(U_MEM0_SETUP, REG_SETUP_RESERVED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
            i21555WriteConfig(U_MEM0_TRAN, regTran);

            /* Do BAR register */
            regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
            i21555WriteConfig(U_MEM0_BAR, regBar);

            /* Compute CPU address */
            *cpuAddress = PCI_IO_TO_CPU(pciIOList.pciMemBase) +
                          (pciAddr - pciIOList.pciMemBase);
	    break;

	case I21555_MEM_WINDOW1:
            regBar = i21555ReadConfig(U_MEM1_BAR);
            regTran = i21555ReadConfig(U_MEM1_TRAN);
            regSetup = i21555ReadConfig(U_MEM1_SETUP);

            /* Reclaim used memory */
            if (regSetup & REG_SETUP_ENABLE)
                regSetup = i21555SetupBARSize(regSetup);
            else
                regSetup = 0;

            /* Size too large */
            if (prefetchable == TRUE)
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_PF_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_PF_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                i21555SetupBAR(U_MEM1_SETUP, REG_SETUP_RESERVED, size,
                               REG_SETUP_MEM_SPACE | REG_SETUP_PREFETCH);
                i21555EnableSetupBAR(U_MEM1_SETUP, REG_SETUP_RESERVED);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(U_MEM1_TRAN, regTran);
 
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(U_MEM1_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase) +
                              (pciAddr - pciPfMemList.pciMemBase);
                }
            else
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                i21555SetupBAR(U_MEM1_SETUP, REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE);
                i21555EnableSetupBAR(U_MEM1_SETUP, REG_SETUP_RESERVED);

                /* Do translation register */
                regTran = (regTran & REG_TRAN_RESERVED) | cpciAddress;
                i21555WriteConfig(U_MEM1_TRAN, regTran);
 
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(U_MEM1_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_MEM_TO_CPU(pciMemList.pciMemBase) +
                              (pciAddr - pciMemList.pciMemBase);
                }
	    break;
	case I21555_MEM_WINDOW2:
            /* Get the size of the current window */
            i21555TblGetSetup (NULL, NULL, LTABLE_SIZE, &regSetup);
            regBar = i21555ReadConfig(U_MEM2_BAR);

            /* Size too large */
            if (prefetchable == TRUE)
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_PF_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_PF_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                regSetup = LTABLE_DATA_CTRL_PF | LTABLE_DATA_CTRL_EN;
                i21555TblSetup(&cpciAddress, &regSetup, 0, size);
 
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                i21555WriteConfig(U_MEM2_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_PF_MEM_TO_CPU(pciPfMemList.pciMemBase) +
                              (pciAddr - pciPfMemList.pciMemBase);
                }
            else
                {
                /* If previously allocated then free up the memory */
                regBar &= REG_BAR_SIZE_MASK;
                if (isI21555AllocatedMem(regBar,regSetup))
                    i21555MemFree (CPCI_MEM_SPACE, regBar, regSetup);

                if ((pciAddr = i21555MemAlloc(CPCI_MEM_SPACE, size)) == 0xFFFFFFFF)
                    return (BRIDGE_WIN_SIZE_TOO_LARGE);

                regSetup = LTABLE_DATA_CTRL_EN;
                i21555TblSetup(&cpciAddress, &regSetup, 0, size);
 
                regBar = (regBar & REG_BAR_RESERVED) | pciAddr;
                /* Do BAR register */
                i21555WriteConfig(U_MEM2_BAR, regBar);

                /* Compute CPU address */
                *cpuAddress = PCI_MEM_TO_CPU(pciMemList.pciMemBase) +
                              (pciAddr - pciMemList.pciMemBase);
                }
            break;
        default:
            printf("sysI21555MasterSSet: Invalid window parameter. Aborting\n");
            return (BRIDGE_INVALID_WINDOW);
            break;
        }
    return (OK);
    }

/***************************************************************************
* sysI21555SlavePSet - Sets up the CPCI to PCI translations 
*
* This routine sets a window to access the local PCI bus.  It is assumed
* the the Primary interface of the I21555 is on the local PCI bus.  This
* routine should only be used in this case.  This is typically called by
* sysI21555SlaveSet().  
*
* Since we're on the Primary interface, we can only modify the translated 
* and bar addresses and only if the Secondary side has enabled the window.  
* Only the Secondary side can set the size.  This does not apply to 
* I21555_MEM_WINDOW3.  See restrictions for the parameters below.
*
* <window> should be I21555_IO_WINDOW, I21555_MEM_WINDOW1 or I21555_MEM_WINDOW2
* <cpciAddress> is the Compact PCI base address to respond to.  If this value
*               is 0xFFFFFFFF then the BAR address is not set.  This allows
*               a master to set this window.
* <pciAddress> is the local PCI base address to respond access when a master
*        accesses <cpciAddress>.
* <size> the size of the window to set.  This is ignored except for 
*        I21555_MEM_WINDOW2.
* <prefetchable> is a flag to indicate enabling of pre-fetchable space.  This
*           is only applicable to I21555_MEM_WINDOW2.
*
*
* RETURNS: OK - Success
*          BRIDGE_INVALID_INTERFACE - Primary interface is not on local bus.
*                  Use sysI21555SlaveSSet() instead 
*          BRIDGE_INVALID_SIZE - If size is not a power of 2.
*          BRIDGE_INVALID_BASE_SIZE - If (<cpciAddress> & (<size> - 1) != 0)
*
*
* SEE ALSO: sysI21555SlaveSSet(), sysI21555SlaveSet()
*
*/ 
STATUS sysI21555SlavePSet
    (
    UINT window,
    UINT cpciAddress,		
    UINT pciAddress,
    UINT size,
    BOOL prefetchable
    )
    {
    UINT regSetup, regTran, regBar;
    
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        printf("sysI21555SlavePSet: Primary interface is not on local PCI bus.  Use sysI21555SlaveSSet() instead\n");
        return (BRIDGE_INVALID_INTERFACE);
        }

    if (isNot2N(size))
        return (BRIDGE_INVALID_SIZE);

    if (cpciAddress & (size - 1))
        return (BRIDGE_INVALID_BASE_SIZE);

    switch (window)
        {
	case I21555_IO_WINDOW:
            regBar = i21555ReadConfig(CFG_HDR2(U_MEM0_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(U_MEM0_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(U_MEM0_SETUP));

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(U_MEM0_TRAN), regTran);

            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(CFG_HDR2(U_MEM0_BAR), regBar);
                }
	    break;

	case I21555_MEM_WINDOW1:
            regBar = i21555ReadConfig(CFG_HDR2(U_MEM1_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(U_MEM1_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(U_MEM1_SETUP));

            /* Check to see if window is enabled */
            if (!(regSetup & REG_SETUP_ENABLE))
	        return (BRIDGE_WIN_NOT_ENABLED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(U_MEM1_TRAN), regTran);

            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM1_BAR, regBar);
                }
	    break;
	case I21555_MEM_WINDOW2:
            /* Get the size of the current window */
            i21555TblGetSetup (NULL, NULL, LTABLE_SIZE, &regSetup);

            regBar = i21555ReadConfig(CFG_HDR2(U_MEM2_BAR));

            if (prefetchable == TRUE)
                regSetup = LTABLE_DATA_CTRL_PF | LTABLE_DATA_CTRL_EN;
            else
                regSetup = LTABLE_DATA_CTRL_EN;

            i21555TblSetup(&pciAddress, &regSetup, 0, size);
 
            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(D_MEM2_BAR, regBar);
                }

	    break;
        default:
            printf("sysI21555SlavePSet: Invalid window parameter. Aborting\n");
            return (BRIDGE_INVALID_WINDOW);
	    break;
        }
    return (OK);
    }

/****************************************************************************
* sysI21555SlaveSSet - Sets up the CPCI to PCI translations 
*
* This routine sets a window to access the local PCI bus.  It is assumed
* the the Secondary interface of the I21555 is on the local PCI bus.  This
* routine should only be used in this case.  This is typically called by
* sysI21555SlaveSet().  <size> should be a power of 2 and 
* (<cpciAddress> & (<size> - 1)) should be zero.
*
* <window> should be I21555_IO_WINDOW, I21555_MEM_WINDOW1, I21555_MEM_WINDOW2,
*          or I21555_MEM_WINDOW3
* <cpciAddress> is the Compact PCI base address to respond to.  If this value
*               is 0xFFFFFFFF then the BAR address is not set.  This allows
*               a master to set this window.
* <pciAddress> is the local PCI base address to respond access when a master
*        accesses <cpciAddress>.
* <size> the size of the window to set.
* <prefetchable> is a flag to indicate enabling of pre-fetchable space.  This
*           is only applicable to I21555_MEM_WINDOW1, I21555_MEM_WINDOW2, or 
*           I21555_MEM_WINDOW3.
*
* Note that I21555_MEM_WINDOW1 maps the CSR registers into the first
* 4KB of the memory window.
*
* RETURNS: OK - Success
*          BRIDGE_INVALID_INTERFACE - Secondary interface is not on local bus.
*                  Use sysI21555SlavePSet() instead 
*          BRIDGE_INVALID_SIZE - If size is not a power of 2.
*          BRIDGE_INVALID_BASE_SIZE - If (<cpciAddress> & (<size> - 1) != 0)
*
* SEE ALSO: sysI21555SlavePSet(), sysI21555SlaveSet() 
*
*/ 
STATUS sysI21555SlaveSSet
    (
    UINT window,
    UINT cpciAddress,
    UINT pciAddress,
    UINT size,
    BOOL prefetchable
    )
    {
    UINT regSetup, regTran, regBar;
    
    if (interfaceOrientation == IS_PRIMARY_INTERFACE)
        {
        printf("sysI21555SlaveSSet: Secondary interface is not on local PCI bus.  Use sysI21555SlavePSet() instead\n");
        return (BRIDGE_INVALID_INTERFACE);
        }

    if (isNot2N(size))
        return (BRIDGE_INVALID_SIZE);

    if (cpciAddress & (size - 1))
        return (BRIDGE_INVALID_BASE_SIZE);
    
    switch (window)
        {
	case I21555_IO_WINDOW:
            regBar = i21555ReadConfig(CFG_HDR2(D_MEM1_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(D_MEM1_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(D_MEM1_SETUP));

            /* Do setup register */
            i21555SetupBAR(CFG_HDR2(D_MEM1_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_IO_SPACE);
            i21555EnableSetupBAR(CFG_HDR2(D_MEM1_SETUP), REG_SETUP_RESERVED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(D_MEM1_TRAN), regTran);

            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(CFG_HDR2(D_MEM1_BAR), regBar);
                }
	    break;

	case I21555_MEM_WINDOW1:
            regBar = i21555ReadConfig(CFG_HDR2(D_MEM0_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(D_MEM0_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(D_MEM0_SETUP));

            if (prefetchable == TRUE)
                i21555SetupBAR(CFG_HDR2(D_MEM0_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE | REG_SETUP_PREFETCH);
            else
                i21555SetupBAR(CFG_HDR2(D_MEM0_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE);

            i21555EnableSetupBAR(CFG_HDR2(D_MEM0_SETUP), REG_SETUP_RESERVED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(D_MEM0_TRAN), regTran);

            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(CFG_HDR2(D_MEM0_BAR), regBar);
                }
	    break;

	case I21555_MEM_WINDOW2:
            regBar = i21555ReadConfig(CFG_HDR2(D_MEM2_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(D_MEM2_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(D_MEM2_SETUP));

            if (prefetchable == TRUE)
                i21555SetupBAR(CFG_HDR2(D_MEM2_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE | REG_SETUP_PREFETCH);
            else
                i21555SetupBAR(CFG_HDR2(D_MEM2_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE);

            i21555EnableSetupBAR(CFG_HDR2(D_MEM2_SETUP), REG_SETUP_RESERVED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(D_MEM2_TRAN), regTran);

            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(CFG_HDR2(D_MEM2_BAR), regBar);
                }
	    break;

	case I21555_MEM_WINDOW3:
            regBar = i21555ReadConfig(CFG_HDR2(D_MEM3_BAR));
            regTran = i21555ReadConfig(CFG_HDR2(D_MEM3_TRAN));
            regSetup = i21555ReadConfig(CFG_HDR2(D_MEM3_SETUP));

            if (prefetchable == TRUE)
                i21555SetupBAR(CFG_HDR2(D_MEM3_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE | REG_SETUP_PREFETCH);
            else
                i21555SetupBAR(CFG_HDR2(D_MEM3_SETUP), REG_SETUP_RESERVED, size, REG_SETUP_MEM_SPACE);

            i21555EnableSetupBAR(CFG_HDR2(D_MEM3_SETUP), REG_SETUP_RESERVED);

            /* Do translation register */
            regTran = (regTran & REG_TRAN_RESERVED) | pciAddress;
            i21555WriteConfig(CFG_HDR2(D_MEM3_TRAN), regTran);
 
            if (cpciAddress != 0xFFFFFFFF)
                {
                /* Do BAR register */
                regBar = (regBar & REG_BAR_RESERVED) | cpciAddress;
                i21555WriteConfig(CFG_HDR2(D_MEM3_BAR), regBar);
                }
	    break;

        default:
            printf("sysI21555SlaveSSet: Invalid window parameter.  Aborting\n");
            return (BRIDGE_INVALID_WINDOW);
            break;
        }
    return (OK);
    }

/****************************************************************************
*
* sysI21555MasterSet - Sets up access to Compact PCI bus
*
* This routine enables access to the Compact PCI bus from local CPU.
* See sysI21555MasterPSet() and sysI21555MasterSSet() for parameter
* information.
*
* RETURNS: Return value from sysI21555MasterPSet() or sysI21555MasterSSet()
*
* SEE ALSO: sysI21555SlaveSet(), sysI21555MasterPSet(), sysI21555MasterSSet()
*
*/ 
STATUS sysI21555MasterSet
    (
    UINT window,
    UINT cpciAddress,
    UINT size,
    BOOL prefetchable,
    UINT *cpuAddress
    )
    {
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        return (sysI21555MasterSSet(window, cpciAddress, size, prefetchable, cpuAddress));
    else
        return (sysI21555MasterPSet(window, cpciAddress, size, prefetchable, cpuAddress));
    }

/****************************************************************************
*
* sysI21555SlaveSet - Sets enables access to local PCI bus
*
* This routine enables access to the local PCI bus from a Compace PCI master.
* See sysI21555SlavePSet() and sysI21555SlaveSSet() for parameter information.*
* RETURNS: Return value from sysI21555SlavePSet() or sysI21555SlaveSSet()
*
* SEE ALSO: sysI21555SlaveSet(), sysI21555SlavePSet(), sysI21555SlaveSSet()
*
*/ 
STATUS sysI21555SlaveSet
    (
    UINT window,
    UINT cpciAddress,
    UINT pciAddress,
    UINT size,
    BOOL prefetchable
    )
    {
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        return (sysI21555SlaveSSet(window, cpciAddress, pciAddress, size, prefetchable));
    else
        return (sysI21555SlavePSet(window, cpciAddress, pciAddress, size, prefetchable));
    }

#ifdef INCLUDE_PCI_SHOW
/****************************************************************************
*
* sysI21555PCIDevConfigShow - Display a device on the cPCI bus
*
* This routine displays information regarding a designated device on the cPCI
* bus.
*
* RETURNS: N/A
*/ 
void sysI21555PCIDevConfigShow
    (
    int busNo,          /* PCI bus number */
    int deviceNo,       /* PCI device number */
    int funcNo,         /* PCI function number */
    int regEnd		/* Additional registers */
    )
    {
    UCHAR cVal;
    USHORT wVal;
    UINT32 lVal;
    int i;

    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_VENDOR_ID, 2, &wVal, 0);
    printf("VENDOR_ID       : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_DEVICE_ID, 2, &wVal, 0);
    printf("DEVICE_ID       : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_COMMAND, 2, &wVal, 0);
    printf("COMMAND         : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_STATUS, 2, &wVal, 0);
    printf("STATUS          : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_REVISION, 1, &cVal, 0);
    printf("REVISION        : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_PROGRAMMING_IF, 1, &cVal, 0);
    printf("PROGRAMMING_IF  : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_CLASS, 1, &cVal, 0);
    printf("CLASS           : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_CACHE_LINE_SIZE, 1, &cVal, 0);
    printf("CACHE_LINE_SIZE : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_LATENCY_TIMER, 1, &cVal, 0);
    printf("LATENCY_TIMER   : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_HEADER_TYPE, 1, &cVal, 0);
    printf("HEADER_TYPE     : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BIST, 1, &cVal, 0);
    printf("BIST            : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_0, 4, &lVal, 0);
    printf("BASE_ADDRESS_0  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_1, 4, &lVal, 0);
    printf("BASE_ADDRESS_1  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_2, 4, &lVal, 0);
    printf("BASE_ADDRESS_2  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_3, 4, &lVal, 0);
    printf("BASE_ADDRESS_3  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_4, 4, &lVal, 0);
    printf("BASE_ADDRESS_4  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_BASE_ADDRESS_5, 4, &lVal, 0);
    printf("BASE_ADDRESS_5  : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_CIS, 4, &lVal, 0);
    printf("BASE_CIS        : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_SUB_VENDER_ID, 2, &wVal, 0);
    printf("SUB_VENDER_ID   : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_SUB_SYSTEM_ID, 2, &wVal, 0);
    printf("SUB_SYSTEM_ID   : %8x\n", wVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_EXPANSION_ROM, 4, &lVal, 0);
    printf("EXPANSION_ROM   : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_RESERVED_0, 4, &lVal, 0);
    printf("RESERVED_0      : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_RESERVED_1, 4, &lVal, 0);
    printf("RESERVED_1      : %8x\n", lVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_DEV_INT_LINE, 1, &cVal, 0);
    printf("DEV_INT_LINE    : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_DEV_INT_PIN, 1, &cVal, 0);
    printf("DEV_INT_PIN     : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_MIN_GRANT, 1, &cVal, 0);
    printf("MIN_GRANT       : %8x\n", cVal);
    sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, PCI_CFG_MAX_LATENCY, 1, &cVal, 0);
    printf("MAX_LATENCY     : %8x\n", cVal);

    for (i=((PCI_CFG_MAX_LATENCY + 3) & ~3); i < regEnd; i+=4)
        { 
        sysI21555PCIConfigRead ( busNo, deviceNo, funcNo, i, 4, &lVal, 0);
        printf("Reg %8x    : %8x\n", i,lVal); 
        } 
    }

/****************************************************************************
*
* sysI21555PCIShow - show all the devices on CompactPCI bus.  
*
* This function displays a table of all the PCI devices on the target bus and
* subordinate buses.  The output will contain the bus number, device, number,
* function number, device/vendor ID, and type of device description.
* The bus number, device number, and function number can be used in the
* calls to sysI21555PCIConfigRead and sysI21555PCIConfigWrite. Note that 
* this is used only for the target (subordinate) bus. 
*
* If <devVenID> is not NULL then up to <maxEntries> of the PCI device/vendor
* information found will be placed in the array pointed to by <devVenID>.  Any 
* unused entries up to <maxEntries>-1 wil be set to 0xffffffff.  
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555PCIConfigRead(), sysI21555PCIConfigWrite(), sysI21555PCIConfigProbe()
*
*/ 
void sysI21555PCIShow
    (
    UINT *devVenID,
    UINT maxEntries
    )
    {
    static struct ccode {
        UINT codeValue;   
        char *typeDev;
    } classCode[] = { 
        {0x0000, "All currently implemented devices"},
        {0x0001, "VGA compatible device"},
        {0x0100, "SCSI bus controller"},
        {0x0101, "IDE controller"},
        {0x0102, "Floppy disk controller"},
        {0x0103, "IPI bus controller"},
        {0x0180, "Other mass storage controller"},
        {0x0200, "Ethernet controller"},
        {0x0201, "Token Ring controller"},
        {0x0202, "FDDI controller"},
        {0x0280, "Other network controller"},
        {0x0300, "VGA compatible controller"},
        {0x0301, "XGA controller"},
        {0x0380, "Other display controller"},
        {0x0400, "Video"},
        {0x0401, "Audio"},
        {0x0480, "Other multimedia device"},
        {0x0500, "RAM"},
        {0x0501, "FLASH"},
        {0x0580, "Other memory controller"},
        {0x0600, "Host bridge"},
        {0x0601, "ISA bridge"},
        {0x0602, "EISA bridge"},
        {0x0603, "MC bridge"},
        {0x0604, "PCI-to-PCI bridge"},
        {0x0605, "PCMCIA bridge"},
        {0x0680, "Other bridge device"},
        {0x0700, "Generice XT-compatible serial controller"},
        {0x0701, "Paralle Port"},
        {0x0702, "Multiport serial controller"},
        {0x0703, "Modem"},
        {0x0780, "Other commumication device"},
        {0x0800, "Programmable Interrupt Controller"},
        {0x0801, "DMA controller"},
        {0x0802, "Timer"},
        {0x0803, "RTC controller"},
        {0x0804, "PCI Hot-Plug controller"},
        {0x0880, "Other system peripheral"},
        {0x0900, "Keyboard controller"},
        {0x0901, "Digitzer (pen)"},
        {0x0902, "Mouse controller"},
        {0x0903, "Scanner controller"},
        {0x0904, "Gameport controller"},
        {0x0980, "Other input controller"},
        {0x0a00, "Docking station"},
        {0x0b00, "386 Processor"},
        {0x0b01, "486 Processor"},
        {0x0b02, "Pentium Processor"},
        {0x0b10, "Alpha Processor"},
        {0x0b20, "PowerPC Processor"},
        {0x0b30, "MIPS Processor"},
        {0x0b40, "Co-processor"},
        {0x0c00, "IEEE 1394"},
        {0x0c01, "ACCESS.bus"},
        {0x0c02, "SSA"},
        {0x0c03, "USB"},
        {0x0c04, "Fibre Channel"},
        {0x0c05, "SMBus (System Management Bus)"},
        {0x0d00, "iRDA compatible controller"},
        {0x0d01, "Consumer IR controller"},
        {0x0d10, "RF controller"},
        {0x0d80, "Other type of wireless controller"},
        {0x0e00, "I2O Spec 1.0"},
        {0x0f01, "Saltellite TV controller"},
        {0x0f02, "Saltellite Audio controller"},
        {0x0f03, "Saltellite Voice controller"},
        {0x0f04, "Saltellite Data controller"},
        {0x1000, "Network/computing encryption/decryption"},
        {0x1010, "Entertainment encryption/decryption"},
        {0x1080, "Other encryption/decryption"},
        {0x1100, "DPIO"},
        {0x1080, "Oether data acq/signal processing controller"},
    };
  
    UINT pciDevVen;
    USHORT pciClass; 
    char *tempDevName = NULL; 
    int k, devNum, busNum, funcNum, index=0;
    int numClass = sizeof(classCode)/sizeof(struct ccode); 

    STATUS status;

    printf("PCI BUS#     DEVICE#    FUNCTION#    DEVICE/VENDOR ID    TYPE OF DEVICE\n");
    printf("             (IDSEL)                                                    \n");
    printf("--------     -------    ---------    ----------------    ---------------\n");

    /* Do Type 0 Configuration probe */
    for (devNum=1; devNum < (1 << 21); devNum <<= 1)
        {
        for (funcNum=0; funcNum<8; funcNum++)
            {
            status = sysI21555PCIConfigRead (0, devNum, funcNum, 0, 4, &pciDevVen, 0);
            if (status == OK)
                {
                status = sysI21555PCIConfigRead (0, devNum, funcNum, 0x0a, 2, &pciClass, 0);
                if (status == OK)
                    {
                    for (k=0; k<numClass; k++)
                        if (pciClass == classCode[k].codeValue)
                            {
                            if (tempDevName != classCode[k].typeDev)
                                {
                                printf("%8d    %8x    %9d    %16x    %s\n", 0, devNum, funcNum, pciDevVen, classCode[k].typeDev); 
                                if ((index < maxEntries) && (devVenID)) 
                                    devVenID[index++] = pciDevVen; 
                                }
                            tempDevName = classCode[k].typeDev;
			    break;
                            }
		    if (k == numClass)
                        printf("%8d    %8x    %9d    %16x    %s\n", 0, devNum, funcNum, pciDevVen, "UNKNOWN"); 
                    }
                }
            }
            tempDevName = NULL;
        }

    tempDevName = NULL;
    /* Do Type 1 Configuration probe */
    for (busNum=0; busNum < 256; busNum++)
        {
        for (devNum=0; devNum < 0x20; devNum++)
            {
            for (funcNum=0; funcNum<8; funcNum++)
                {
                status = sysI21555PCIConfigRead (busNum, devNum, funcNum, 0, 4, &pciDevVen, 1);
                if (status == OK)
                    {
                    status = sysI21555PCIConfigRead (busNum, devNum, funcNum, 0x0a, 2, &pciClass, 1);
                    if (status == OK)
                        {
                        for (k=0; k<numClass; k++)
                            if (pciClass == classCode[k].codeValue)
                                {
                                if (tempDevName != classCode[k].typeDev)
                                    {
                                    printf("   %d%12d%12d\t       0x%-10x\t%s\n", busNum, devNum, funcNum, pciDevVen, classCode[k].typeDev); 
                                    if ((index < maxEntries) && (devVenID)) 
			                devVenID[index++] = pciDevVen; 
                                    }
                                tempDevName = classCode[k].typeDev;
			        break;
                                }
		            if (k == numClass)
                            printf("   %d%12d%12d\t       0x%-10x\t%s\n", busNum, devNum, funcNum, pciDevVen, "UNKNOWN"); 
                        }
                    }
                }
            tempDevName = NULL;
            }
        tempDevName = NULL;
        }

    if (devVenID)
        for (k=index; k < maxEntries; k++)
            devVenID[k] = 0xffffffff;

    return;
    }

/****************************************************************************
* sysI21555SlaveDisplay - Display the CompactPCI Slave mappings
*
* This routine displays the current address mapping from the CompactPCI bus, 
* through the PCI, and to the CPU address corresponding to each CompactPCI 
* address.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555MasterDisplay()
*
*/
void sysI21555SlaveDisplay (void)
    {
    int i;
    UINT tVal; 
    BOOL pHdr = TRUE;
    UINT wBaseAdr, wSize, wLocalAdr;
    I21555_CSR *i21555Csr;

    if (sysCPCIEnable == FALSE)
        {
        printf("sysI21555SlaveDisplay : CPCI interface not available\n");
     	return;
	}

    /* Secondary interface is on local bus ? */
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        /* Downstream Memory 0 Window */
        wLocalAdr = i21555ReadConfig(D_MEM0_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(D_MEM0_BAR));
        wSize = i21555ReadConfig(D_MEM0_SETUP);
        /* There's only mapping when the window is larger than or equal to CSR_SIZE */
        if (i21555SetupBARSize(wSize) >= CSR_SIZE)
            {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 1 Window */
	wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(D_MEM1_BAR));
        wSize = i21555ReadConfig(D_MEM1_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 2 Window */
	wLocalAdr = i21555ReadConfig(D_MEM2_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(D_MEM2_BAR));
        wSize = i21555ReadConfig(D_MEM2_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 3 Window */
	wLocalAdr = i21555ReadConfig(D_MEM3_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(D_MEM3_BAR));
        wSize = i21555ReadConfig(D_MEM3_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }
	}
    else
	{
        i21555Csr = i21555GetCsr();

        /* Upstream Memory 0 Window */
        wLocalAdr = i21555ReadConfig(U_MEM0_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(U_MEM0_BAR));
        wSize = i21555ReadConfig(U_MEM0_SETUP);
        /* There's only mapping when the window is larger than or equal to CSR_SIZE */
        if (i21555SetupBARSize(wSize) >= CSR_SIZE)
            {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Upstream Memory 1 Window */
	wLocalAdr = i21555ReadConfig(U_MEM1_TRAN);
        wBaseAdr = i21555ReadConfig(CFG_HDR2(U_MEM1_BAR));
        wSize = i21555ReadConfig(U_MEM1_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Do the Lookup Table */
	wBaseAdr = i21555ReadConfig(CFG_HDR2(U_MEM2_BAR));
        wSize = i21555ReadConfig(CHIP_CONTROL);
        if (wSize & LTABLE_LUT_SEL)
            wSize = tblPageLUT1[((wSize & LTABLE_MASK) >> LTABLE_SHFT)];
        else
            wSize = tblPageLUT0[((wSize & LTABLE_MASK) >> LTABLE_SHFT)];

        for (i=0; i < LTABLE_SIZE; i++)
            {
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | PSWAP((i*4));
            tVal = PSWAP(i21555Csr->tableData);
 	    if (tVal & 0x1)
	        {
	        wLocalAdr = tVal & 0xFFFFFFFC;
	        printI21555Slave(wBaseAdr+(wSize*i), wLocalAdr, (wSize | (wLocalAdr & 0x9)), pHdr);
	        pHdr =  FALSE;
	        }
            }
	}
    }

/****************************************************************************
* sysI21555MasterDisplay - Display the CompactPCI Master mappings
*
* This routine displays the current address mapping from the CPU, through the 
* PCI, and to the CompactPCI address corresponding to each CPU/PCI address.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555SlaveDisplay()
*
*/
void sysI21555MasterDisplay (void)
    {
    int i;
    UINT tVal; 
    BOOL pHdr=TRUE;
    UINT wBaseAdr, wSize, wLocalAdr;
    I21555_CSR *i21555Csr;

    if (sysCPCIEnable == FALSE)
	{
        printf("sysI21555MasterDisplay : CPCI interface not available\n");
        return;
	}

    /* Secondary interface is on local bus ? */
    if (interfaceOrientation == IS_SECONDARY_INTERFACE)
        {
        i21555Csr = i21555GetCsr();

        /* Upstream Memory 0 Window */
        wBaseAdr = i21555ReadConfig(U_MEM0_TRAN);
        wLocalAdr = i21555ReadConfig(U_MEM0_BAR);
        wSize = i21555ReadConfig(U_MEM0_SETUP);
        if (wSize & REG_SETUP_ENABLE)
            {
	    printI21555Master(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Upstream Memory 1 Window */
	wBaseAdr = i21555ReadConfig(U_MEM1_TRAN);
        wLocalAdr = i21555ReadConfig(U_MEM1_BAR);
        wSize = i21555ReadConfig(U_MEM1_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Master(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Do the Lookup Table */
	wLocalAdr = i21555ReadConfig(U_MEM2_BAR);
        wSize = i21555ReadConfig(CHIP_CONTROL);
        if (wSize & LTABLE_LUT_SEL)
            wSize = tblPageLUT1[((wSize & LTABLE_MASK) >> LTABLE_SHFT)];
        else
            wSize = tblPageLUT0[((wSize & LTABLE_MASK) >> LTABLE_SHFT)];
	 
        for (i=0; i < LTABLE_SIZE; i++)
            {
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | PSWAP((i*4));
            tVal = PSWAP(i21555Csr->tableData);
 	    if (tVal & 0x1)
	        {
	        wBaseAdr = tVal & 0xFFFFFFFC;
	        printI21555Master(wBaseAdr, wLocalAdr+(wSize*i), (wSize | (wBaseAdr & 0x9)), pHdr);
	        pHdr = FALSE;
	        }
            }
	}
    else
	{
        /* Downstream Memory 0 Window */
        wBaseAdr = i21555ReadConfig(D_MEM0_TRAN);
        wLocalAdr = i21555ReadConfig(D_MEM0_BAR);
        wSize = i21555ReadConfig(D_MEM0_SETUP);
        /* There's only mapping when the window is larger than or equal CSR_SIZE */
        if (i21555SetupBARSize(wSize) >= CSR_SIZE)
            {
	    printI21555Slave(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 1 Window */
	wBaseAdr = i21555ReadConfig(D_MEM1_TRAN);
        wLocalAdr = i21555ReadConfig(D_MEM1_BAR);
        wSize = i21555ReadConfig(D_MEM1_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Master(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 2 Window */
	wBaseAdr = i21555ReadConfig(D_MEM2_TRAN);
        wLocalAdr = i21555ReadConfig(D_MEM2_BAR);
        wSize = i21555ReadConfig(D_MEM2_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Master(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }

        /* Downstream Memory 3 Window */
	wBaseAdr = i21555ReadConfig(D_MEM3_TRAN);
        wLocalAdr = i21555ReadConfig(D_MEM3_BAR);
        wSize = i21555ReadConfig(D_MEM3_SETUP);
	if (wSize & REG_SETUP_ENABLE)
	    {
	    printI21555Master(wBaseAdr, wLocalAdr, wSize, pHdr);
	    pHdr =  FALSE;
	    }
	}
    }
#endif

/*****************************************************************************
* i21555ReadConfig - Read a i21555 Config Register
*
* This routine read either the local or subordinate bus Configuration
* Register.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL UINT i21555ReadConfig 
    (
    UINT regAdr
    )
    {
    UINT val;

    /* Check to see if this is the local bus */
    if (((UINT)regAdr & 0xFFFFFF00) == 0)
        {
        if (pciConfigInLong(BUS_NO(pciConfigAddr), DEV_NO(pciConfigAddr),
                            FUNC_NO(pciConfigAddr), regAdr, &val) == 0xFFFFFFFF)
        return (0xFFFFFFFF);
        }
    else
        if (sysI21555PCIConfigRead ((((UINT)regAdr >> 16) & 0xFFFF), 
                                    (((UINT)regAdr >> 11) & 0x1F), 
                                    (((UINT)regAdr >> 8) & 0x7), 
                                    ((UINT)regAdr & 0xFC), 4, &val,
                                    ((UINT)regAdr & 0x3)) == 0xFFFFFFFF)
            return (0xFFFFFFFF);
    return (val);
    }

/*****************************************************************************
* i21555WriteConfig - Write a i21555 Config Register
*
* This routine write either the local or subordinate bus Configuration
* Register.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL STATUS i21555WriteConfig 
    (
    UINT regAdr,
    UINT val
    )
    {
    /* Check to see if this is the local bus */
    if (((UINT)regAdr & 0xFFFFFF00) == 0)
        {
        if ((pciConfigOutLong(BUS_NO(pciConfigAddr), DEV_NO(pciConfigAddr),
                              FUNC_NO(pciConfigAddr),
                              regAdr, val)) == 0xFFFFFFFF)
        return (0xFFFFFFFF);
        }
    else
        if (sysI21555PCIConfigWrite ((((UINT)regAdr >> 16) & 0xFFFF), 
                                     (((UINT)regAdr >> 11) & 0x1F), 
                                     (((UINT)regAdr >> 8) & 0x7), 
                                     ((UINT)regAdr & 0xFC), 4, val,
                                     ((UINT)regAdr & 0x3)) == 0xFFFFFFFF)
            return (0xFFFFFFFF);

    return (OK);
    }

/**************************************************************************
* i21555SetupBARSize - determine the size of a PCI region
*
* This routine determines the size of PCI address region.  It takes as input
* the value of the BAR after being written with 0xffffffff.
*
* RETURNS: the size of the region
*/
LOCAL UINT i21555SetupBARSize
    (
    UINT setupBar
    )
    {
    int i;

    setupBar &= 0xFFFFFFE0;

    for (i=0; i < 32; i++)
        if (setupBar & (1 << i))
            return ((1 << i));
 
    return (0);
    }

/****************************************************************************
* i2155SetupBARMask - calculate the mask for a BAR to achieve proper alignment
*
* This routine determines the proper mask for the calculation of a BAR.  It
* takes as input the value of the BAR after being written with 0xffffffff.
*
* RETURNS: the proper mask for the address
*/
LOCAL UINT i21555SetupBARMask
    (
    UINT setupBar
    )
    {
    int i;
    UINT val=0;
 
    setupBar &= 0xFFFFFFE0;
 
    for (i=0; i < 32; i++)
        {
        if (setupBar & (1 << i))
            break;
 
        val |= (1 << i);
        }
 
    return (~val);
    }

/*****************************************************************************
* i21555VerifyLocalSecondary - Verifies that local translated address in not the board's own address.
*
* This routine verifies that local translated address in not
* the board's own address.  If it is then change <localAdr> to
* the value of the local address.  <space> is the bus space identifier 
* and should be one of the following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL void i21555VerifyLocalSecondary 
    (
    UINT space,
    UINT busAdr,
    UINT *localAdr
    )
    {
    UINT wBusAdr;
    UINT wLocalAdr;
    UINT wLocalSize;
    UINT tVal;
    int i;
    I21555_CSR *i21555Csr = i21555GetCsr();

    switch (space)
        {
        case CPCI_MEM_SPACE:
        case CPCI_PF_MEM_SPACE:

            /* Look at the Upstream Memory 0 Setup Register */
            if ((i21555ReadConfig(D_MEM0_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(U_MEM0_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(U_MEM0_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(U_MEM0_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
	        if ((wLocalSize >= CSR_SIZE) &&
                    (busAdr >= wBusAdr) &&
                    (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }

            /* Look at the Upstream Memory 1 Setup Register */
            if ((i21555ReadConfig(U_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(U_MEM1_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(U_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(U_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }

            wBusAdr = i21555ReadConfig(U_MEM2_BAR) & 0xFFFFFFE0;
            i = i21555ReadConfig(CHIP_CONTROL);
            if (i & LTABLE_LUT_SEL)
                wLocalSize = tblPageLUT1[((i & LTABLE_MASK) >> LTABLE_SHFT)];
            else
                wLocalSize = tblPageLUT0[((i & LTABLE_MASK) >> LTABLE_SHFT)];


            /* Look at the Upstream Memory 2 Lookup Table */
            if (wLocalSize)
                for (i=0; i < LTABLE_SIZE; i++)
                    {
                    i21555Csr->tableOffset =
                        (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) |
                        PSWAP((i*4));
                    tVal = i21555Csr->tableData;
                    tVal = PSWAP(tVal);
 
            /* Is valid and match the space designation ? */
            if ((tVal & 1) && ((tVal & 0x8) == space))
                {
                wLocalAdr = tVal & 0xFFFFFFE0;
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr) + i*wLocalSize);
                    return;
                    }
                }
            }
            break;

        case CPCI_IO_SPACE:
            /* Look at the Downstream Memory 1 Setup Register */
            if ((i21555ReadConfig(D_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
                wBusAdr = i21555ReadConfig(D_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
	            *localAdr = PCI_IO_TO_CPU(*localAdr);
                    return;
                    }
                }
            break;

        case CPCI_CNFG_SPACE:
            /* There are no local addresses accessable through
             * Configuration Space */
            break;
        }
    }

/****************************************************************************
* i21555VerifyLocalPrimary - Verifies that local translated address in not
*  the board's own address.
*
* This routine verifies that local translated address in not
*  the board's own address.  If it is then change <localAdr> to
*  the value of the local address.  <space> is the bus space identifier 
*  and should be one of the following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL void i21555VerifyLocalPrimary 
    (
    UINT space,
    UINT busAdr,
    UINT *localAdr
    )
    {
    UINT wBusAdr;
    UINT wLocalAdr;
    UINT wLocalSize;

    switch (space)
        {
        case CPCI_MEM_SPACE:
        case CPCI_PF_MEM_SPACE:
            /* Look at the Downstream Memory 0 Setup Register */
            if ((i21555ReadConfig(D_MEM0_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM0_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(D_MEM0_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM0_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }

            /* Look at the Downstream Memory 1 Setup Register */
            if ((i21555ReadConfig(D_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(D_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }

            /* Look at the Downstream Memory 2 Setup Register */
            if ((i21555ReadConfig(D_MEM2_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM2_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(D_MEM2_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM2_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }

            /* Look at the Downstream Memory 3 Setup Register */
            if ((i21555ReadConfig(D_MEM3_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM3_TRAN);
                wLocalAdr = (wLocalAdr - PCI2DRAM_BASE_ADRS);
                wBusAdr = i21555ReadConfig(D_MEM3_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM3_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    return;
                    }
                }
            break;

        case CPCI_IO_SPACE:
            /* Look at the Downstream Memory 1 Setup Register */
            if ((i21555ReadConfig(D_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
                wBusAdr = i21555ReadConfig(D_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wLocalSize)))
                    {
                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
	            *localAdr = PCI_IO_TO_CPU(*localAdr);
                    return;
                    }
                }
            break;

        case CPCI_CNFG_SPACE:
            /* There are no local addresses accessable through
             * Configuration Space */
            break;
        }
    }

/****************************************************************************
* i21555TranslateSecondary - Translate a CPCI bus to local address
*
* This routine translates a CompactPCI bus address to a local CPU
* address. <space> is the bus space identifier and should be one
* of the following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL STATUS i21555TranslateSecondary 
    (
    UINT space,
    UINT busAdr,
    UINT *localAdr,
    UINT cfgHdrOffset,
    UINT localOffsetMem,
    UINT localOffsetPfMem,
    UINT localOffsetIO,
    UINT busOffsetMem,
    UINT busOffsetIO
    )
    {
    UINT wBusAdr;
    UINT wLocalAdr=0;
    UINT wBusSize;
    UINT tVal;
    int i;
    STATUS status = ERROR;
    I21555_CSR *i21555Csr = i21555GetCsr();

    switch (space)
        {
        case CPCI_MEM_SPACE:
        case CPCI_PF_MEM_SPACE:
            /* Look at the Upstream Memory 0 Setup Register */
            if ((i21555ReadConfig(U_MEM0_SETUP) & 0x9) == space)
                {
                wBusAdr = i21555ReadConfig(U_MEM0_TRAN);
                wBusAdr -= busOffsetMem;
                wLocalAdr = i21555ReadConfig(cfgHdrOffset + U_MEM0_BAR) & 0xFFFFFFE0;
                wBusSize = i21555ReadConfig(U_MEM0_SETUP);
                wBusSize = i21555SetupBARSize(wBusSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wBusSize)))
                    {
                    tVal = i21555ReadConfig(U_MEM0_SETUP);

                    *localAdr = (wLocalAdr +
                                 (busAdr & i21555SetupBARMask(tVal)));
                    if (space == CPCI_MEM_SPACE)
                        *localAdr = *localAdr - pciMemList.pciMemBase +
                                    localOffsetMem;
                    else
                        *localAdr = *localAdr - pciPfMemList.pciMemBase +
                                    localOffsetPfMem;

                    status = OK;
                    break;
                    }
                }

            /* Look at the Upstream Memory 1 Setup Register */
            if ((i21555ReadConfig(U_MEM1_SETUP) & 0x9) == space)
                {
                wBusAdr = i21555ReadConfig(U_MEM1_TRAN);
                wBusAdr -= busOffsetMem;
                wLocalAdr = i21555ReadConfig(cfgHdrOffset + U_MEM1_BAR) & 0xFFFFFFE0;
                wBusSize = i21555ReadConfig(U_MEM1_SETUP);
                wBusSize = i21555SetupBARSize(wBusSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wBusSize)))
                    {
                    tVal = i21555ReadConfig(U_MEM1_SETUP);

                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    if (space == CPCI_MEM_SPACE)
                        *localAdr = *localAdr - pciMemList.pciMemBase +
                                    localOffsetMem;
                    else
                        *localAdr = *localAdr - pciPfMemList.pciMemBase +
                                    localOffsetPfMem;
                    status = OK;
                    break;
                    }
                }

            wLocalAdr = i21555ReadConfig(cfgHdrOffset + U_MEM2_BAR) & 0xFFFFFFE0;
            i = i21555ReadConfig(CHIP_CONTROL);
            if (i & LTABLE_LUT_SEL)
                wBusSize = tblPageLUT1[((i & LTABLE_MASK) >> LTABLE_SHFT)];
            else
                wBusSize = tblPageLUT0[((i & LTABLE_MASK) >> LTABLE_SHFT)];

            /* Look at the Upstream Memory 2 Lookup Table */
            if (wBusSize)
                for (i=0; i < LTABLE_SIZE; i++)
                    {
                    i21555Csr->tableOffset = (i21555Csr->tableOffset &
                                              PSWAP(0xFFFFFF00)) | 
                                              PSWAP((i*4));
                    tVal = i21555Csr->tableData;
                    tVal = PSWAP(tVal);

                    /* Is valid and match the space designation ? */
                    if ((tVal & 1) && ((tVal & 0x8) == space))
                        {
                        wBusAdr = tVal & 0xFFFFFFE0;
                        wBusAdr -= busOffsetMem;
                        if ((busAdr >= wBusAdr) &&
                            (busAdr < (wBusAdr + wBusSize)))
                            {
                            *localAdr = wLocalAdr +
                                        (busAdr - wBusAdr + i*wBusSize);
                            if (space == CPCI_MEM_SPACE)
                                *localAdr = *localAdr -
                                            pciMemList.pciMemBase +
                                            localOffsetMem;
                            else
                                *localAdr = *localAdr -
                                            pciPfMemList.pciMemBase +
                                            localOffsetPfMem;
      
                            status = OK;
                            break;
                            }
                        }
                    }
            break;
        case CPCI_IO_SPACE:

            /* Check the Upstream I/O BAR */
            if ((i21555ReadConfig(U_MEM0_SETUP) & 0x9) == space)
                {
                wBusAdr = i21555ReadConfig(U_MEM0_TRAN);
                wBusAdr -= busOffsetIO; 
                wLocalAdr = i21555ReadConfig(cfgHdrOffset + U_IO_BAR) & 0xFFFFFFE0;
                wBusSize = i21555ReadConfig(U_MEM0_SETUP);
                wBusSize = i21555SetupBARSize(wBusSize);
                if ((busAdr >= wBusAdr) && (busAdr < (wBusAdr + wBusSize)))
                    {
                    tVal = i21555ReadConfig(U_MEM0_SETUP);

                    *localAdr = (wLocalAdr + (busAdr - wBusAdr));
                    *localAdr = (*localAdr - pciIOList.pciMemBase +
                                 localOffsetIO);
         
                    status = OK;
                    break;
                    }
                }
            break;
        case CPCI_CNFG_SPACE:
            /* Simply return the address */
            *localAdr = busAdr;
            return (OK);
            break;
        }

    return (status);
    }

/****************************************************************************
* i21555TranslatePrimary - Translate the Primary IC
*
* This routine attempts to translate an address across the Primary interface.
* <space> is the bus space identifier and should be one of the 
* following :
*
* CPCI_MEM_SPACE     - 0, PCI Memory Space
* CPCI_IO_SPACE      - 1, PCI I/O Space
* CPCI_PF_MEM_SPACE  - 8, PCI Prefetchable Memory Space
* CPCI_CNFG_SPACE    - 3, PCI Configuration Space
*
* Note : Currently there is no direct translation to the Configuration Space
*        hence the value return for CPCI_CNFG_SPACE should
*        be used with sysCNFGToPCI() and then sysI21555PCIConfigRead()
*	 in order to read or write the target address.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL STATUS i21555TranslatePrimary 
    (
    UINT space,
    UINT localAdr,
    UINT *busAdr,
    UINT cfgHdrOffset, 
    UINT localOffsetMem,
    UINT localOffsetPfMem,
    UINT localOffsetIO,
    UINT busOffsetMem,
    UINT busOffsetIO
    )
    {
    UINT wBusAdr;
    UINT wLocalAdr;
    UINT wLocalSize;

    switch (space)
        {
        case CPCI_MEM_SPACE:
        case CPCI_PF_MEM_SPACE:
            /* Look at the Downstream Memory 0 Setup Register */
            if ((i21555ReadConfig(D_MEM0_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM0_TRAN);
                wLocalAdr = (wLocalAdr - localOffsetMem);
                wBusAdr = i21555ReadConfig(cfgHdrOffset + D_MEM0_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM0_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((localAdr >= wLocalAdr) && (localAdr < (wLocalAdr + wLocalSize)))
                    {
                    *busAdr = (wBusAdr + localAdr - wLocalAdr + busOffsetMem);
                    return (OK);
                    }
                }

            /* Look at the Downstream Memory 1 Setup Register */
            if ((i21555ReadConfig(D_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
                if (space == CPCI_MEM_SPACE)
                    wLocalAdr = (wLocalAdr - localOffsetMem);
                else
                    wLocalAdr = (wLocalAdr - localOffsetPfMem);
                wBusAdr = i21555ReadConfig(cfgHdrOffset + D_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((localAdr >= wLocalAdr) && (localAdr < (wLocalAdr + wLocalSize)))
                    {
                    *busAdr = (wBusAdr + localAdr - wLocalAdr + busOffsetMem);
                    return (OK);
                    }
                }

            /* Look at the Downstream Memory 2 Setup Register */
            if ((i21555ReadConfig(D_MEM2_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM2_TRAN);
                if (space == CPCI_MEM_SPACE)
                    wLocalAdr = (wLocalAdr - localOffsetMem);
                else
                    wLocalAdr = (wLocalAdr - localOffsetPfMem);
                wBusAdr = i21555ReadConfig(cfgHdrOffset + D_MEM2_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM2_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((localAdr >= wLocalAdr) && (localAdr < (wLocalAdr + wLocalSize)))
                    {
                    *busAdr = (wBusAdr + localAdr - wLocalAdr + busOffsetMem);
                    return (OK);
                    }
                }

            /* Look at the Downstream Memory 3 Setup Register */
            if ((i21555ReadConfig(D_MEM3_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM3_TRAN);
                if (space == CPCI_MEM_SPACE)
                    wLocalAdr = (wLocalAdr - localOffsetMem);
                else
                    wLocalAdr = (wLocalAdr - localOffsetPfMem);
                wBusAdr = i21555ReadConfig(cfgHdrOffset + D_MEM3_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM3_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((localAdr >= wLocalAdr) &&
                    (localAdr < (wLocalAdr + wLocalSize)))
                    {
                    *busAdr = (wBusAdr + localAdr - wLocalAdr + busOffsetMem);
                    return (OK);
                    }
                }
            break;

        case CPCI_IO_SPACE:
            /* Look at the Downstream Memory 1 Setup Register */
            if ((i21555ReadConfig(D_MEM1_SETUP) & 0x9) == space)
                {
                wLocalAdr = i21555ReadConfig(D_MEM1_TRAN);
                wBusAdr = i21555ReadConfig(cfgHdrOffset + D_MEM1_BAR) & 0xFFFFFFE0;
                wLocalSize = i21555ReadConfig(D_MEM1_SETUP);
                wLocalSize = i21555SetupBARSize(wLocalSize);
                if ((localAdr >= wLocalAdr) &&
                    (localAdr < (wLocalAdr + wLocalSize)))
                    {
                    *busAdr = (wBusAdr + localAdr - wLocalAdr + busOffsetIO);
                    return (OK);
                    }
                }
            break;

        case CPCI_CNFG_SPACE:
            /* There are no local addresses accessable through
             * Configuration Space */
            break;
        }

    return (ERROR);
    }

/***************************************************************************
* i21555Orientation - Determines whether the Secondary interface is
*      on the local (PCI) bus.
*
* This routine determines whether the local interface is the secondary or
* primary side of the bridge chip by checking whether the Primary Access 
* Lockout bit is writeable.  
*
* RETURNS: IS_SECONDARY_INTERFACE or IS_PRIMARY_INTERFACE
*
* NOMANUAL:
*/

LOCAL UINT i21555Orientation (void)
    {
    UINT chipControl, testv;
    UINT flag;
    UINT key = intLock();

    chipControl = i21555ReadConfig(CHIP_CONTROL);

    /* Toggle to Primary Access Lockout bit */
    testv = (chipControl & ~PRIMARY_ACCESS_LOCKOUT) | (~chipControl & PRIMARY_ACCESS_LOCKOUT);

    /* Write the register */
    i21555WriteConfig(CHIP_CONTROL, testv);

    /* Read back the value */
    testv = i21555ReadConfig(CHIP_CONTROL);

    /* See if the bit has changed */
    if ((testv & PRIMARY_ACCESS_LOCKOUT) == (chipControl & PRIMARY_ACCESS_LOCKOUT))
        flag = IS_PRIMARY_INTERFACE;
    else
        flag = IS_SECONDARY_INTERFACE;

    /* Write back the original value */
    i21555WriteConfig(CHIP_CONTROL, chipControl);

    intUnlock(key);

    return (flag);
    }

/***************************************************************************
* i21555GetCsr - Returns pointer to the CSR address
*
* This routine returns a pointer to the address of the CSR registers.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL I21555_CSR *i21555GetCsr (void)
    {
    I21555_CSR *i21555Csr=NULL;
    int is_io_space;

    is_io_space = i21555ReadConfig(P_CSR_MEM_BAR);

    if (is_io_space & PCI_CMD_IO_ENABLE)
        i21555Csr = (I21555_CSR *)PCI_IO_TO_CPU((is_io_space & 0xFFFFFFFE));
    else
        i21555Csr = (I21555_CSR *)PCI_MEM_TO_CPU((is_io_space));

    return (i21555Csr);
    }

/***************************************************************************
* i21555EnableSetupBAR - Enables the I21555 Memory Setup BAR translation
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL void i21555EnableSetupBAR 
    (
    UINT regAdr,
    UINT mask
    )
    {
    UINT tVal = 0; 

    /* Get currents bits and set enable bit */
    tVal = i21555ReadConfig(regAdr) | REG_SETUP_ENABLE;

    /* Write the register */
    i21555WriteConfig(regAdr,tVal);
    }

/***************************************************************************
* i21555SetupBAR - Writes to the I21555 Memory Setup BAR
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL void i21555SetupBAR 
    (
    UINT regAdr,
    UINT mask,
    UINT sizeVal,
    UINT ctrlFlag
    )
    {
    UINT tVal = 0; 
    UINT tVal2 = 0; 
    int i;

    for (i=0; i < 32; i++)
        {   
        if ((1 << i) & sizeVal)
            break;

        tVal |= (1 << i);
        }

    /* Get reserved bits */
    tVal2 = i21555ReadConfig(regAdr) & mask;

    /* Generate size mask and clear out reserved bits */
    tVal = ((~tVal & REG_BAR_SIZE_MASK) & ~mask);

    /* Enable base address register               */
    /* Don't enable the translation yet           */
    tVal = (tVal | ctrlFlag | tVal2) & 0x7FFFFFFF;

    /* Write the register */
    i21555WriteConfig(regAdr,tVal);
    }

/***************************************************************************
* i21555TblGetSetup - INTEL 21555 setup of the Lookup Table
*
* Gets the Upstream Lookup Table setup.  <addrList> points to the entries to 
* read from the table.  <ctrlList> points to the list of attributes entries 
* to read for each address. <listCnt> indicates the number of entries to 
* read from the list.  
*
* If <addrList> is NULL then it is not filled.  If <ctrlList> is NULL then 
* it is not filled.
*
* It is assumed that the CSR based address is programmed into the local
* I/O BAR of i21555 (P_CSR_IO_BAR);
*
* RETURNS: OK or ERROR if <winSize> or <listCnt> is invalid
*
* NOMANUAL:
*/

LOCAL STATUS i21555TblGetSetup 
    (
    UINT *addrList,		/* ptr to addresses Table list */
    UINT *ctrlList,		/* ptr to control Table list */
    UINT listCnt,		/* Number of entries in the list */
    UINT *winSize		/* Total size of the translation window */
    )
    {
    I21555_CSR *i21555Csr = i21555GetCsr();
    int i;
    UINT tVal, pageSize, *tblPage;

    if (listCnt > LTABLE_SIZE)
        return (ERROR);

    /* Determine the particular table size being used */
    tVal = i21555ReadConfig(CHIP_CONTROL);
    if (tVal & LTABLE_LUT_SEL)
        tblPage = tblPageLUT1;
    else
        tblPage = tblPageLUT0;

    /* calculate the page size */
    pageSize = tblPage[((tVal & LTABLE_MASK) >> LTABLE_SHFT)];

    /* Get the Lookup Table entries */
    for (i=0; i < listCnt; i++)
        {
        /* send the address to the chip */
        i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | PSWAP((i*4));

        /* read the data from the chip */
        tVal = i21555Csr->tableData;
        tVal = PSWAP(tVal);

        /* add a page to the total window size */
        *winSize += pageSize;

        /* add the entry to the address and control lists */
        if (addrList)
            addrList[i] = (tVal & LTABLE_DATA_BASE_MASK);
        if (ctrlList)
            ctrlList[i] = (tVal & LTABLE_DATA_CTRL_MASK); 
        }

    return (OK);
    }

/***************************************************************************
* i21555TblSetup - INTEL 21555 setup of the Lookup Table
*
* Sets up the Upstream Lookup Table for Window 2 translations.  <addrList>
* points to the list of address entries to write to the table.  <ctrlList>
* points to the list of attributes entries to write for each address. 
* <listCnt> indicates the number of entries passed in the list.  
*
* If <listCnt> is 0 then <winSize> is used to setup LTABLE_SIZE (64) into
* even segments with each translation entry being <winSize>/LTABLE_SIZE in 
* size.  <addrList> then is a pointer to the starting address and
* <ctrlList> is a pointer to the attribute.
*
* <winSize> should be a in the range of { 0, 16K, 32K, 64K, 128K, 256K, 
* 512K, 1MB, 2MB, 4MB, 8MB, 16MB, 32MB, 64MB, 128MB, 256MB, 512MB, 1GB,
* 2GB}.  Any other values are considered invalid.  Also <addrList> is 
* assumed to contained the base address to start from and <ctrlList> 
* contains the attributes for all entries in this case. 
*
* If <listCnt> is > 0 and < LTABLE_SIZE then any unprogrammed entries 
* are left unchanged.
* 
* It is assumed that the CSR based address is programmed into the local
* I/O BAR of i21555 (P_CSR_IO_BAR);
*
* RETURNS: OK or ERROR if <winSize> or <listCnt> is invalid
*
* NOMANUAL:
*/

LOCAL STATUS i21555TblSetup 
    (
    UINT *addrList,		/* ptr to addresses Table list */
    UINT *ctrlList,		/* ptr to control Table list */
    UINT listCnt,		/* Number of entries in the list */
    UINT winSize		/* Total size of the translation window */
    )
    {
    I21555_CSR *i21555Csr = i21555GetCsr();
    int i=0, j;
    UINT tVal, tVal2, *tblPage, tblPageSize=0, lut=0;

    if (listCnt > LTABLE_SIZE)
        return (ERROR);

    /* Find the maximum size/page support when LUT is 0 */
    for (i=0; i < sizeof(tblPageLUT0)/sizeof(UINT); i++)
        if (tblPageLUT0[i] > tblPageSize)
            tblPageSize = tblPageLUT0[i];
      
    /* If the window size is so big such that LUT=0 can't handle it */
    /* then default to LUT=1                                        */
    if (winSize/LTABLE_SIZE > tblPageSize)
        {
        tblPage = tblPageLUT1;
        tblPageSize = sizeof(tblPageLUT1);
        lut = LTABLE_LUT_SEL;
        }
    else
        {
        tblPage = tblPageLUT0;
        tblPageSize = sizeof(tblPageLUT0);
        }
    
    if (!listCnt)
        {
        /* Setup the Upstream 2 BAR using the Table Lookup */
        /* using evenly spaced windows.                    */
 
        /* Determine page size */
        for (i=0; i < tblPageSize; i++)
            if (tblPage[i] == winSize/LTABLE_SIZE)
                {
                tVal = i21555ReadConfig(CHIP_CONTROL);
                tVal |= (i << LTABLE_SHFT);
                i21555WriteConfig(CHIP_CONTROL,tVal);
                break;
                }

        if (i == tblPageSize)
            return (ERROR);

        /* Fill in the Lookup Table entries */
        for (j=0; j < LTABLE_SIZE; j++)
            {
            tVal = PSWAP(((*addrList + (tblPage[i]*j)) | *ctrlList));
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | 
                                 PSWAP((j*4));
            tVal2 = i21555Csr->tableData;
            tVal2 &= PSWAP(0x000000F6);  /* Get reserved bits */
            tVal2 |= tVal;
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | 
                                 PSWAP((j*4));
            i21555Csr->tableData = tVal2;
            }
        }
    else
        {
        /* Fill in the Lookup Table entries */
        for (j=0; j < listCnt; j++)
            {
            tVal = PSWAP(((addrList[j] + tblPage[i]*j) | ctrlList[j]));
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | 
                                 PSWAP((j*4));
            tVal2 = i21555Csr->tableData;
            tVal2 &= PSWAP(0x000000F6);	/* Get reserved bits */
            tVal2 |= tVal;
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | 
                                 PSWAP((j*4));
            i21555Csr->tableData = tVal2;
            }
        }

    /* Now set the base address for bus to local translations */
    tVal = i21555ReadConfig(D_MEM3_BAR);
    tVal |= pciPfMemList.pciMemBase;
    i21555WriteConfig(D_MEM3_BAR,tVal);

    return (OK);
    }
 
/***************************************************************************
* i21555MemInit - Initialize the PCI memory allocation facilities
*
* Set up the structures needed for use in allocation of PCI memory
* usage for PCI to CPCI window setups.
*
* RETURNS: N/A
*
* NOMANUAL:
*/

LOCAL void i21555MemInit (void)
    {
    PCI_MEM_NODE *listPtr; 
   
    /* Initialize the list facilities */ 
    lstInit(&pciIOList.pciLst);
    lstInit(&pciMemList.pciLst);
    lstInit(&pciPfMemList.pciLst);
   
    /* Allocate list for PCI I/O memory usage */
    listPtr = (PCI_MEM_NODE *)memalign(sizeof(LLONG),sizeof(PCI_MEM_NODE));
    if (listPtr != NULL)
        {
        pciIOList.pciMemBase = listPtr->pciMemBase = PCI_BRIDGE_IO_ADRS;
        pciIOList.pciMemTotal = listPtr->pciMemSize = PCI_BRIDGE_IO_SIZE;
        lstAdd(&pciIOList.pciLst, (NODE *)listPtr);
        }
   
    /* Allocate list for PCI Memory memory usage */
    listPtr = (PCI_MEM_NODE *)memalign(sizeof(LLONG),sizeof(PCI_MEM_NODE));
    if (listPtr != NULL)
        {
        pciMemList.pciMemBase = listPtr->pciMemBase = PCI_BRIDGE_MEM_ADRS;
        pciMemList.pciMemTotal = listPtr->pciMemSize = PCI_BRIDGE_MEM_SIZE;
        lstAdd(&pciMemList.pciLst, (NODE *)listPtr);
        }
   
    /* Allocate list for PCI Prefetchable Memory memory usage */
    listPtr = (PCI_MEM_NODE *)memalign(sizeof(LLONG),sizeof(PCI_MEM_NODE));
    if (listPtr != NULL)
        {
        pciPfMemList.pciMemBase = listPtr->pciMemBase = PCI_BRIDGE_PF_MEM_ADRS;
        pciPfMemList.pciMemTotal = listPtr->pciMemSize = PCI_BRIDGE_PF_MEM_SIZE;
        lstAdd(&pciPfMemList.pciLst, (NODE *)listPtr);
        }
    }
 
#ifdef INCLUDE_PCI_SHOW
/***************************************************************************
* i21555MemShow - Show memory allocation table
*
*
* RETURNS: N/A
*
* NOMANUAL:
*/

void i21555MemShow (void)
    {
    PCI_MEM_NODE *pciPtr; 
 
    printf("PCI I/O Space Allocation (Base %8x, Size %8x)\n", pciIOList.pciMemBase, pciIOList.pciMemTotal);
    printf("-------------------------------------------------------\n");

    /* Display I/O space list */
    pciPtr = (PCI_MEM_NODE *)lstFirst(&pciIOList.pciLst);
    while (pciPtr)
        {
        printf("\tMemory Base : %8x\n", pciPtr->pciMemBase);
        printf("\tMemory Size : %8x\n\n", pciPtr->pciMemSize);
        pciPtr = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr);
        }

    printf("PCI Memory Space Allocation (Base %8x, Size %8x)\n", pciMemList.pciMemBase, pciMemList.pciMemTotal);
    printf("----------------------------------------------------------\n");

    /* Display Memory space list */
    pciPtr = (PCI_MEM_NODE *)lstFirst(&pciMemList.pciLst);
    while (pciPtr)
        {
        printf("\tMemory Base : %8x\n", pciPtr->pciMemBase);
        printf("\tMemory Size : %8x\n\n", pciPtr->pciMemSize);
        pciPtr = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr);
        }

    printf("PCI Pre-fetchable Memory Space Allocation (Base %8x, Size %8x)\n", pciPfMemList.pciMemBase, pciPfMemList.pciMemTotal);
    printf("------------------------------------------------------------------------\n");

    /* Display Pre-fetchable Memory space list */
    pciPtr = (PCI_MEM_NODE *)lstFirst(&pciPfMemList.pciLst);
    while (pciPtr)
        {
        printf("\tMemory Base : %8x\n", pciPtr->pciMemBase);
        printf("\tMemory Size : %8x\n\n", pciPtr->pciMemSize);
        pciPtr = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr);
        }
    }
#endif

/****************************************************************************
* isNot2N - determine if a value is a power of 2
*
* This function determines if the parameter is a perfect power of 2.
*
* RETURNS: TRUE if size is a power of 2, FALSE otherwise
*/
LOCAL BOOL isNot2N
    (
    UINT size
    )
    {
    int i;
    BOOL found=FALSE;

    for (i=0; i < 32; i++)
        if ((1 << i) & size)
            {
            if (found == TRUE)
                return (TRUE);
            else
                found = TRUE;
            }

    return (FALSE);
    }
    
/***************************************************************************
* isI21555AllocatedMem - Check to see if memory is from allocated pool
*
* This function checks to see if the given memory region is allocated from
* the range of addresses allocated for memory windows.
*
* RETURNS: N/A
*
* NOMANUAL:
*/
LOCAL BOOL isI21555AllocatedMem    
   (
   UINT base,
   UINT size
   )
   {
   if (size == 0)
       return (FALSE);

   if ((base >= pciIOList.pciMemBase) && ((base + size) <= (pciIOList.pciMemBase + pciIOList.pciMemTotal)))
       return (TRUE);
   if ((base >= pciMemList.pciMemBase) && ((base + size) <= (pciMemList.pciMemBase + pciMemList.pciMemTotal)))
       return (TRUE);
   if ((base >= pciPfMemList.pciMemBase) && ((base + size) <= (pciPfMemList.pciMemBase + pciPfMemList.pciMemTotal)))
       return (TRUE);

   return (FALSE);
   }
    
/***************************************************************************
* i21555MemCleanup - Unfragement memory structures
*
* This function defragments some of the memory structures.
*
* RETURNS: N/A
*
* NOMANUAL:
*/
LOCAL void i21555MemCleanup    
    (
    LIST *lstPtr
    )
    {
    PCI_MEM_NODE *pciPtr1, *pciPtr2;

    /* Get the first node */
    pciPtr1 = (PCI_MEM_NODE *)lstFirst(lstPtr);
  
    while (pciPtr1)
        { 
        pciPtr2 = (PCI_MEM_NODE *)lstPrevious((NODE *)pciPtr1);
        if ((pciPtr2) && (pciPtr2->pciMemBase + pciPtr2->pciMemSize == pciPtr1->pciMemBase))
            {
            pciPtr1->pciMemSize += pciPtr2->pciMemSize;
            pciPtr1->pciMemBase = pciPtr2->pciMemBase;
            lstDelete(lstPtr,(NODE *)pciPtr2);
            cfree((char *)pciPtr2); 
            }

        if ((pciPtr2) && (pciPtr2->pciMemBase + pciPtr2->pciMemSize >= pciPtr1->pciMemBase) && ((pciPtr2->pciMemBase + pciPtr2->pciMemSize) <= (pciPtr1->pciMemBase + pciPtr1->pciMemSize)))
            {
            pciPtr1->pciMemSize += (pciPtr1->pciMemBase - pciPtr2->pciMemBase);
            pciPtr1->pciMemBase = pciPtr2->pciMemBase;
            lstDelete(lstPtr,(NODE *)pciPtr2);
            cfree((char *)pciPtr2); 
            }

        /* Just a precaution, it could happen after i21555MemAlloc is done */
        if (pciPtr1->pciMemSize == 0)
            {
            pciPtr2 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
            lstDelete(lstPtr,(NODE *)pciPtr1);
            cfree((char *)pciPtr1); 
            pciPtr1 = pciPtr2;
            }
        else
            pciPtr1 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
        }
    }
    
LOCAL BOOL checkRangeFail
    (
    UINT poolBase,
    UINT poolSize,
    UINT reqSize
    )
    {
    UINT poolBaseNew;

    if (reqSize > poolSize)
        return (TRUE);

    /* If the pool address is not aligned on the address size then */
    /* round up and see if we still have enough memory             */
    if (poolBase & (reqSize - 1))
        {
        /* Round up to alignment of reqSize */
        poolBaseNew = (poolBase + (reqSize - 1)) & ~(reqSize - 1); 

        /* Now is there still enough memory ? */
        if ((poolBaseNew + reqSize) > (poolBase + poolSize)) 
            return (TRUE);
        }

    return (FALSE);
    }

/***************************************************************************
* i21555MemAlloc - Allocate memory for PCI space
*
* <pciSize> should be a power of 2.
*
* RETURNS: 0xFFFFFFFF if not enough memory available
*
* NOMANUAL:
*/

LOCAL UINT i21555MemAlloc 
    (
    UINT pciSpace,
    UINT pciSize
    )
    {
    LIST *lstPtr;
    PCI_MEM_NODE *pciPtr1, *pciPtr2; 
    UINT pciAddr;

    if (isNot2N(pciSize))
        return (0xFFFFFFFF);

    /* Align to 4 byte boundary */
    pciSize = ((pciSize + sizeof(ULONG) - 1)/sizeof(ULONG))*sizeof(ULONG);

    /* Get the memory allocation list */
    switch (pciSpace)
        {
        case CPCI_IO_SPACE:
            lstPtr = &pciIOList.pciLst;
            break;
        case CPCI_MEM_SPACE:
            lstPtr = &pciMemList.pciLst;
            break;
        case CPCI_PF_MEM_SPACE:
            lstPtr = &pciPfMemList.pciLst;
            break;
        default:
            return (0xFFFFFFFF);
            break;
        }

    /* Get the first node */
    pciPtr1 = (PCI_MEM_NODE *)lstFirst(lstPtr);

    /* Find the fist node bigger than the size */
    while (pciPtr1 && checkRangeFail(pciPtr1->pciMemBase, pciPtr1->pciMemSize, pciSize))
        pciPtr1 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
     
    /* Look to see if any node is best fit */
    pciPtr2 = pciPtr1;
    while (pciPtr1)
        {
        if ((pciPtr1->pciMemSize < pciPtr2->pciMemSize) && 
            (checkRangeFail(pciPtr1->pciMemBase, pciPtr1->pciMemSize, pciSize) == FALSE))
            pciPtr2 = pciPtr1;
        pciPtr1 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
        }

    /* If pciPtr2 == NULL, not enough memory available */
    if (pciPtr2)
        {
        if (pciPtr2->pciMemBase & (pciSize - 1))
            {
            pciPtr1 = (PCI_MEM_NODE *)memalign(sizeof(LLONG),sizeof(PCI_MEM_NODE));
            pciPtr1->pciMemBase = pciPtr2->pciMemBase;
            pciPtr2->pciMemBase = (pciPtr2->pciMemBase + (pciSize - 1)) & ~(pciSize - 1);
            pciPtr1->pciMemSize = pciPtr2->pciMemBase - pciPtr1->pciMemBase;
            pciPtr2->pciMemSize -= pciPtr1->pciMemSize;
            lstInsert(lstPtr, lstPrevious((NODE *)pciPtr2), (NODE *)pciPtr1);
            }

        pciAddr = pciPtr2->pciMemBase;
        pciPtr2->pciMemSize -= pciSize; 
        pciPtr2->pciMemBase += pciSize; 
        }
    else
        pciAddr = 0xFFFFFFFF;

    i21555MemCleanup(lstPtr);
    return (pciAddr);
    }

/***************************************************************************
* i21555MemFree - Free memory for PCI space
*
*
* RETURNS: N/A 
*
* NOMANUAL:
*/

LOCAL void i21555MemFree 
    (
    UINT pciSpace,
    UINT pciAddr,
    UINT pciSize
    )
    {
    LIST *lstPtr;
    PCI_MEM_NODE *pciPtr1, *pciPtr2, *pciPtr3; 

    /* Align to 4 byte boundary */
    pciSize = ((pciSize + sizeof(ULONG) - 1)/sizeof(ULONG))*sizeof(ULONG);

    /* Get the memory allocation list */
    switch (pciSpace)
        {
        case CPCI_IO_SPACE:
            lstPtr = &pciIOList.pciLst;
            break;
        case CPCI_MEM_SPACE:
            lstPtr = &pciMemList.pciLst;
            break;
        case CPCI_PF_MEM_SPACE:
            lstPtr = &pciPfMemList.pciLst;
            break;
        default:
            return;
            break;
        }

    /* Get the first node */
    pciPtr1 = (PCI_MEM_NODE *)lstFirst(lstPtr);

    /* Find the fist node bigger than the size */
    while (pciPtr1 && (pciPtr1->pciMemBase < (pciAddr + pciSize)))
        /* This could happen if we tried to free up memory that's not */
        /* allocated yet.  This could happen if we reboot the system  */
        /* and the base address registers are not cleared.            */
        if ((pciAddr >= pciPtr1->pciMemBase) && 
            ((pciAddr + pciSize) <= (pciPtr1->pciMemBase + pciPtr1->pciMemSize)))
            return;
        else
            pciPtr1 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
     
    /* If this node is the contiguous start of the end of the free range */
    if (pciPtr1 && (pciPtr1->pciMemBase == (pciAddr + pciSize)))
        {
        pciPtr1->pciMemSize += pciSize;
        pciPtr1->pciMemBase = pciAddr;
        i21555MemCleanup(lstPtr);
        return;
        }

    /* If the end of the previous range is the start of the free buffer */
    if (pciPtr1 && lstPrevious((NODE *)pciPtr1))
        {
        pciPtr1 = (PCI_MEM_NODE *)lstPrevious((NODE *)pciPtr1); 
        if ((pciPtr1->pciMemBase + pciPtr1->pciMemSize) == (pciAddr + pciSize))
            {
            pciPtr1->pciMemSize += pciSize;
            i21555MemCleanup(lstPtr);
            return;
            }
        }

    /* Create a new node to save the freed address */
    pciPtr1 = (PCI_MEM_NODE *)lstFirst(lstPtr);
    pciPtr2 = (PCI_MEM_NODE *)memalign(sizeof(LLONG),sizeof(PCI_MEM_NODE));
    if (pciPtr2 != NULL)
        {
        pciPtr2->pciMemSize = pciSize;
        pciPtr2->pciMemBase = pciAddr;

        while (pciPtr1)
            {
            if (pciPtr1->pciMemBase >= (pciAddr + pciSize))
                {
                pciPtr3 = (PCI_MEM_NODE *)lstPrevious((NODE *)pciPtr1);
                lstInsert(lstPtr, (NODE *)pciPtr3, (NODE *)pciPtr2);
                i21555MemCleanup(lstPtr);
                return;
                }
            else
                if (lstNext((NODE *)pciPtr1))
                    pciPtr1 = (PCI_MEM_NODE *)lstNext((NODE *)pciPtr1);
                else
                    {
                    lstInsert(lstPtr, (NODE *)pciPtr1, (NODE *)pciPtr2);
                    i21555MemCleanup(lstPtr);
                    return;
                    }
            }
        }
    return;
    }
 
#ifdef INCLUDE_PCI_SHOW
/****************************************************************************
* printI21555Slave - Display the CompactPCI Slave mappings string
*
* This routine displays the current address mapping from the CompactPCI,
* through the PCI, and to the CPU address corresponding to each virtual
* CPU/PCI address.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555SlaveDisplay()
*
*/
LOCAL void printI21555Slave 
    (
    UINT CPCIAdr,
    UINT PCIAdr,
    UINT size,
    BOOL printHdr
    )
    {
    static char *spaceStr[4] = {"I/O", "MEMORY", "PREFETCHABLE MEMORY"};
    UINT spaceIndex, CPUAdr = 0;
          
    CPCIAdr &= REG_BAR_SIZE_MASK;  	  

    if ((size & 0x9) == 0x1)
        {
        spaceIndex=0;
        CPUAdr = 0xFFFFFFFF;
        }
    else
        if ((size & 0x9) == 0x8)
            {
	    spaceIndex=2;
	    if ((PCIAdr >= PCI2DRAM_BASE_ADRS) & (PCIAdr < (PCI2DRAM_BASE_ADRS + DRAM_SIZE)))
                CPUAdr = (PCIAdr & REG_BAR_SIZE_MASK) - PCI2DRAM_BASE_ADRS;
	    else 
	        CPUAdr = 0xFFFFFFFF;
            }
	else
	    {
	    spaceIndex=1;
	    if ((PCIAdr >= PCI2DRAM_BASE_ADRS) & (PCIAdr < (PCI2DRAM_BASE_ADRS + DRAM_SIZE)))
                CPUAdr = (PCIAdr & REG_BAR_SIZE_MASK) - PCI2DRAM_BASE_ADRS;
	    else 
	        CPUAdr = 0xFFFFFFFF;
	    }

	size = i21555SetupBARSize(size);

 	if (printHdr == TRUE)
	    {
            printf("\nCPCI SLAVE MAPPING OF CPCI MEMORY\n\n");
	    printf("CPCI BASE   PCI BASE   CPU BASE    SIZE (Hex)   SPACE\n");
	    printf("----------- ---------- ----------- ------------ -------------------\n");
	    }
	if (CPUAdr == 0xFFFFFFFF)
	    printf("   %8x   %8x         N/A     %8x %s\n", CPCIAdr, PCIAdr, size, spaceStr[spaceIndex]);
	else
	    printf("   %8x   %8x    %8x     %8x %s\n", CPCIAdr, PCIAdr, CPUAdr, size, spaceStr[spaceIndex]);
    }

/****************************************************************************
*
* printI21555Master - Display the CompactPCI Master mappings string
*
* This routine displays the current address mapping from the CPU, through the 
* PCI, and to the CompactPCI address corresponding to each virtual CPU/PCI
* address.
*
* RETURNS: N/A
*
* SEE ALSO: sysI21555SlaveDisplay()
*
*/
LOCAL void printI21555Master 
    (
    UINT CPCIAdr,
    UINT PCIAdr,
    UINT size,
    BOOL printHdr
    )
    {
    static char *spaceStr[3] = {"I/O", "MEMORY", "PREFETCHABLE MEMORY"};
    UINT spaceIndex, CPUAdr = 0;
      
    CPCIAdr &= REG_BAR_SIZE_MASK;  	  

    if (size & REG_SETUP_IO_SPACE)
        {
        spaceIndex=0;
        CPUAdr = PCI_IO_TO_CPU((PCIAdr & REG_BAR_SIZE_MASK));
        }
    else
        if (size & REG_SETUP_PREFETCH)
            {
	    spaceIndex=2;
            CPUAdr = PCI_PF_MEM_TO_CPU((PCIAdr & REG_BAR_SIZE_MASK));
            }
        else
            {
	    spaceIndex=1;
            CPUAdr = PCI_MEM_TO_CPU((PCIAdr & REG_BAR_SIZE_MASK));
            }

    size = i21555SetupBARSize(size);

    if (printHdr == TRUE)
        {
        printf("\nCPCI MASTER MAPPING OF CPCI MEMORY\n\n");
        printf("CPU BASE    PCI BASE   CPCI BASE   SIZE (Hex)   SPACE\n");
        printf("----------- ---------- ----------- ------------ -------------------\n");
        }
    printf("   %8x   %8x    %8x     %8x %s\n",CPUAdr, (PCIAdr & REG_BAR_SIZE_MASK), CPCIAdr, size, spaceStr[spaceIndex]);
    }

/****************************************************************************
* dumpI21555Hdr - Show the I21555 header
*
* This function displays the Primary, Secondary, and Device-Specific 
* Configuration header of the I21555 chip.  
*
* RETURNS: N/A
*
* SEE ALSO: dumpI21555Csr(), dumpI21555LookupTbl()
*
*/ 
void dumpI21555Hdr (void)
    {
    UINT control_reg;

    control_reg = i21555ReadConfig(CHIP_CONTROL);

    /* Disable target abort if to avoid machine checks */
    i21555WriteConfig(CHIP_CONTROL, (control_reg & 0xFFFFFFFE));

    printf("\nInterface Configuration 1\n");

    printf("    Device/Vendor ID (1): 0x%x\n", i21555ReadConfig(VEN_DEV_ID));
    printf("    Command/Status (1): 0x%x\n", i21555ReadConfig(COMMAND));
    printf("    Revision/Primary Class Code (1): 0x%x\n", i21555ReadConfig(REVISION));
    printf("    BiST/CLS/MLT/Headr Type (1): 0x%x\n", i21555ReadConfig(BIST));
    printf("    Primary/Secondary CSR Memory BAR (1): 0x%x\n", i21555ReadConfig(D_MEM0_BAR));
    printf("    Primary/Secondary CSR I/O BAR (1): 0x%x\n", i21555ReadConfig(P_CSR_IO_BAR));
    printf("    Downstream/Upstream I/O BAR (1): 0x%x\n", i21555ReadConfig(D_MEM1_BAR));
    printf("    Downstream Mem 2/Upstream Mem 1 BAR (1): 0x%x\n", i21555ReadConfig(D_MEM2_BAR));
    printf("    Downstream Mem 3/Upstream Mem 2 BAR (1): 0x%x\n", i21555ReadConfig(D_MEM3_BAR));
    printf("    Downstream Mem 3 BAR upper 32 bits (1): 0x%x\n", i21555ReadConfig(D_MEM3U_BAR));
    printf("    Subsystem/Subsystem Vendor ID (1): 0x%x\n", i21555ReadConfig(SUBSYSTEM_ID)); 

    printf("    Expansion ROM Base Address (1): 0x%x\n", i21555ReadConfig(P_EROM_BAR)); 
    printf("    Capabilities Pointer (1): 0x%x\n", i21555ReadConfig(ENC_CAP_PTR)); 
    printf("    Interrupt/Grant Control (1): 0x%x\n", i21555ReadConfig(INTERRUPT));

    printf("\nInterface Configuration 2\n");

    printf("    Device/Vendor ID (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(VEN_DEV_ID)));
    printf("    Command/Status (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(COMMAND)));
    printf("    Revision/Primary Class Code (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(REVISION)));
    printf("    BiST/CLS/MLT/Headr Type (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(BIST)));
    printf("    Primary/Secondary CSR Memory BAR (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(D_MEM0_BAR)));
    printf("    Primary/Secondary CSR I/O BAR (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(P_CSR_IO_BAR)));
    printf("    Downstream/Upstream I/O BAR (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(D_MEM1_BAR)));
    printf("    Downstream Mem 2/Upstream Mem 1 BAR (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(D_MEM2_BAR)));
    printf("    Downstream Mem 3/Upstream Mem 2 BAR (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(D_MEM3_BAR)));
    printf("    Downstream Mem 3 BAR upper 32 bits (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(D_MEM3U_BAR)));
    printf("    Subsystem/Subsystem Vendor ID (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(SUBSYSTEM_ID))); 
    printf("    Expansion ROM Base Address (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(P_EROM_BAR))); 
    printf("    Capabilities Pointer (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(ENC_CAP_PTR))); 
    printf("    Interrupt/Grant Control (2): 0x%x\n", i21555ReadConfig(CFG_HDR2(INTERRUPT)));

    printf("\nDevice-Specific Confguration\n");

    printf("    Downstream Configuration Address: 0x%x\n", i21555ReadConfig(D_CONF_ADDR));
    printf("    Downstream Configuration Data: 0x%x\n", i21555ReadConfig(D_CONF_DATA));
    printf("    Upstream Configuration Address: 0x%x\n", i21555ReadConfig(U_CONF_ADDR));
    printf("    Upstream Configuration Data: 0x%x\n", i21555ReadConfig(U_CONF_DATA));
    printf("    Configuration Control/Own Bits: 0x%x\n", i21555ReadConfig(CONFIG_OWN));
    printf("    Downstream Memory 0 Translated Base: 0x%x\n", i21555ReadConfig(D_MEM0_TRAN));
    printf("    Downstream IO/Memory 1 Translated Base: 0x%x\n", i21555ReadConfig(D_MEM1_TRAN));
    printf("    Downstream Memory 2 Translated Base: 0x%x\n", i21555ReadConfig(D_MEM2_TRAN));
    printf("    Downstream Memory 3 Translated Base: 0x%x\n", i21555ReadConfig(D_MEM3_TRAN));
    printf("    Upstream IO/Memory 0 Translated Base: 0x%x\n", i21555ReadConfig(U_MEM0_TRAN));
    printf("    Upstream Memory 1 Translated Base: 0x%x\n", i21555ReadConfig(U_MEM1_TRAN));
    printf("    Downstream Memory 0 Setup: 0x%x\n", i21555ReadConfig(D_MEM0_SETUP));
    printf("    Downstream IO/Memory 1 Setup: 0x%x\n", i21555ReadConfig(D_MEM1_SETUP));
    printf("    Downstream Memory 2 Setup: 0x%x\n", i21555ReadConfig(D_MEM2_SETUP));
    printf("    Downstream Memory 3 Setup: 0x%x\n", i21555ReadConfig(D_MEM3_SETUP));
    printf("    Downstream Upper 32 Bits Setup: 0x%x\n", i21555ReadConfig(D_MEM3U_SETUP));
    printf("    Primary Expansion ROM Setup: 0x%x\n", i21555ReadConfig(P_EROM_SETUP));
    printf("    Upstream IO/Memory 0 Setup: 0x%x\n", i21555ReadConfig(U_MEM0_SETUP));
    printf("    Upstream Memory 1 Setup: 0x%x\n", i21555ReadConfig(U_MEM1_SETUP));
    printf("    Chip Control: 0x%x\n", (i21555ReadConfig(CHIP_CONTROL) & 0xFFFFFFFE) | (CHIP_CONTROL & 0x00000001));
    printf("    Chip Status/Arbiter Control: 0x%x\n", i21555ReadConfig(CHIP_STATUS));
    printf("    Primary/Secondary SERR# Disables: 0x%x\n", i21555ReadConfig(SERR_DISABLE));
    printf("    Reset Control: 0x%x\n", i21555ReadConfig(RESET_CONTROL));
    printf("    Power Management Capabilities: 0x%x\n", i21555ReadConfig(POWER_CAP));
    printf("    Power Management Control: 0x%x\n", i21555ReadConfig(POWER_CTRL));
    printf("    VPD Address: 0x%x\n", i21555ReadConfig(VPD_SETUP));
    printf("    VPD Data: 0x%x\n", i21555ReadConfig(VPD_DATA));
    printf("    Hot Swap Control: 0x%x\n", i21555ReadConfig(HOT_SWAP));

    /* Restore the Master Abort Mode bit */
    i21555WriteConfig(CHIP_CONTROL, control_reg);
    }


/****************************************************************************
* dumpI21555LookupTbl - Show the I21555 Lookup Table
*
* This function displays the Secondary Interface Upstream Lookup Table
* entries of the I21555 chip. 
*
* RETURNS: N/A
*
* SEE ALSO: dumpI21555Hdr(), dumpI21555Csr()
*
*/ 
void dumpI21555LookupTbl (void)
    {
    I21555_CSR *i21555Csr;
    int is_io_space;
    int i;
    UINT tVal;

    is_io_space = i21555ReadConfig(P_CSR_IO_BAR) & 0x1;
    i21555Csr = i21555GetCsr();

    if (is_io_space)
        for (i = 0; i < LTABLE_SIZE; i++)
            {
	    /* Write the offset register */
            i21555Csr->tableOffset = (i21555Csr->tableOffset & PSWAP(0xFFFFFF00)) | PSWAP((i*4));
	  
            tVal = PSWAP(i21555Csr->tableData);

            printf("    Lookup Table %d : 0x%x\n", i, tVal);
	    }
	else
	    for (i = 0; i < LTABLE_SIZE; i++)
                printf("    Lookup Table %d : 0x%x\n", i, PSWAP(i21555Csr->upMem2Tbl[i]));
    }

/****************************************************************************
* dumpI21555Csr - Show the I21555 CSRs
*
* This function displays the CSR registers of the I21555 chip.
*
* RETURNS: N/A
*
* SEE ALSO: dumpI21555Hdr(), dumpI21555LookupTbl()
*
*/ 
void dumpI21555Csr (void)
    {
    int is_io_space;
    I21555_CSR *i21555Csr;
    UINT control_reg;

    i21555Csr = i21555GetCsr();

    control_reg = i21555ReadConfig(CHIP_CONTROL);

    /* Disable target abort if to avoid machine checks */
    i21555WriteConfig(CHIP_CONTROL, (control_reg & 0xFFFFFFFE));

    is_io_space = i21555ReadConfig(P_CSR_IO_BAR) & 1;

    printf("    Downstream Configuration Address : 0x%x\n", PSWAP(i21555Csr->downConfigAdr));
    printf("    Downstream Configuration Data : 0x%x\n", PSWAP(i21555Csr->downConfigDat));
    printf("    Downstream Configuration Address : 0x%x\n", PSWAP(i21555Csr->upConfigAdr));
    printf("    Upstream Configuration Data : 0x%x\n", PSWAP(i21555Csr->upConfigDat));
    printf("    Configuration Control/Own Bits : 0x%x\n", PSWAP(i21555Csr->configOwn));
    printf("    Downstream I/O Address : 0x%x\n", PSWAP(i21555Csr->downIOAdr));
    printf("    Downstream I/O Data : 0x%x\n", PSWAP(i21555Csr->downIODat));
    printf("    Downstream I/O Address : 0x%x\n", PSWAP(i21555Csr->upIOAdr));
    printf("    Downstream I/O Data : 0x%x\n", PSWAP(i21555Csr->upIODat));
    printf("    I/O Control/Status/Own Bits : 0x%x\n", PSWAP(i21555Csr->IOOwn));
    printf("    Lookup Table Offset : 0x%x\n", PSWAP(i21555Csr->tableOffset));
    printf("    Lookup Table Data : 0x%x\n", PSWAP(i21555Csr->tableData));
    printf("    I2O Outbound Post_List Status : 0x%x\n", PSWAP(i21555Csr->i2oOPS));
    printf("    I2O Outbound Post_List Interrupt Mask : 0x%x\n", PSWAP(i21555Csr->i2oOPM));
    printf("    I2O Inbound Post_List Status : 0x%x\n", PSWAP(i21555Csr->i2oIPS));
    printf("    I2O Inbound Post_List Interrupt Mask : 0x%x\n", PSWAP(i21555Csr->i2oIPM));
    printf("    I2O Inbound Queue : 0x%x\n", PSWAP(i21555Csr->i2oIQ));
    printf("    I2O Outbound Queue : 0x%x\n", PSWAP(i21555Csr->i2oOQ));
    printf("    I2O Inbound Free_List Head Pointer : 0x%x\n", PSWAP(i21555Csr->i2oIFHP));
    printf("    I2O Inbound Post_List Head Pointer : 0x%x\n", PSWAP(i21555Csr->i2oIPTP));
    printf("    I2O Outbound Free_List Tail Pointer : 0x%x\n", PSWAP(i21555Csr->i2oOFTP));
    printf("    I2O Outbound Post_List Head Pointer : 0x%x\n", PSWAP(i21555Csr->i2oOPHP));
    printf("    I2O Inbound Post_List Counter : 0x%x\n", PSWAP(i21555Csr->i2oIPC));
    printf("    I2O Inbound Free_List Counter : 0x%x\n", PSWAP(i21555Csr->i2oIFC));
    printf("    I2O Outbound Post_List Counter : 0x%x\n", PSWAP(i21555Csr->i2oOPC));
    printf("    I2O Outbound Free_List Counter : 0x%x\n", PSWAP(i21555Csr->i2oOFC));
    printf("    Downstream Memory 0 Translated Base : 0x%x\n", PSWAP(i21555Csr->downMem0Tran));
    printf("    Downstream IO/Memory 1 Translated Base : 0x%x\n", PSWAP(i21555Csr->downMem1Tran));
    printf("    Downstream Memory 2 Translated Base : 0x%x\n", PSWAP(i21555Csr->downMem2Tran));
    printf("    Downstream Memory 3 Translated Base : 0x%x\n", PSWAP(i21555Csr->downMem3Tran));
    printf("    Upstream IO/Memory 0 Translated Base : 0x%x\n", PSWAP(i21555Csr->upMem0Tran));
    printf("    Upstream Memory 1 Translated Base : 0x%x\n", PSWAP(i21555Csr->upMem1Tran));
    printf("    Chip Status CSR : 0x%x\n", PSWAP(i21555Csr->chipStatus));
    printf("    Chip Clear/Set IRQ Mask : 0x%x\n", PSWAP(i21555Csr->chipIRQMask));
    printf("    Upstream Page Boundary IRQ 0 : 0x%x\n", PSWAP(i21555Csr->upPageBoundIRQ0));
    printf("    Upstream Page Boundary IRQ 1 : 0x%x\n", PSWAP(i21555Csr->upPageBoundIRQ1));
    printf("    Upstream Page Boundary IRQ Mask 0 : 0x%x\n", PSWAP(i21555Csr->upPageBoundMsk0));
    printf("    Upstream Page Boundary IRQ Mask 1 : 0x%x\n", PSWAP(i21555Csr->upPageBoundMsk1));
    printf("    Primary/Secondary Clear IRQ : 0x%x\n", PSWAP(i21555Csr->clrIRQ));
    printf("    Primary/Secondary Set IRQ : 0x%x\n", PSWAP(i21555Csr->setIRQ));
    printf("    Primary/Secondary Clear IRQ mask : 0x%x\n", PSWAP(i21555Csr->clrIRQMask));
    printf("    Primary/Secondary Set IRQ mask : 0x%x\n", PSWAP(i21555Csr->setIRQMask));
    printf("    Scratchpad 0 : 0x%x\n", PSWAP(i21555Csr->scratchpad0));
    printf("    Scratchpad 1 : 0x%x\n", PSWAP(i21555Csr->scratchpad1));
    printf("    Scratchpad 2 : 0x%x\n", PSWAP(i21555Csr->scratchpad2));
    printf("    Scratchpad 3 : 0x%x\n", PSWAP(i21555Csr->scratchpad3));
    printf("    Scratchpad 4 : 0x%x\n", PSWAP(i21555Csr->scratchpad5));
    printf("    Scratchpad 6 : 0x%x\n", PSWAP(i21555Csr->scratchpad6));
    printf("    Scratchpad 7 : 0x%x\n", PSWAP(i21555Csr->scratchpad7));
    printf("    ROM Setup : 0x%x\n", PSWAP(i21555Csr->romSetup));
    printf("    ROM Address : 0x%x\n", PSWAP(i21555Csr->romCtrl));

    if (!is_io_space)
        dumpI21555LookupTbl();

    /* Restore the Master Abort Mode bit */
    i21555WriteConfig(CHIP_CONTROL, control_reg);
    }
#endif
