/*
 * ops-lm2f.c
 *
 * Copyright (C) 2004 ICT CAS
 * Author: Li xiaoyu, ICT CAS
 *   lixy@ict.ac.cn
 *
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>

#include <bonito.h>*/
#include "bonito64.h"
#include <types/vxTypesOld.h>

#define	PCI_OPS_CS5536_IDSEL	14

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

extern void	cs5536_pci_conf_write4(int function, int reg, UINT32 value);
extern UINT	cs5536_pci_conf_read4(int function, int reg);

static void bflush(void)
{
	/* flush Bonito register writes */
	(void)BONITO_PCICMD;
}

static int lm2e_pci_config_access(unsigned char access_type,
				  int busnum, unsigned int devfn,
				  int where, UINT32 *data)
{
	unsigned int addr, type;
	void *addrp;
	int device; 
	int function; 
	int reg; 

	device = (devfn>>3)&0x1f;
	function = (devfn&0x07);
	reg = where & ~3;
	
	/************************************************************************/
	/* CS5536 PCI ACCESS ROUTINE :						*/
	/*	Note the functions circle call :				*/
	/* lm2e_pci_config_access()--->cs5536_pci_conf_read/write4()--->	*/
	/* _rdmsr/_wrmsr()--->lm2e_pci_config_access()				*/
	/************************************************************************/
	if( (busnum == 0) && (device == 14) && (reg < 0xF0) ){
		switch(access_type){
			case PCI_ACCESS_READ :
				*data = cs5536_pci_conf_read4(function, reg);
				break;
			case PCI_ACCESS_WRITE :
				cs5536_pci_conf_write4(function, reg, *data);
				break;
		}
		return 0;
	}
	
	if (busnum == 0) {
		/* Type 0 configuration on onboard PCI bus */
		if (device > 20 || function > 7) {
			*data = -1;	/* device out of range */
			return  0x86; /*PCIBIOS_DEVICE_NOT_FOUND;*/
		}
		addr = (1 << (device + 11)) | (function << 8) | reg;
		type = 0;
	} else {
		/* Type 1 configuration on offboard PCI bus */
		if (device > 31 || function > 7) {
			*data = -1;	/* device out of range */
			return 0x86; /*PCIBIOS_DEVICE_NOT_FOUND;*/
		}
		addr = (busnum << 16) | (device << 11) | (function << 8) | reg;
		type = 0x10000;
	}

	/* clear aborts */
	BONITO_PCICMD |= 0x30000000; /*BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT;*/

	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	bflush();

	addrp = (void *)(((0x1fe80000 | (addr & 0xffff)) & 0x1fffffff ) | 0xa0000000);
	if (access_type == 1 /*PCI_ACCESS_WRITE*/) {
		*(volatile unsigned int *)addrp = /*cpu_to_le32*/(*data);
	} else {
		*data = /*le32_to_cpu*/(*(volatile unsigned int *)addrp);
	}
	if (BONITO_PCICMD & (0x30000000/*BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT*/)) {
		BONITO_PCICMD |= 0x30000000; /*BONITO_PCICMD_MABORT | BONITO_PCICMD_MTABORT*/;
		*data = -1;
		return 0x86;/*PCIBIOS_DEVICE_NOT_FOUND*/;
	}

	return 0;/*PCIBIOS_SUCCESSFUL;*/

}

static int lm2e_pci_pcibios_read(int busnum, unsigned int devfn,
				 int where, int size, UINT32 * val)
{
	UINT32 data;
	int ret;
	
	data = 0;
	ret = lm2e_pci_config_access(PCI_ACCESS_READ,
			busnum, devfn, where, &data);

	if (ret != /*PCIBIOS_SUCCESSFUL*/0)
		return ret;

	if (size == 1)
		*val = (data >> ((where & 3) << 3)) & 0xff;
	else if (size == 2)
		*val = (data >> ((where & 3) << 3)) & 0xffff;
	else
		*val = data;

	return /*PCIBIOS_SUCCESSFUL*/ 0;
}

static int lm2e_pci_pcibios_write(int busnum, unsigned int devfn,
				  int where, int size, UINT32 val)
{
	UINT32 data;
	int ret;

	data = 0;
	if (size == 4)
		data = val;
	else {
		ret = lm2e_pci_config_access(/*PCI_ACCESS_READ*/0,
				busnum, devfn, where, &data);
		if (ret != /*PCIBIOS_SUCCESSFUL*/0)
			return ret;

		if (size == 1)
			data = (data & ~(0xff << ((where & 3) << 3))) |
			    (val << ((where & 3) << 3));
		else if (size == 2)
			data = (data & ~(0xffff << ((where & 3) << 3))) |
			    (val << ((where & 3) << 3));
	}

	ret = lm2e_pci_config_access(/*PCI_ACCESS_WRITE*/1,
			busnum, devfn, where, &data);
	if (ret != /*PCIBIOS_SUCCESSFUL*/0)
		return ret;

	return /*PCIBIOS_SUCCESSFUL*/0;
}

void _rdmsr(UINT32 msr, UINT32 *hi, UINT32 *lo)
{
	int busnum;
	UINT32 devfn;
	busnum=0;
	devfn = /*PCI_DEVFN(14, 0);*/14<<3;
	lm2e_pci_pcibios_write(busnum, devfn, 0xf4, 4, msr);
	lm2e_pci_pcibios_read(busnum, devfn, 0xf8, 4, lo);
	lm2e_pci_pcibios_read(busnum, devfn, 0xfc, 4, hi);
	/*printk("rdmsr msr %x, lo %x, hi %x\n", msr, *lo, *hi);*/
}

void _wrmsr(UINT32 msr, UINT32 hi, UINT32 lo)
{
	int busnum;
	UINT32 devfn;
	busnum=0;
	devfn = /*PCI_DEVFN(14, 0);*/14<<3;
	lm2e_pci_pcibios_write(busnum, devfn, 0xf4, 4, msr);
	lm2e_pci_pcibios_write(busnum, devfn, 0xf8, 4, lo);
	lm2e_pci_pcibios_write(busnum, devfn, 0xfc, 4, hi);
	/*printk("wrmsr msr %x, lo %x, hi %x\n", msr, lo, hi);*/ 
}

/*struct pci_ops loongson2e_pci_pci_ops = {
	.read = lm2e_pci_pcibios_read,
	.write = lm2e_pci_pcibios_write
};*/
