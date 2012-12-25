/*
	Loongson2F_7010A  uart Driver
*/

#include <vxWorks.h>
#include <iv.h>
#include <intLib.h>
#include <stdlib.h>
#include <memLib.h>
#include <ioLib.h>
#include <iosLib.h>
#include <string.h>
#include <semLib.h>
#include "wdLib.h" 
#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "errnoLib.h"
#include "errno.h"
#include "Lsn2eCpciSBC.h"
#include "config.h"
#include "sysLib.h"
#include "uart.h"

LOCAL int uartDrvNum;		/* driver number assigned to this driver */
/* char   inchar; */

LOCAL int uartOpen(UART_DEV *pChan, char *name, int mode);
LOCAL int uartRead(UART_DEV *pUartDv,char *buffer, int nbytes);
LOCAL int uartWrite(UART_DEV *pUartDv,char *buffer, int nbytes);
LOCAL int uartIoctl(UART_DEV *pUartDv,int command, int arg);

void uartDevInit(UART_DEV *pChan);
LOCAL void uartInitChannel(UART_DEV *pChan);
void uartInt( UART_DEV * pChan );
LOCAL STATUS  uartBaudSet(UART_DEV * pChan, UINT baud);
LOCAL STATUS uartOptsSet(UART_DEV * pChan, UINT options	);
    
UART_DEV  uartChan[UART_NUM];

#if 0
unsigned char uartPool[UART_NUM][UART_POOL_SIZE];
unsigned int uartLen[UART_NUM];
/* add by sunchao */
/*
extern SEM_ID	uartSem[UART_NUM];
extern WDOG_ID	uartWdog[UART_NUM];
extern unsigned char uartFrameFlag[UART_NUM];
extern unsigned char uartSemGiveFlag[UART_NUM];
extern unsigned char *uartRegBase[UART_NUM];
*/
SEM_ID	uartSem[UART_NUM];
unsigned char uartSemGiveFlag[UART_NUM];
WDOG_ID	uartWdog[UART_NUM];
unsigned char uartFrameFlag[UART_NUM];
unsigned int uartFrameTime[UART_NUM];
#endif
/*******************************************************************************
*
*/
void sysUartHwInit (void)
{
	uartChan[0].unit = 0;
	uartChan[0].regs = (UINT8 *) (COM2_BASE_ADDR);
	uartChan[0].intLevel = IV_VIA686_COM2_OFFSET;
	uartChan[0].baudRate = CONSOLE_BAUD_RATE;
	uartChan[0].xtal = UART_CLK_RATE;
	uartChan[0].len = 0;
	uartChan[0].frameFlag = TRUE;
	uartChan[0].semGiveFlag = FALSE;

#if(UART_NUM > 1)
	uartChan[1].unit = 1;
	uartChan[1].regs = (UINT8 *) (COM3_BASE_ADDR);
	uartChan[1].intLevel = IV_VIA686_COM3_OFFSET;
	uartChan[1].baudRate = CONSOLE_BAUD_RATE;
	uartChan[1].xtal = UART_CLK_RATE;
	uartChan[1].len = 0;
	uartChan[1].frameFlag = TRUE;
	uartChan[1].semGiveFlag = FALSE;
#if(UART_NUM > 2)
	uartChan[2].unit = 2;
	uartChan[2].regs = (UINT8 *) (COM4_BASE_ADDR);
	uartChan[2].intLevel = IV_VIA686_COM4_OFFSET;
	uartChan[2].baudRate = CONSOLE_BAUD_RATE;
	uartChan[2].xtal = UART_CLK_RATE;
	uartChan[2].len = 0;
	uartChan[2].frameFlag = TRUE;
	uartChan[2].semGiveFlag = FALSE;
#endif
#endif

	uartDevInit(&uartChan[0]);
#if(UART_NUM > 1)
	uartDevInit(&uartChan[1]);
#if(UART_NUM > 2)
	uartDevInit(&uartChan[2]);
#endif
#endif
}

