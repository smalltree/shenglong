/*
	Loongson2F_7010A  beep Driver
*/

#include <vxWorks.h>
#include <iv.h>
#include <intLib.h>
#include <stdlib.h>
#include <memLib.h>
#include <ioLib.h>
#include <iosLib.h>
#include <string.h>
#include "gpio.h"

LOCAL int gpioDrvNum;		/* driver number assigned to this driver */

LOCAL int gpioOpen(GPIO_DEV *pGpioDv, char *name, int  mode); 
LOCAL int gpioRead(GPIO_DEV *pGpioDv,char *buffer, int nbytes);
LOCAL int gpioWrite(GPIO_DEV *pGpioDv,char *buffer, int nbytes);
LOCAL int gpioIoctl(GPIO_DEV *pGpioDv,int command, int arg);

/*******************************************************************************
*
* beepDrv - initialize the gpioDrv driver
*
* RETURNS:  OK, or ERROR if the driver cannot be installed.
*/

STATUS gpioDrv (void)
    {
    /* check if driver already installed */

    if (gpioDrvNum > 0)
	return (OK);

    gpioDrvNum = iosDrvInstall (gpioOpen, (FUNCPTR) NULL, gpioOpen,
			(FUNCPTR) NULL, gpioRead, gpioWrite, gpioIoctl);

    return (gpioDrvNum == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* beepDevCreate - create a device for an on-board beep
*
* RETURNS:  OK, or ERROR if the driver is not installed,
* or the device already exists.
*/

STATUS gpioDevCreate
(
	char *name,
	UINT32 regBase,
	UINT32 bit
)
{
	GPIO_DEV  *pGpioDev;

	pGpioDev = (GPIO_DEV *)malloc(sizeof(GPIO_DEV));
	bzero((char *)pGpioDev, sizeof(GPIO_DEV));
	pGpioDev->regBase = regBase;
	pGpioDev->bit = bit;

	if(iosDevAdd(&pGpioDev->pDevHdr, name, gpioDrvNum) == ERROR){
		free(pGpioDev);
		return ERROR;
	}

	return OK;
}

/*******************************************************************************
*
* gpioOpen - open file to GPIO
*
* RETURNS: Ptr to device structure.
*/

LOCAL int gpioOpen
    (
    GPIO_DEV *pGpioDv,
    char      *name,
    int        mode
    )
    {
    return ((int) pGpioDv);
    }

/*******************************************************************************
*
* gpioRead - read GPIO
*
* RETURNS: 1
*/
LOCAL int gpioRead
(
	GPIO_DEV *pGpioDv,
	char 	  *buffer,
	int		   nbytes
)
{
	UINT32	tmp;
	UINT32   oldVal;

	oldVal = intLock();
	tmp = *(volatile unsigned int *)(pGpioDv->regBase);
	intUnlock(oldVal);

	if(tmp & (1<<pGpioDv->bit)){
		*buffer = 0;
	}
	else
		*buffer = 1;

	return 1;
}

/*******************************************************************************
*
* gpioWrite - write GPIO
*
* RETURNS: 1
*/
LOCAL int gpioWrite
(
	GPIO_DEV *pGpioDv,
	char 	  *buffer,
	int		   nbytes
)
{
	UINT32	tmp;
	UINT32	oldVal;

	oldVal = intLock();
	tmp = *(volatile unsigned int *)(pGpioDv->regBase);
	if(*buffer){
		 tmp &= ~(1<<pGpioDv->bit);
		 *(volatile unsigned int *)(pGpioDv->regBase) = tmp;
	}
	else{
		tmp |= (1<<pGpioDv->bit);
		*(volatile unsigned int *)(pGpioDv->regBase) = tmp;
	}
	intUnlock(oldVal);

	return 1;
}

/*******************************************************************************
*
* gpioIoctl - control GPIO
*
* RETURNS: 1
*/
LOCAL int gpioIoctl
(
	GPIO_DEV *pGpioDv,
	int 	          command,
	int		   arg
)
{
	return OK;
}
