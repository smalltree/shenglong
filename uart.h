#ifndef UART_H_
#define UART_H_
#include "drv/sio/ns16552Sio.h"
#include <semLib.h>

#define  UART_NUM	3
#define UART_MIN_RATE 50
#define UART_MAX_RATE 115200

#define SIO_PAR_RESET	0x2012
#define SIO_LEN_GET		0x2013
#define UART_POOL_SIZE	512

#define REG(reg, pchan) \
 (*(volatile UINT8 *)((UINT32)pchan->regs + (reg)))

typedef struct 
{
	DEV_HDR		pDevHdr;

	INT8 			inchar;
	
	UINT8			unit;
	UINT8 			*regs;
	UINT8 			ier;
	UINT8 			lcr;
	UINT8 			mcr;
	UINT8 			pad1;
	UINT8			intLevel;
	UINT8    			semGiveFlag;
	UINT8   			frameFlag;
	UINT8  			pool[UART_POOL_SIZE];
	UINT16      		channelMode;
	
	UINT32			baudRate;
	UINT32			xtal;
	UINT32			options;
	UINT32 			frameTime;
	UINT32 			wdTimeOut;
	
	UINT32 			count;
	UINT32 			len;

	SEM_ID			semRev;	/* binary sem for syncronization */
	WDOG_ID  		wDog;
}UART_DEV;

#endif

