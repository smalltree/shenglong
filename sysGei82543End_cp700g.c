/* sysGei82543End.c - Intel 82543GC END driver support routines*/

/* Copyright 2002 Wind River Systems, Inc.  */

#include "copyright_wrs.h"

/*
modification history
--------------------
01b,21nov02,zmm  Cleanups after code review.
01a,23sep02,zmm  Written.
*/

/*
Currently, this module only supports the Intel Gigabit 83542GC Ethernet Adapter. 

SEE ALSO: ifLib,
.I "Intel 82543 User's Manual,"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "config.h"
#include "endLib.h"

#include "vmLib.h"
#include "drv/pci/pciIntLib.h"
#include "cp7000g.h"

#include "drv/end/gei82543End.h"

#undef I82543_DEBUG
#ifdef I82543_DEBUG
#   undef    LOCAL
#   define    LOCAL
#endif    /* I82543_DEBUG */

/* Intel PRO1000F LAN Adapter type */

#define UNKNOWN                     -1
#define GEI_MAX_UNITS               3

#define KB_128                      0x20000
#define KB_64                       0x10000
#define GEI0_MEMBASE0_LOW           PCI0_MEM0_ADRS  /* memory base for CSR */
#define GEI0_MEMBASE0_HIGH          0x00000000      /* memory base for CSR */
#define GEI0_MEMSIZE0               KB_128          /* memory size for CSR, 128KB */
#define GEI0_MEMBASE1               0xfd100000      /* memory base for Flash */
#define GEI0_MEMSIZE1               0x00100000      /* memory size for Flash, 1MB */
#define GEI0_INT_LVL                IV_GEI1_VEC

#define GEI1_MEMBASE0_LOW           (GEI0_MEMBASE0_LOW+2*GEI0_MEMSIZE0) /* mem base */
#define GEI1_MEMBASE0_HIGH          0x00000000      /* memory base for CSR */
#define GEI1_MEMSIZE0               KB_128          /* memory size for CSR, 128KB */
#define GEI1_MEMBASE1               0xfd100000      /* memory Base for Flash */
#define GEI1_MEMSIZE1               0x00100000      /* memory size for Flash, 1MB */
#define GEI1_INT_LVL                IV_GEI2_VEC

/* defines for Alaska PHY */

#define ALASKA_PHY_SPEC_CTRL_REG        0x10
#define ALASKA_PHY_SPEC_STAT_REG        0x11
#define ALASKA_INT_ENABLE_REG           0x12
#define ALASKA_INT_STATUS_REG           0x13
#define ALASKA_EXT_PHY_SPEC_CTRL_REG    0x14
#define ALASKA_RX_ERROR_COUNTER         0x15
#define ALASKA_LED_CTRL_REG             0x18
#define ALASKA_PSCR_ASSERT_CRS_ON_TX    0x0800
#define ALASKA_EPSCR_TX_CLK_25          0x0070
#define ALASKA_PSCR_AUTO_X_1000T        0x0040
#define ALASKA_PSCR_AUTO_X_MODE         0x0060
#define ALASKA_PSSR_DPLX                0x2000
#define ALASKA_PSSR_SPEED               0xc000
#define ALASKA_PSSR_10MBS               0x0000
#define ALASKA_PSSR_100MBS              0x4000
#define ALASKA_PSSR_1000MBS             0x8000
#define ALASKA_PHY_DEF_ADDR             0x1

/* typedefs */

typedef struct geiResource        /* GEI_RESOURCE */
    {
    UINT32 memBaseLow;            /* Base Address LOW */
    UINT32 memBaseHigh;           /* Base Address HIGH */
    UINT32 flashBase;             /* Base Address for FLASH */
    char   irq;                   /* Interrupt Request Level */
    BOOL   adr64;
    int    boardType;             /* type of LAN board this unit is */
    int    pciBus;                /* PCI Bus number */
    int    pciDevice;             /* PCI Device number */
    int    pciFunc;               /* PCI Function number */

    UINT   memLength;             /* required memory size */
    } GEI_RESOURCE;

/* locals */

