#ifndef GPIO_H_
#define GPIO_H_

typedef struct 
{
	DEV_HDR		pDevHdr;
	UINT32		regBase;
	UINT32		bit;
}GPIO_DEV;

STATUS gpioDrv (void);  
STATUS gpioDevCreate(char *name, UINT32 regBase, UINT32 bit);  

#endif
