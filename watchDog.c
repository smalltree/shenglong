#include <wdLib.h>
#include <logLib.h>
#include <sysLib.h>
#include "watchDog.h"

void WdIsr(void);

WDOG_ID	dog;

void watchDog(void)
{
	if((dog = wdCreate()) == NULL){
		logMsg("watchDog create failed!\n", 0, 0, 0, 0, 0, 0);
		return;
	}
	if(wdStart(dog, sysClkRateGet(), (FUNCPTR)WdIsr, 0) == ERROR){
		logMsg("watchDog start failed!\n", 0, 0, 0, 0, 0, 0);
		return;
	}
	/* watchdog enable */
	LS_GPIO_DIR = 0x00;
	LS_GPIO_DAT |= (1 << GPIO3);
}

void Enable_wd(int num)
{
	if(num>0)
	{
		LS_GPIO_DIR = 0x00;
		LS_GPIO_DAT |= (1 << GPIO3);	
	}
	else
	{
		LS_GPIO_DAT &= ~(1 << GPIO3);
	}
		
}
void Feed_wd(void)
{
	 LS_GPIO_DAT ^= (1 << GPIO2); 
}

void WdIsr(void)
{
	 LS_GPIO_DAT ^= (1 << GPIO2); 
	if(wdStart(dog, sysClkRateGet(), (FUNCPTR)WdIsr, 0) == ERROR){
		logMsg("watchDog start failed!\n", 0, 0, 0, 0, 0, 0);
		return;
	}
}