LOCAL UINT32 geiUnits = 0 ;     /* number of GEIs we found */

LOCAL GEI_RESOURCE geiResources [GEI_MAX_UNITS] =
    {
    {GEI0_MEMBASE0_LOW, GEI0_MEMBASE0_HIGH, GEI0_MEMBASE1, GEI0_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, GEI0_MEMSIZE0},
    {GEI1_MEMBASE0_LOW, GEI1_MEMBASE0_HIGH, GEI1_MEMBASE1, GEI1_INT_LVL, 
     UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, GEI1_MEMSIZE0},
     };

LOCAL int       sysGei82543IntEnable  (int unit);
LOCAL int       sysGei82543IntDisable (int unit);
LOCAL int       sysGei82543IntAck     (int unit);

#ifdef INCLUDE_PCI
/*****************************************************************************
*
* sysGei82543PciInit - prepare LAN adapter for 82543 initialization
*
* This routine find out the PCI device, and map its memory and IO address.
* It must be done prior to initializing the 82543, sysGei82543Init().  Also
* must be done prior to MMU initialization, usrMmuInit().
*
* RETURNS: N/A
*/

STATUS sysGei82543PciInit 
    (
    )
    {
    GEI_RESOURCE *pReso;
    int pciBus;
    int pciDevice;
    int pciFunc;
    int unit;
    int pro1000DevUnit =0;
    BOOL duplicate;
    UINT32 bar0;
    UINT32 memBaseLow;
    UINT32 memBaseHigh;
    UINT32 flashBase;
    UINT16 boardId =0;    
    int    ix;
	
#if 0 /*wangfq*/
    pciConfigOutLong (0, 1, 0, PCI_CFG_BASE_ADDRESS_0, GEI0_MEMBASE0_LOW); 
    pciConfigOutLong (0, 1, 0, PCI_CFG_BASE_ADDRESS_1, GEI0_MEMBASE0_LOW + GEI0_MEMSIZE0);
    pciConfigOutLong (0, 2, 0, PCI_CFG_BASE_ADDRESS_0, GEI1_MEMBASE0_LOW);
    pciConfigOutLong (0, 2, 0, PCI_CFG_BASE_ADDRESS_1, GEI1_MEMBASE0_LOW + GEI1_MEMSIZE0);
#endif

    for (unit = 0; unit < GEI_MAX_UNITS; unit++)
        {      
        boardId = UNKNOWN;

        if (pciFindDevice (PRO1000_PCI_VENDOR_ID, PRO1000_PCI_DEVICE_ID, 
                                pro1000DevUnit, &pciBus, &pciDevice, 
                                &pciFunc) == OK)
            {  
            pro1000DevUnit++;
            boardId = /*PRO1000T_BOARD*/PRO1000_546_BOARD; /*wangfq*/
            }
        else  
            break;  
               
        /* check the duplicate */

        pReso     = &geiResources [0];
        duplicate = FALSE;
        for (ix = 0; ix < GEI_MAX_UNITS; ix++, pReso++)
            {
            if ((ix != unit) && (pReso->pciBus == pciBus) &&
                (pReso->pciDevice == pciDevice) && (pReso->pciFunc == pciFunc))
                duplicate = TRUE;
            }
        if (duplicate) 
            continue;

        /* we found the right one */

        pReso = &geiResources [unit];
        pReso->boardType = boardId;
        pReso->pciBus    = pciBus;
        pReso->pciDevice = pciDevice;
        pReso->pciFunc   = pciFunc;

        pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_BASE_ADDRESS_0, &bar0);
        
        pReso->adr64=( (bar0 & BAR0_64_BIT) == BAR0_64_BIT)? TRUE : FALSE;        
        /* get memory base address and IO base address */

        pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                          PCI_CFG_BASE_ADDRESS_0, &memBaseLow);
    if (pReso->adr64)
            {        
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_1, &memBaseHigh);
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_2, &flashBase);
            }
        else
            {
            pciConfigInLong (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                              PCI_CFG_BASE_ADDRESS_1, &flashBase);
            memBaseHigh = 0x0;
            }
     
        memBaseLow &= ~0x1ffff;
        flashBase  &= ~0xffff;

        /* over write the resource table with read value */

        pReso->memBaseLow   = memBaseLow | 0xa0000000; /*wangfq*/
        pReso->memBaseHigh  = memBaseHigh| 0xa0000000; /*wangfq*/
        pReso->flashBase    = flashBase| 0xa0000000; /*wangfq*/

        /* enable mapped memory and IO addresses */

        pciConfigOutWord (pReso->pciBus, pReso->pciDevice, pReso->pciFunc,
                           PCI_CFG_COMMAND, PCI_CMD_IO_ENABLE |
                           PCI_CMD_MEM_ENABLE | PCI_CMD_MASTER_ENABLE);
        geiUnits++;
        }

    return OK;
    }

