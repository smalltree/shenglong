/*****************************************************************************************
通过vxWorks自己提供的看门狗WD来模拟Auxxiliary clock
******************************************************************************************/
#include <wdLib.h>
#include <logLib.h>
#include <sysLib.h>
#include "config.h"
#include "softAuxTimer.h"

#ifdef INCLUDE_SOFTAUXTIMER
/* defines */

#define RTC_LONG_RATE	32768	/* aux clock counter runs at 32kHz */

/* locals */
LOCAL WDOG_ID auxDog;
LOCAL int   sysAuxClkTicksPerSecond = 60;	/* default aux timer rate    */
LOCAL int   sysAuxClkArg	    = (int)NULL;/* aux clock int routine arg */
LOCAL BOOL  sysAuxClkRunning	    = FALSE;	/* sys aux clock enabled flag*/
LOCAL FUNCPTR	sysAuxClkRoutine    = NULL;	/* aux clock interpt routine */
LOCAL int   sysAuxClkTicks; 		        /* aux clk ticks */

/*******************************************************************************
*
*/
void sysAuxClkInit(void)
{
	if((auxDog = wdCreate()) == NULL){
		logMsg("watchDog create failed!\n", 0, 0, 0, 0, 0, 0);
		return;
	}
	sysAuxClkTicksPerSecond = sysClkRateGet();
}
/*******************************************************************************
*
* sysAuxClkInt - interrupt level processing for auxiliary clock
*
* This routine handles the auxiliary clock interrupt.  It is attached to the
* clock interrupt vector by the routine sysAuxClkConnect().
* The appropriate routine is called and the interrupt is acknowleged.
*/

LOCAL void sysAuxClkInt (void)
    {
    /* clear the RTCLong1 interrupt */
    if (sysAuxClkRoutine != NULL)
	(*sysAuxClkRoutine) (sysAuxClkArg);	/* call system clock routine */

    if(wdStart(auxDog, 1, (FUNCPTR)sysAuxClkInt, 0) == ERROR){
		logMsg("auxDog start failed in int\n", 0, 0, 0, 0, 0, 0);
		return;
	}
    }
/*******************************************************************************
*
* sysAuxClkConnect - connect a routine to the auxiliary clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* auxiliary clock interrupt.  It does not enable auxiliary clock
* interrupts.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), sysAuxClkEnable()
*/

STATUS sysAuxClkConnect
    (
    FUNCPTR routine,    /* routine called at each aux clock interrupt    */
    int arg             /* argument to auxiliary clock interrupt routine */
    )
    {
    sysAuxClkRoutine = routine;
    sysAuxClkArg = arg;

    return (OK);
    }
/*******************************************************************************
*
* sysAuxClkEnable - turn on auxiliary clock interrupts
*
* This routine enables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkConnect(), sysAuxClkDisable(), sysAuxClkRateSet()
*/

void sysAuxClkEnable (void)
    {
    if (!sysAuxClkRunning)
	{
	int key;

	key = intLock ();
	sysAuxClkRunning = TRUE;
	sysAuxClkTicks = sysAuxClkTicksPerSecond;
    if(wdStart(auxDog, 1, (FUNCPTR)sysAuxClkInt, 0) == ERROR){
		logMsg("auxDog start failed in enable\n", 0, 0, 0, 0, 0, 0);
		return;
	}
	intUnlock (key);
	}
    }
/*******************************************************************************
*
* sysAuxClkDisable - turn off auxiliary clock interrupts
*
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkEnable()
*/

void sysAuxClkDisable (void)
    {
    if (sysAuxClkRunning)
	{
	int key;

	key = intLock ();
	sysAuxClkRunning = FALSE;
	wdCancel(auxDog);
	intUnlock (key);
	}
    }
/*******************************************************************************
*
* sysAuxClkRateGet - get the auxiliary clock rate
*
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/

int sysAuxClkRateGet (void)
    {
    return (sysAuxClkTicksPerSecond);
    }

/*******************************************************************************
*
* sysAuxClkRateSet - set the auxiliary clock rate
*
* This routine sets the interrupt rate of the auxiliary clock.
* It does not enable auxiliary clock interrupts.
*
* RETURNS: OK, or ERROR if the tick rate is invalid or the timer cannot be set.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/

STATUS sysAuxClkRateSet
    (
    int ticksPerSecond     /* number of clock interrupts per second */
    )
    {
    if (ticksPerSecond < AUX_CLK_RATE_MIN || ticksPerSecond > AUX_CLK_RATE_MAX){
		logMsg("tick must be 1 ~ 60\n", 0, 0, 0, 0, 0, 0);
	return (ERROR);
    	}
/*
    if (((RTC_LONG_RATE / ticksPerSecond) < 4)
	|| ((RTC_LONG_RATE / ticksPerSecond) > 0x00ffffff))
	return (ERROR);
*/
    sysAuxClkTicksPerSecond = ticksPerSecond;

    if (sysAuxClkRunning)
	{
	sysAuxClkDisable ();
	sysAuxClkEnable ();
	}

    return (OK);
    }
#endif

