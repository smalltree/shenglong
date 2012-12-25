#include "flash_sst.h"


/*********************************************************************************************************
*函数功能: 擦除flash芯片的一个扇区
*入口参数: 扇区起始地址
*出口参数: 无
**********************************************************************************************************/
void sstEraseSector(unsigned int offset)
{
	/* 发送命令 */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + offset), FL_SECT);
	sstIsBusy(offset);
}

/*********************************************************************************************************
*函数功能: 擦除整个flash芯片
*入口参数: 无
*出口参数: 无
**********************************************************************************************************/
void sstEraseChip(void)
{
	/* 发送命令 */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE_CHIP);
	sstIsBusy(0x7d00);
}
/*********************************************************************************************************
*函数功能: 等待flash 空闲(用toggle bit的方式)
*入口参数: 任意地址
*出口参数: 1: 忙  0:闲
**********************************************************************************************************/
unsigned int sstIsBusy(unsigned int offset)
{
	unsigned int busy;
	unsigned char poll1, poll2;

	while(1)
	{
		poll1 = inb(SST_BASEADDR + offset);
		poll2 = inb(SST_BASEADDR + offset);
		if((poll1 ^ poll2) & 0x40){
			busy = 1;
			break;
		}
		else{
			poll1 = inb(SST_BASEADDR + offset);
			poll2 = inb(SST_BASEADDR + offset);
			if((poll1 ^ poll2) & 0x40) busy = 1;
			else busy = 0;/* internal program complete */
			break;
		}
	}

	return busy;
}
/********************************************************************************************************
*函数功能: 向指定地址写入一个字节
*入口参数: offset:地址   dat: 要写入的数据
*出口参数: 无
*********************************************************************************************************/
void sstWriteByte(unsigned int offset, unsigned char dat)
{
	unsigned int stat;
	/* 发送命令 */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xA0);
	outb((SST_BASEADDR + offset), dat);

	do{
		stat = sstIsBusy(offset);
	}while(stat == 1);
}
/********************************************************************************************************
*函数功能: 软件延时函数
*
*
*********************************************************************************************************/
void sstSoftDelay1(unsigned int t)
{
	unsigned int i;
	for(i = 0; i < t; i ++);
}