#define LOCAL_GEI_READ_REG(offset,result)     \
        do {                         \
           UINT32 temp;              \
           temp = ((*(volatile UINT32 *)(devRegBase + (offset)))); \
           result = PSWAP(temp); /* swap the data */  \
           } while (0)

#define LOCAL_GEI_WRITE_REG(offset, value) \
        ((*(volatile UINT32 *)(devRegBase + (offset))) = \
        (UINT32) PSWAP(value))

/*************************************************************************
*
* eepromReadBits - Read bits from EEPROM
*
* This routine read data in from EEPROM
*
* RETURNS: value in the WORD size
*/

LOCAL UINT16 eepromReadBits
    (
    UINT32       devRegBase,
    int          bitsNum
    )
    {
    UINT32   ix;
    UINT16   val = 0;
    UINT16   tmp;

    for (ix = 0; ix < bitsNum; ix++)
        {
        /* raise the clk */ 

        LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, EECD_CS_BIT | EECD_SK_BIT);

	sysDelay(); sysDelay(); sysDelay();

	LOCAL_GEI_READ_REG(INTEL_82543GC_EECD, tmp);
        val = ( val << 1) | ((tmp & EECD_DO_BIT) ? 1 : 0);

        /* lower the clk */

        LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, EECD_CS_BIT);

	sysDelay(); sysDelay(); sysDelay();
        }      

    return (val);
    }

/*************************************************************************
*
* eepromWriteBits - write bits out to EEPROM
*
* This routine writes bits out to eeprom  
*
* RETURNS: N/A
*/

LOCAL void eepromWriteBits
    (
    UINT32       devRegBase,
    UINT16       value,
    UINT16       bitNum
    )
    {
    volatile UINT16 data;

    if (bitNum == 0)
           return;

    while (bitNum--)
    {
    data = (value & (0x1 << bitNum )) ? EECD_DI_BIT : 0;

    data |=  EECD_CS_BIT;

    /* write the data */

    LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, data);

    sysDelay(); sysDelay();

    /* raise the clk */ 

    LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, data | EECD_SK_BIT);

    sysDelay(); sysDelay();

    /* lower the clk */

    LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, data);

    sysDelay(); sysDelay();
    }

    return;
    }

/*************************************************************************
*
* eepromReadWord - Read a Word from EEPROM
*
* RETURNS: value in WORD size
*/

LOCAL UINT16 eepromReadWord
    (
    UINT32       devRegBase,
    UINT32       index
    )
    {
    UINT16 val;
    UINT32 tmp;

    if (index >= EEPROM_WORD_SIZE)
        {
        return 0;
        }

    LOCAL_GEI_READ_REG(INTEL_82543GC_EECD, tmp);

    LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, EECD_CS_BIT);

    sysDelay(); sysDelay(); sysDelay();

    /* write the opcode out */

    eepromWriteBits (devRegBase, EEPROM_READ_OPCODE, EEPROM_CMD_BITS);

    /* write the index out */

    eepromWriteBits (devRegBase, index, EEPROM_INDEX_BITS);

    /* read the data */

    val = eepromReadBits (devRegBase, EEPROM_DATA_BITS);

    /* clean up access to EEPROM */      

    tmp &= ~(EECD_DI_BIT | EECD_DO_BIT | EECD_CS_BIT);

    LOCAL_GEI_WRITE_REG(INTEL_82543GC_EECD, tmp);

    return val;
    }

