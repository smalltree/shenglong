/* sysSerial.c - serial device initialization */

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
01a,12sep02,zmm  Written.
*/

#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "Lsn2eCpciSBC.h"
#include "config.h"
#include "sysLib.h"
#include "drv/sio/ns16552Sio.h"

NS16550_CHAN ns16550Chan[NUM_TTY];

/******************************************************************************
*
* sysSerialHwInit - initialize the BSP serial devices to a quiescent state
*
* This routine initializes the BSP serial device descriptors and puts the
* devices in a quiescent state.  It is called from sysHwInit() with
* interrupts locked.
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit()
*/ 

void sysSerialHwInit (void)
    {
    /* intialize the chips device descriptors */

    ns16550Chan[0].regs 	= (UINT8 *)(COM1_BASE_ADDR)/*(K1BASE + BONITO_PCIIO_BASE + COM1_BASE_ADDR)*/;
    ns16550Chan[0].level	= IV_VIA686_COM1_OFFSET;
    ns16550Chan[0].regDelta = UART_REG_ADDR_INTERVAL;
    ns16550Chan[0].baudRate = CONSOLE_BAUD_RATE;
    ns16550Chan[0].xtal		= UART_CLK_RATE;
	
#if 0
	/* intialize the chips device descriptors */
    ns16550Chan[1].regs = (UINT8 *) (COM2_BASE_ADDR);
    ns16550Chan[1].level	= IV_VIA686_COM2_OFFSET;
    ns16550Chan[1].regDelta = UART_REG_ADDR_INTERVAL;
    ns16550Chan[1].baudRate = CONSOLE_BAUD_RATE;
    ns16550Chan[1].xtal = UART_CLK_RATE;

	/* intialize the chips device descriptors */
    ns16550Chan[2].regs = (UINT8 *) (COM3_BASE_ADDR);
    ns16550Chan[2].level	= IV_VIA686_COM2_OFFSET;
    ns16550Chan[2].regDelta = UART_REG_ADDR_INTERVAL;
    ns16550Chan[2].baudRate = CONSOLE_BAUD_RATE;
    ns16550Chan[2].xtal = UART_CLK_RATE;

	/* intialize the chips device descriptors */
    ns16550Chan[3].regs = (UINT8 *) (COM4_BASE_ADDR);
    ns16550Chan[3].level	= IV_VIA686_COM2_OFFSET;
    ns16550Chan[3].regDelta = UART_REG_ADDR_INTERVAL;
    ns16550Chan[3].baudRate = CONSOLE_BAUD_RATE;
    ns16550Chan[3].xtal = UART_CLK_RATE;
#endif
       /* reset the chips */
    
    ns16550DevInit (&ns16550Chan[0]);  /* uart int. disabled */
#if 0
    ns16550DevInit (&ns16550Chan[1]);  /* uart int. disabled */

    ns16550DevInit (&ns16550Chan[2]);  /* uart int. disabled */

    ns16550DevInit (&ns16550Chan[3]);  /* uart int. disabled */
#endif
    }

/******************************************************************************
*
* sysSerialHwInit2 - connect BSP serial device interrupts
*
* This routine connects the BSP serial device interrupts.  It is called from
* sysHwInit2().  
* 
* Serial device interrupts cannot be connected in sysSerialHwInit() because
* the kernel memory allocator was not initialized at that point, and
* intConnect() calls malloc().
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit2()
*/ 

void sysSerialHwInit2 (void)
    {

    	(void) intConnect (INUM_TO_IVEC (IV_COM1_VEC), ns16550Int, (int)sysSerialChanGet(0));
		sysIntEnablePIC(IV_VIA686_COM1_OFFSET);	
#if 0
	(void) intConnect (INUM_TO_IVEC (IV_COM2_VEC), ns16550Int, (int)sysSerialChanGet(1));
		sysIntEnablePIC(IV_VIA686_COM2_OFFSET);

	(void) intConnect (INUM_TO_IVEC (IV_COM3_VEC), ns16550Int, (int)sysSerialChanGet(2));
		sysIntEnablePIC(IV_VIA686_COM2_OFFSET);

	(void) intConnect (INUM_TO_IVEC (IV_COM4_VEC), ns16550Int, (int)sysSerialChanGet(3));
		sysIntEnablePIC(IV_VIA686_COM2_OFFSET);
		/*tmp=read_c0_status;
		write_c0_status(tmp|0x400);*/
#endif
    }

/******************************************************************************
*
* sysSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* This routine gets the SIO_CHAN device associated with a specified serial
* channel.
*
* RETURNS: A pointer to the SIO_CHAN structure for the channel, or ERROR
* if the channel is invalid.
*/ 

SIO_CHAN * sysSerialChanGet
    (
    int channel		/* serial channel */
    )
    {
    switch (channel)
	{
	case 0:
	    return ((SIO_CHAN *)&ns16550Chan[0]);

	case 1:
		return ((SIO_CHAN *)&ns16550Chan[1]);

	case 2:
		return ((SIO_CHAN *)&ns16550Chan[2]);

	case 3:
		return ((SIO_CHAN *)&ns16550Chan[3]);
		
	default:
	    return NULL;
	}
    }