/*******************************************************************************
*
*/
void sysUartHwInit2 (void)
{
    	(void) intConnect (INUM_TO_IVEC (IV_COM2_VEC), uartInt, (int)&uartChan[0]);
#if(UART_NUM > 1)
	(void) intConnect (INUM_TO_IVEC (IV_COM3_VEC), uartInt, (int)&uartChan[1]);
#if(UART_NUM > 2)
	(void) intConnect (INUM_TO_IVEC (IV_COM4_VEC), uartInt, (int)&uartChan[2]);
#endif
#endif
}
/*******************************************************************************
*
*/
void uartDevInit(UART_DEV *pChan)
{
    int oldlevel = intLock ();

    pChan->channelMode  = SIO_MODE_INT;
    pChan->options      = (CLOCAL | CREAD| CS8);/*(CLOCAL | CREAD | CS8);*/
    pChan->mcr		= MCR_OUT2;
	
    /* reset the chip */

    uartInitChannel (pChan);
	  
    intUnlock (oldlevel);
}

/*******************************************************************************
*
*/
LOCAL void uartInitChannel(UART_DEV *pChan)
{
    /* set the requested baud rate */

    uartBaudSet(pChan, pChan->baudRate);

    /* set the options */

    uartOptsSet(pChan, pChan->options);
}
/*******************************************************************************
*
* uartDrv - initialize the uartDrv driver
*
* RETURNS:  OK, or ERROR if the driver cannot be installed.
*/