/*************************************************************************
*
* sysGei82543PhyReset - Reset the attached PHY
*
* This routine reset the PHY via reset bit in Control Reister  
*/
void sysGei82543PhyReset(PHY_INFO * info, UINT8 phyAddr)
{
    info->phyWriteRtn (info->pDrvCtrl, phyAddr, 
                      MII_CTRL_REG, MII_CR_RESET);
}


/*************************************************************************
*
* sysGei82543PhyInit - initialize the attached PHY
*
* This routine configures the PHY for operation
*
* RETURNS: N/A
*/
void sysGei82543PhyInit(PHY_INFO* info, UINT8 phyAddr)
{
    UINT16 tmp;

    /* disable all interrupts in ALASKA PHY */

    info->phyReadRtn (info->pDrvCtrl, phyAddr,
                     ALASKA_INT_ENABLE_REG, &tmp);
    tmp = 0;

    info->phyWriteRtn (info->pDrvCtrl, phyAddr,
                      ALASKA_INT_ENABLE_REG, tmp);

    /* CRS assert on transmit */

    info->phyReadRtn (info->pDrvCtrl, phyAddr,
                     ALASKA_PHY_SPEC_CTRL_REG, &tmp);

    tmp |= ALASKA_PSCR_ASSERT_CRS_ON_TX;

    info->phyWriteRtn (info->pDrvCtrl, phyAddr,
                      ALASKA_PHY_SPEC_CTRL_REG, tmp);

    /* set the clock rate when operate in 1000T mode */

    info->phyReadRtn (info->pDrvCtrl, phyAddr,
                     ALASKA_EXT_PHY_SPEC_CTRL_REG, &tmp);


    tmp |= ALASKA_EPSCR_TX_CLK_25;

    info->phyWriteRtn (info->pDrvCtrl, phyAddr,
                      ALASKA_EXT_PHY_SPEC_CTRL_REG,tmp);

    /* set the LED mode */

    info->phyReadRtn (info->pDrvCtrl, phyAddr,
                     ALASKA_LED_CTRL_REG, &tmp);

    tmp |= 0x8;

    info->phyWriteRtn (info->pDrvCtrl, phyAddr,
                      ALASKA_LED_CTRL_REG,tmp);
}

/*****************************************************************************
*
* sys82543BoardInit - prepare LAN adapter for 82543 initialization
*
* This routine is expected to perform any adapter-specific or target-specific
* initialization that must be done prior to initializing the 82543.
*
* The 82543 driver calls this routine from the driver attach routine before
* any other routines in this library.
*
* RETURNS: OK or ERROR if the adapter could not be prepared for initialization.
*/

STATUS sys82543BoardInit
    (
    int    unit,                    /* unit number */
    ADAPTOR_INFO  *pBoard       /* board information for the GEI driver */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];
    UINT16 tmp;

    if (unit >= geiUnits)
        return (ERROR);

    if (pReso->boardType !=  PRO1000F_BOARD && 
        pReso->boardType !=  PRO1000T_BOARD)
           return ERROR;

    /* initializes the board information structure */

    pBoard->boardType     = pReso->boardType;
    pBoard->vector        = pReso->irq;
    pBoard->phyType       = GEI_PHY_GMII_TYPE;
    pBoard->phyBspPreInit = sysGei82543PhyReset; 
    pBoard->phySpecInit   = sysGei82543PhyInit;
    pBoard->phyAddr       = ALASKA_PHY_DEF_ADDR; 
    pBoard->regBaseLow    = pReso->memBaseLow;
    pBoard->regBaseHigh   = pReso->memBaseHigh;
    pBoard->flashBase     = pReso->flashBase;
    pBoard->adr64         = pReso->adr64;

    pBoard->intEnable     = sysGei82543IntEnable;
    pBoard->intDisable    = sysGei82543IntDisable;
    pBoard->intAck        = sysGei82543IntAck;
    pBoard->delayFunc     = (FUNCPTR) sysDelay;
    pBoard->delayUnit     = 250;    /* sysDelay() takes about 720ns delay */
    pBoard->sysLocalToBus = NULL; 
    pBoard->sysBusToLocal = NULL; 

    /* specify the interrupt connect/disconnect routines to be used */

    pBoard->intConnect    = (FUNCPTR) intConnect; 
    pBoard->intDisConnect = NULL;

    /* get ethernet address */

    tmp = eepromReadWord(pBoard->regBaseLow, 0);
    pBoard->enetAddr[0] = tmp & 0xff;
    pBoard->enetAddr[1] = (tmp>>8) & 0xff;
    tmp = eepromReadWord(pBoard->regBaseLow, 1);
    pBoard->enetAddr[2] = tmp & 0xff;
    pBoard->enetAddr[3] = (tmp>>8) & 0xff;
    tmp = eepromReadWord(pBoard->regBaseLow, 2);
    pBoard->enetAddr[4] = tmp & 0xff;
    pBoard->enetAddr[5] = (tmp>>8) & 0xff;

    /* get init control words */

    pBoard->eeprom_icw1 = eepromReadWord(pBoard->regBaseLow, 0xA);
    pBoard->eeprom_icw2 = eepromReadWord(pBoard->regBaseLow, 0xF);

    return (OK);
    }

