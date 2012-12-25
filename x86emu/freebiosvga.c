/* Use biosemu/x86emu to POST VGA cards
 *
 * Copyright 2002 Fuxin Zhang,BaoJian Zheng
 * Institute of Computing Technology,Chinese Academy of Sciences,China 
 */

#include "stdio.h"
#include "biosemui.h"
#include "stdlib.h"
#include "string.h"
#include "config.h"
#include "drv\pci\pciConfigLib.h"
#include <io.h>


#include "vgaRom.c"


#define VGA_DEVNUM 7
#define VGA_FUNCNUM 0

#ifndef readw
#define readw(addr)             ((*(volatile unsigned short *)(0xa0000000|(addr))))
#endif
#ifndef readl
#define readl(addr)             ((*(volatile unsigned int *)(0xa0000000|(addr))))
#endif

/* Length of the BIOS image */
#define MAX_BIOSLEN         (64 * 1024L)

static	u32              debugFlags = 0;
static	BE_VGAInfo       VGAInfo[1] = {{0}};

static	RMREGS          regs;
static  RMSREGS         sregs;

static struct pci_device vga_dev;

extern void prom_printf(char *fmt, ...);

int vga_bios_init(void)
{
	unsigned long romsize = 0;
	unsigned long romaddress = 0;
	unsigned char magic[2];
	unsigned short ppcidata; /* pointer to pci data structure */
	unsigned char pcisig[4]; /* signature of pci data structure */
	unsigned char codetype;
	unsigned short vendor_id, device_id;
	int pciBus =0, pciDevice = 0, pciFunc = 0;
/*	unsigned int vesa_mode = 0;*/

	pciConfigInWord(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_VENDOR_ID, &vendor_id);
	pciConfigInWord(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_DEVICE_ID, &device_id);

#define VGA_VENID_ATI          0x1039
#define VGA_DEVID_ATI          0x0325
	if (pciFindDevice (VGA_VENID_ATI, VGA_DEVID_ATI, 0,
                       &pciBus, &pciDevice, &pciFunc) != OK)
	{
		prom_printf("Can not find vga devicex\n");
		return ERROR;
        }

	if (vendor_id == 0x1039 && device_id == 0x0325)
		BE_wrw(0xc015e,0x4750);

	
#ifdef VGA_NOROM
	pciConfigInLong(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_EXPANSION_ROM, (int*)&romaddress);
	romaddress &= (~1);
	/* enable rom address decode */
	pciConfigOutLong(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_EXPANSION_ROM,romaddress|1);

	if (romaddress == 0) {
		prom_printf("No rom address assigned,skipped\n");
		return -1;
	}
#ifdef BONITOEL
	romaddress |= 0x10000000;
#endif
#else
	romaddress = (unsigned int)vgaRom;
#endif
	prom_printf("Rom mapped to %lx\n",romaddress);

	magic[0] = readb(romaddress);
	magic[1] = readb(romaddress + 1);

	if (magic[0]==0x55 && magic[1]==0xaa) {
		prom_printf("VGA bios found\n");

		/* rom size is stored at offset 2,in 512 byte unit*/
		romsize = (readb(romaddress + 2)) * 512;
		prom_printf("rom size is %ldk\n",romsize/1024);

		ppcidata = readw(romaddress + 0x18);
		prom_printf("PCI data structure at offset %x\n",ppcidata);
		pcisig[0] = readb(romaddress + ppcidata);
		pcisig[1] = readb(romaddress + ppcidata + 1);
		pcisig[2] = readb(romaddress + ppcidata + 2);
		pcisig[3] = readb(romaddress + ppcidata + 3);
		if (pcisig[0]!='P' || pcisig[1]!='C' ||
				pcisig[2]!='I' || pcisig[3]!='R') {
			prom_printf("PCIR expected,read %c%c%c%c\n",
					pcisig[0],pcisig[1],pcisig[2],pcisig[3]);
			prom_printf("Invalid pci signature found,give up\n");
			return -1;
		}

		codetype  = readb(romaddress + ppcidata + 0x14);

		if (codetype != 0) {
			prom_printf("Not x86 code in rom,give up\n");
			return -1;
		}

	} else {
		prom_printf("No valid bios found,magic=%x%x\n",magic[0],magic[1]);
		return -1;
	}


	vga_dev.bus = 0;
	vga_dev.devnum = VGA_DEVNUM;
	vga_dev.func = VGA_FUNCNUM;
	vga_dev.pa_id = vendor_id | (device_id << 16);
	memset(VGAInfo,0,sizeof(BE_VGAInfo));
	VGAInfo[0].pciInfo =  &vga_dev;
#ifdef VGA_NOROM
	VGAInfo[0].BIOSImage = (void*)malloc(romsize);
	if (VGAInfo[0].BIOSImage == NULL) {
		prom_printf("Error alloc memory for vgabios\n");
		return -1;
	}
	VGAInfo[0].BIOSImageLen = romsize;
	memcpy(VGAInfo[0].BIOSImage,(char*)(0xa0000000|romaddress),romsize);
#else
	VGAInfo[0].BIOSImage = romaddress|0xa0000000;
	VGAInfo[0].BIOSImageLen = romsize;
#endif

	BE_init(debugFlags,65536,&VGAInfo[0]);

	regs.h.ah =  0;
	regs.h.al = (VGA_DEVNUM <<3)|( VGA_FUNCNUM &0x7);

#ifdef DEBUG_EMU_VGA
	X86EMU_trace_on();
#endif
	BE_callRealMode(0xC000,0x0003,&regs,&sregs);
#if 0
	{
		RMREGS in;
		RMREGS out;
		in.e.eax = 0x0003;
		BE_int86(0x10,&in,&out);
	}
#endif

#ifndef VGA_NOROM 
	/*BE_exit(); */
	pciConfigInLong(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_EXPANSION_ROM,(int*)&romaddress);
	/* disable rom address decode */
	pciConfigOutLong(0, VGA_DEVNUM, VGA_FUNCNUM, PCI_CFG_EXPANSION_ROM,romaddress & ~1);
#endif

	prom_printf("vgabios_init: Emulation done\n");
	return 1;

}