STATUS uartDrv (void)
    {
    /* check if driver already installed */

    if (uartDrvNum > 0)
	return (OK);

    uartDrvNum = iosDrvInstall (uartOpen, (FUNCPTR) NULL, uartOpen,
			(FUNCPTR) NULL, uartRead, uartWrite, uartIoctl);

    return (uartDrvNum == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* uartDevCreate - create a device for an on-board beep
*
* RETURNS:  OK, or ERROR if the driver is not installed,
* or the device already exists.
*/

STATUS uartDevCreate
(
	char *name,
	UINT32 index
)
{
	UART_DEV  *pUartDev;
/*
	pUartDev = (UART_DEV *)malloc(sizeof(UART_DEV));
	bzero((char *)pUartDev, sizeof(UART_DEV));
	pUartDev->regBase = regBase;
*/
	pUartDev = &uartChan[index];
	
	pUartDev->semRev = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
	pUartDev->wDog = wdCreate ();

	/*
	semBInit (&pUartDev->syncSem, SEM_Q_FIFO,  SEM_EMPTY);
	*/
	if(iosDevAdd(&pUartDev->pDevHdr, name, uartDrvNum) == ERROR){
		free(pUartDev);
		return ERROR;
	}

	return OK;
}

/******************************************************************************
*
* uartBaudSet - change baud rate for channel
*
* This routine sets the baud rate for the UART. The interrupts are disabled
* during chip access.
*
* RETURNS: OK
*/

LOCAL STATUS  uartBaudSet
    (
    UART_DEV * pUartDev,	/* pointer to channel */
    UINT	   baud		/* requested baud rate */
    )
    {
    int   oldlevel;
    int   divisor = ((pUartDev->xtal + (8 * baud)) / (16 * baud));

    /* disable interrupts during chip access */

    oldlevel = intLock ();

    /* Enable access to the divisor latches by setting DLAB in LCR. */

    REG(LCR, pUartDev) = LCR_DLAB | pUartDev->lcr;

    /* Set divisor latches. */

    REG(DLL,pUartDev) = divisor;
    REG(DLM,pUartDev) = (divisor >> 8);

    /* Restore line control register */

    REG(LCR, pUartDev) = pUartDev->lcr;

    pUartDev->baudRate = baud;

    pUartDev->frameTime = ((CPU_CLOCK_RATE)/(pUartDev->baudRate))*50;
    pUartDev->wdTimeOut = 
    intUnlock (oldlevel);

    return (OK);
    }

/*******************************************************************************
*
* uartOptsSet - set the serial options
* RETURNS:
* Returns OK to indicate success, otherwise ERROR is returned
*/

LOCAL STATUS uartOptsSet
    (
    UART_DEV * pChan,	/* pointer to channel */
    UINT options		/* new hardware options */
    )
    {
    FAST int     oldlevel;		/* current interrupt level mask */

    pChan->lcr = 0; 
    pChan->mcr &= (~(MCR_RTS | MCR_DTR)); /* clear RTS and DTR bits */
    
    if (pChan == NULL || options & 0xffffff00)
	return ERROR;

    switch (options & CSIZE)
	{
	case CS5:
	    pChan->lcr = CHAR_LEN_5; break;
	case CS6:
	    pChan->lcr = CHAR_LEN_6; break;
	case CS7:
	    pChan->lcr = CHAR_LEN_7; break;
	default:
	case CS8:
	    pChan->lcr = CHAR_LEN_8; break;
	}

    if (options & STOPB)
	pChan->lcr |= LCR_STB;
    else
	pChan->lcr |= ONE_STOP;
    
    switch (options & (PARENB | PARODD))
	{
	case PARENB|PARODD:
	    pChan->lcr |= LCR_PEN; break;
	case PARENB:
	    pChan->lcr |= (LCR_PEN | LCR_EPS); break;
	default:
	case 0:
	    pChan->lcr |= PARITY_NONE; break;
	}

    REG(IER, pChan) = 0;

    if (!(options & CLOCAL))
	{
	/* !clocal enables hardware flow control(DTR/DSR) */

	pChan->mcr |= (MCR_DTR | MCR_RTS);
    	pChan->ier &= (~TxFIFO_BIT); 
	pChan->ier |= IER_EMSI;    /* enable modem status interrupt */
	}
    else
        pChan->ier &= ~IER_EMSI; /* disable modem status interrupt */ 

    oldlevel = intLock ();

    REG(LCR, pChan) = pChan->lcr;
    REG(MCR, pChan) = pChan->mcr;

    /* now reset the channel mode registers */

    REG(FCR, pChan) = (RxCLEAR | TxCLEAR | FIFO_ENABLE);

    if (options & CREAD)  
	pChan->ier |= RxFIFO_BIT;

    if (pChan->channelMode == SIO_MODE_INT)
	{
        REG(IER, pChan) = pChan->ier;
        }

    intUnlock (oldlevel);

    pChan->options = options;

    return OK;
    }


/*******************************************************************************
*
* ns16550ModeSet - change channel mode setting
*
* This driver supports both polled and interrupt modes and is capable of
* switching between modes dynamically. 
*
* If interrupt mode is desired this routine enables the channels receiver and 
* transmitter interrupts. If the modem control option is TRUE, the Tx interrupt
* is disabled if the CTS signal is FALSE. It is enabled otherwise. 
*
* If polled mode is desired the device interrupts are disabled. 
*
* RETURNS:
* Returns a status of OK if the mode was set else ERROR.
*/

LOCAL STATUS uartModeSet
    (
    UART_DEV * pChan,	/* pointer to channel */
    UINT	newMode		/* mode requested */
    )
{
	return (OK);
}
/*******************************************************************************
*
* ns16550Open - Set the modem control lines 
*
* Set the modem control lines(RTS, DTR) TRUE if not already set.  
* It also clears the receiver, transmitter and enables the fifo. 
*
* RETURNS: OK
*/

LOCAL int uartOpen(UART_DEV *pChan, char *name, int mode) 
{

    return (pChan);
}

/*******************************************************************************
*
* ns16550Ioctl - special device control
*
* Includes commands to get/set baud rate, mode(INT,POLL), hardware options(
* parity, number of data bits), and modem control(RTS/CTS and DTR/DSR).
* The ioctl command SIO_HUP is sent by ttyDrv when the last close() function 
* call is made. Likewise SIO_OPEN is sent when the first open() function call
* is made.
*
* RETURNS: OK on success, EIO on device error, ENOSYS on unsupported
*          request.
*/

LOCAL STATUS uartIoctl
    (
    UART_DEV * 	pChan,		/* pointer to channel */
    int			request,	/* request code */
    int        		arg		/* some argument */
    )
    {
    FAST STATUS  status;

    status = OK;

    switch (request)
	{
	case SIO_BAUD_SET:
	    if (arg < UART_MIN_RATE || arg > UART_MAX_RATE)
		status = EIO;		/* baud rate out of range */
	    else
	        status = uartBaudSet (pChan, arg);
	    break;

        case SIO_BAUD_GET:
            *(int *)arg = pChan->baudRate;
            break; 

        case SIO_MODE_SET:
	    status = (uartModeSet (pChan, arg) == OK) ? OK : EIO;
            break;          

        case SIO_MODE_GET:
            *(int *)arg = pChan->channelMode;
            break;

        case SIO_AVAIL_MODES_GET:
            *(int *)arg = SIO_MODE_INT | SIO_MODE_POLL;
            break;

        case SIO_HW_OPTS_SET:
    	    status = (uartOptsSet (pChan, arg) == OK) ? OK : EIO;
    	    break;

        case SIO_HW_OPTS_GET:
            *(int *)arg = pChan->options;
            break;

        case SIO_HUP:
            /* check if hupcl option is enabled */
		/*
    	    if (pChan->options & HUPCL) 
	    	status = uartHup (pChan);*/
            break;
	
	case SIO_OPEN:
            /* check if hupcl option is enabled */
/*
    	    if (pChan->options & HUPCL) 
	    	status = uartOpen (pChan);*/
	    break;

	case SIO_PAR_RESET:
		uartParReset(pChan);
		break;
	case SIO_LEN_GET:
		*(int *)arg = pChan->len;
		break;
        default:
            status = ENOSYS;
	}
    return (status);
    }

/*******************************************************************************
*
*/
void uartParReset(UART_DEV * pUartDev)
{
	int oldlevel = intLock ();
/*
	uartLen[1] = 0;
	uartSemGiveFlag[1] = FALSE;
	uartFrameFlag[1] = TRUE;
*/
	pUartDev->len = 0;
	pUartDev->semGiveFlag = FALSE;
	pUartDev->frameFlag = TRUE;

  	intUnlock (oldlevel);
}
/*******************************************************************************
*
*/
int IsTimeOut(UART_DEV * pChan, unsigned int old, unsigned int new1)
{
	if( new1 > old){
		if((new1 - old) > pChan->frameTime){
			return TRUE;
		}
		else
			return FALSE;
	}
	else{
		if((0xFFFFFFFF - old + new1) > pChan->frameTime){
			return TRUE;
		}
		else
			return FALSE;
	}
}
 /*******************************************************************************
* add by sunchao
*/
 void uartWdIsr(UART_DEV * pChan)
 {
 /*
     uartSemGiveFlag[pChan->unit] = TRUE;
     semGive(uartSem[pChan->unit]);
 */
 	pChan->semGiveFlag = TRUE;
 	semGive(pChan->semRev);
	 logMsg("I\n", 0,0,0,0,0,0);  
 }
 /*
 *
 */
void uartIntRd 
    (
    UART_DEV * pChan	/* pointer to channel */
    )
    {
    /* read character from Receive Holding Reg. */
    unsigned int count;
	
    pChan->inchar = REG(RBR, pChan);
	/* logMsg("inchar = %x\n", pChan->inchar, 0,0,0,0,0); */
	if( pChan->frameFlag){/*新一帧的第一个字符*/
		pChan->frameFlag = FALSE;
		pChan->count = sysCountGet();
		pChan->pool[pChan->len ++] = pChan->inchar;
		/* uartLen[pChan->unit] ++; */
		/* logMsg("11len = %d char = %x\n", pChan->len, pChan->inchar); */
		wdStart (pChan->wDog, 2, uartWdIsr, pChan); 
		return;
	}
	if(pChan->semGiveFlag== FALSE){
		count = sysCountGet();
		if(IsTimeOut(pChan, pChan->count, count)){
			semGive(pChan->semRev);
			pChan->semGiveFlag= TRUE;
			wdCancel(pChan->wDog); 
			logMsg("N\n", 0,0,0,0,0,0);  
		}/*IsTimeOut*/
		else{
			pChan->count = sysCountGet();
			pChan->pool[pChan->len ++] = pChan->inchar;
			/* uartLen[pChan->unit] ++; */
			wdCancel(pChan->wDog); 
			wdStart (pChan->wDog, 2, uartWdIsr, pChan); 
			/* logMsg("22len = %d char = %x\n", pChan->len, pChan->inchar);  */
		}
	}/*uartSemGiveFlag*/
    }

/********************************************************************************
*
* ns16550Int - interrupt level processing
*
* This routine handles four sources of interrupts from the UART. They are
* prioritized in the following order by the Interrupt Identification Register:
* Receiver Line Status, Received Data Ready, Transmit Holding Register Empty
* and Modem Status.
*
* When a modem status interrupt occurs, the transmit interrupt is enabled if
* the CTS signal is TRUE.
*
* RETURNS: N/A
*
*/
void uartInt 
    (
    UART_DEV * pChan	/* pointer to channel */
    )
    {
    FAST volatile char        intStatus;
	
    intStatus = (REG(IIR, pChan)) & 0x0f;
	/*
	printstr("intStatus = 0x");
	printnum(intStatus);
	printstr("\r\n");
	*/
    /*
     * This UART chip always produces level active interrupts, and the IIR 
     * only indicates the highest priority interrupt.  
     * In the case that receive and transmit interrupts happened at
     * the same time, we must clear both interrupt pending to prevent
     * edge-triggered interrupt(output from interrupt controller) from locking
     * up. One way doing it is to disable all the interrupts at the beginning
     * of the ISR and enable at the end.
     */

    REG(IER,pChan) = 0;    /* disable interrupt */

    switch (intStatus)
	{
	case IIR_RLS:
            /* overrun,parity error and break interrupt */
			
            intStatus = REG(LSR, pChan); /* read LSR to reset interrupt */
	    break;

        case IIR_RDA:     		/* received data available */
	case IIR_TIMEOUT: 
	   /*
	    * receiver FIFO interrupt. In some case, IIR_RDA will
            * not be indicated in IIR register when there is more
	    * than one character in FIFO.
	    */

			
            uartIntRd (pChan);  	/* RxChar Avail */
            /*semGive (pChan->syncSem); */
            break;

       	case IIR_THRE:  /* transmitter holding register ready */
            break;

	case IIR_MSTAT: /* modem status register */
	   break;

        default:
	    break;
        }
    
    REG(IER, pChan) = pChan->ier; /* enable interrupts accordingly */

	
    }	

/*******************************************************************************
*
*/
LOCAL int uartRead(UART_DEV *pUartDv,char *buffer, int nbytes)
{
	unsigned int i;
	unsigned int len;

	semTake(pUartDv->semRev, WAIT_FOREVER);/* 一帧接收完毕  */
	if(nbytes >= pUartDv->len)
		len = pUartDv->len;
	else
		len = nbytes;
	
	for(i = 0; i < len; i ++)
	{
		*(buffer + i) = pUartDv->pool[i];
	}

	return len;
}
/*******************************************************************************
*
*/
LOCAL int uartWrite(UART_DEV *pUartDv,char *buffer, int nbytes)
{
	unsigned int i;
	char pollStatus = REG(LSR, pUartDv);
	
	for(i = 0; i < nbytes; i ++)
	{
		while((REG(LSR, pUartDv) & LSR_THRE) == 0x00)
			;
		REG(THR, pUartDv) = *(buffer + i); 
	}
	
	return nbytes;
}


