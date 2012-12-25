#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#define LS_GPIO_DIR		*(volatile unsigned int *)(0xbfe00100+0x20)
#define LS_GPIO_DAT		*(volatile unsigned int *)(0xbfe00100+0x1C)

#define GPIO0			0
#define GPIO1			1
#define GPIO2			2
#define GPIO3			3

extern   void watchDog(void);
extern   void Enable_wd(int num);
extern   void Feed_wd(void);

#endif