/*****************************************************************************
*
* sysGei82543IntAck - acknowledge an 82543 interrupt
*
* This routine performs any 82543 interrupt acknowledge that may be
* required.  This typically involves an operation to some interrupt
* control hardware.
*
* This routine gets called from the 82543 driver's interrupt handler.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if the interrupt could not be acknowledged.
*/

LOCAL STATUS sysGei82543IntAck
    (
    int    unit        /* unit number */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];

    switch (pReso->boardType)
    {
    case TYPE_PRO1000F_PCI:        /* handle PRO1000F LAN Adapter */

        /* no addition work necessary for the PRO1000F */

        break;

    default:
        return (ERROR);
    }

    return (OK);
    }

/*****************************************************************************
*
* sysGei82543IntEnable - enable 82543 interrupts
*
* This routine enables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to terminate
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be enabled.
*/

LOCAL STATUS sysGei82543IntEnable
    (
    int    unit        /* unit number */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];

    switch (pReso->boardType)
    {
    case TYPE_PRO1000F_PCI:        /* handle PRO1000F LAN Adapter */

        /* no addition work necessary for the PRO1000F */

        break;

    default:
        return (ERROR);
    }

    return (OK);
    }

/*****************************************************************************
*
* sysGei82543IntDisable - disable 82543 interrupts
*
* This routine disables 82543 interrupts.  This may involve operations on
* interrupt control hardware.
*
* The 82543 driver calls this routine throughout normal operation to enter
* critical sections of code.
*
* This routine assumes that the PCI configuration information has already
* been setup.
*
* RETURNS: OK, or ERROR if interrupts could not be disabled.
*/

LOCAL STATUS sysGei82543IntDisable
    (
    int    unit        /* unit number */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];

    switch (pReso->boardType)
    {
    case TYPE_PRO1000F_PCI:        /* handle PRO1000F LAN Adapter */

        /* no addition work necessary for the PRO1000F */

        break;

    default:
        return (ERROR);
    }

    return (OK);
    }

/*****************************************************************************
*
* sysGei82543Show - shows 82543 configuration 
*
* this routine shows (Intel Pro 1000F) configuration 
*
* RETURNS: N/A
*/

void sysGei82543Show
    (
    int    unit        /* unit number */
    )
    {
    GEI_RESOURCE *pReso = &geiResources [unit];

    if (unit >= geiUnits)
        {
        printf ("invalid unit number: %d\n", unit);
        return;
        }

    printf ("\n\tCSR mem base address = %x \
             \n\tFlash mem base address = %x \
             \n\tPCI bus no. = %x \
             \n\tdevice no. = %x \
             \n\tfunction no. = %x \
             \n\tIRQ = %d\n",
             pReso->memBaseLow, pReso->flashBase, pReso->pciBus, 
             pReso->pciDevice, pReso->pciFunc, pReso->irq);    

    return;
    }
#endif /* INCLUDE_PCI */

