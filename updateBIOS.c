#include <vxWorks.h>
#include <stdio.h>
#include "flash_sst.h"

#define  BIOS_SIZE	(512*1024)

unsigned char data[BIOS_SIZE];

unsigned int updateBIOS(void)
{
	FILE* fd;
	int i, num;

	fd = fopen("bootrom.bin", "rb");
	if(fd == NULL){
		printf("open file bootrom.bin failed!\n");
		return ;
	}
	for(num = 0; num < BIOS_SIZE; num++)
	{
	       data[num] = 0xff;
	}
	num = fread(data, 1, BIOS_SIZE, fd);
	fclose(fd);
	printf("the bootrom's size is %d bytes.\n",num);
	
	sstEraseChip();
	sstSoftDelay1(0x3ffffff);

	printf("erase the chip ...\n");
	for(i = 0; i < num; i ++)
	{
		sstWriteByte(i, data[i]);
	}

	printf("BIOS update succeed... please reboot the machine\n");
	
	return OK;
}


