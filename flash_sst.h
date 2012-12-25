#ifndef FLASH_SST_H_
#define FLASH_SST_H_

/* ADDRESS macro */
#define    SST_BASEADDR		0xbfc00000
#define    SST_CMDOFFS1		0x5555
#define    SST_CMDOFFS2		0x2aaa
#define    FL_ERASE			0x80
#define 	FL_SECT				0x30
#define    FL_ERASE_CHIP		0x10

/* IO macro */
#define	outb(a,v)	(*(volatile unsigned char*)(a) = (v))
#define	inb(a)		(*(volatile unsigned char*)(a))

/**/
void sstEraseSector(unsigned int offset);
void sstEraseChip(void);
unsigned int sstIsBusy(unsigned int offset);
void sstWriteByte(unsigned int offset, unsigned char dat);
void sstSoftDelay1(unsigned int t);

#endif

