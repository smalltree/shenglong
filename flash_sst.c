#include "flash_sst.h"


/*********************************************************************************************************
*��������: ����flashоƬ��һ������
*��ڲ���: ������ʼ��ַ
*���ڲ���: ��
**********************************************************************************************************/
void sstEraseSector(unsigned int offset)
{
	/* �������� */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + offset), FL_SECT);
	sstIsBusy(offset);
}

/*********************************************************************************************************
*��������: ��������flashоƬ
*��ڲ���: ��
*���ڲ���: ��
**********************************************************************************************************/
void sstEraseChip(void)
{
	/* �������� */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), FL_ERASE_CHIP);
	sstIsBusy(0x7d00);
}
/*********************************************************************************************************
*��������: �ȴ�flash ����(��toggle bit�ķ�ʽ)
*��ڲ���: �����ַ
*���ڲ���: 1: æ  0:��
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
*��������: ��ָ����ַд��һ���ֽ�
*��ڲ���: offset:��ַ   dat: Ҫд�������
*���ڲ���: ��
*********************************************************************************************************/
void sstWriteByte(unsigned int offset, unsigned char dat)
{
	unsigned int stat;
	/* �������� */
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xAA);
	outb((SST_BASEADDR + SST_CMDOFFS2), 0x55);
	outb((SST_BASEADDR + SST_CMDOFFS1), 0xA0);
	outb((SST_BASEADDR + offset), dat);

	do{
		stat = sstIsBusy(offset);
	}while(stat == 1);
}
/********************************************************************************************************
*��������: �����ʱ����
*
*
*********************************************************************************************************/
void sstSoftDelay1(unsigned int t)
{
	unsigned int i;
	for(i = 0; i < t; i ++);
}
