#ifndef __SDCFG_H__
#define __SDCFG_H__

#include "arch/mips/archMips.h"
  	
#ifdef _ASMLANGUAGE

#define	PT_ENTRY_NULL	((pt_entry_t *) 0)

#define PG_WIRED	0x80000000	/* SW */
#define PG_RO		0x40000000	/* SW */

#define	PG_SVPN		0xfffff000	/* Software page no mask */
#define	PG_HVPN		0xffffe000	/* Hardware page no mask */
#define	PG_ODDPG	0x00001000	/* Odd even pte entry */
#define	PG_ASID		0x000000ff	/* Address space ID */
#define	PG_G		0x00000001	/* HW */
#define	PG_V		0x00000002
#define	PG_NV		0x00000000
#define	PG_M		0x00000004
#define	PG_ATTR		0x0000003f
#define	PG_UNCACHED	0x00000010
#define	PG_CACHED	0x00000018
#define	PG_CACHEMODE	0x00000038
#define	PG_ROPAGE	(PG_V | PG_RO | PG_CACHED) /* Write protected */
#define	PG_RWPAGE	(PG_V | PG_M | PG_CACHED)  /* Not wr-prot not clean */
#define	PG_CWPAGE	(PG_V | PG_CACHED)	   /* Not wr-prot but clean */
#define PG_GRWPAGE  (PG_G | PG_V | PG_M | PG_CACHED) 
#define	PG_IOPAGE	(PG_G | PG_V | PG_M | PG_UNCACHED)
#define	PG_FRAME	0x3fffffc0
#define PG_SHIFT	6
#define pfn_is_ext(x) ((x) & 0x3c000000)
#define vad_to_pfn(x) (((unsigned)(x) >> PG_SHIFT) & PG_FRAME)
#define vad_to_pfn64(x) (((quad_t)(x) >> PG_SHIFT) & PG_FRAME)
#define pfn_to_pad(x) (((x) & PG_FRAME) << PG_SHIFT)
#define vad_to_vpn(x) ((unsigned)(x) & PG_SVPN)
#define vpn_to_vad(x) ((x) & PG_SVPN)
/* User viritual to pte page entry */
#define uvtopte(adr) (((adr) >> PGSHIFT) & (NPTEPG -1))

#define	PG_SIZE_4K	0x00000000
#define	PG_SIZE_16K	0x00006000
#define	PG_SIZE_64K	0x0001e000
#define	PG_SIZE_256K	0x0007e000
#define	PG_SIZE_1M	0x001fe000
#define	PG_SIZE_4M	0x007fe000
#define	PG_SIZE_16M	0x01ffe000

#define LEAF(name) \
  	.text; \
  	.globl	name; \
  	.ent	name; \
name:

#define XLEAF(name) \
  	.text; \
  	.globl	name; \
  	.aent	name; \
name:

#define WLEAF(name) \
  	.text; \
  	.weakext name; \
  	.ent	name; \
name:

#define SLEAF(name) \
  	.text; \
  	.ent	name; \
name:

#define END(name) \
  	.size name,.-name; \
  	.end	name

#define SEND(name) END(name)
#define WEND(name) END(name)

#define EXPORT(name) \
  	.globl name; \
  	.type name,@object; \
name:

#define EXPORTS(name,sz) \
  	.globl name; \
  	.type name,@object; \
  	.size name,sz; \
name:

#define WEXPORT(name,sz) \
  	.weakext name; \
  	.type name,@object; \
  	.size name,sz; \
name:

#define	IMPORTS(name, size) \
	.extern	name,size

#define BSS(name,size) \
	.comm	name,size

#define LBSS(name,size) \
  	.lcomm	name,size

#define PHYS_TO_UNCACHED(addr)  ((addr)|0xA0000000)
#define UNCACHED_TO_PHYS(addr)  ((addr)&0x1FFFFFFF)

#define PHYS_TO_CACHED(addr)    ((addr)|0x80000000)
#define CACHED_TO_PHYS(addr)    ((addr)&0x1FFFFFFF)

#else

#define PHYS_TO_UNCACHED(addr)  PHYS_TO_K1(addr)
#define UNCACHED_TO_PHYS(addr)  K1_TO_PHYS(addr)

#define PHYS_TO_CACHED(addr)    PHYS_TO_K0(addr)
#define CACHED_TO_PHYS(addr)    K0_TO_PHYS(addr)
 
#endif

#define TOTAL_MEM_SIZE 0x20000000 /* total onboard memory size 512MB*/
#define DOWN_MEM_BASE_PHYS_ADDR 0x00000000
#define DOWN_MEM_SIZE 0x10000000 
#define UP_MEM_SIZE (TOTAL_MEM_SIZE - DOWN_MEM_SIZE)
#define UP_MEM_BASE_PHYS_ADDR 0x20000000

/* DDR - sdCfg */
#define	BONITO_SDCFG_TRC_SHIFT			0
#define BONITO_SDCFG_TRP_SHIFT			2
#define BONITO_SDCFG_TWR_SHIFT			3
#define BONITO_SDCFG_TCAS_SHIFT			4
#define BONITO_SDCFG_TRAS_SHIFT			6
#define BONITO_SDCFG_TRFC_SHIFT			7
#define BONITO_SDCFG_TRCD_SHIFT			9
#define BONITO_SDCFG_TREF_SHIFT			10
#define BONITO_SDCFG_DDRTYPE_SHIFT		22
#define BONITO_SDCFG_ISSEQ_SHIFT		26
#define BONITO_SDCFG_DIMM_MOD_NUM_SHIFT	27

/*** memeory initialization use macro,not smbbus ***/
/*bit 31: DDR配置结束标志，1表示结束，只读*/

/*bit 30 选择数据来源，0: 双沿采样；1：DQS采样*/
#define DDR_DQS_SELECT	(0<<30)

/*bit 29 标识DIMM_slot0是否插有内存条 0：无；1：有；*/
#define DDR_DIMM_DIC (1<<29)  

/*bit 28:27 DIMM0/DIMM1上MOUDLE的数目：
	2’b00：DIMM1: 1; DIMM0: 1
	2’b01：DIMM1: 1; DIMM0: 2
	2’b10：DIMM1: 2; DIMM0: 1
	2’b11：DIMM1: 2; DIMM0: 2
*/
#define DDR_DIMM_MODULE_NUM (3<<27)    

/*bit 26 义突发式读写时的块内顺序，
	1’b0：顺序
	1’b1：交替，现在只支持交替方式*/								
#define DDR_IS_SEQ  (1<<26)

/*bit 25:22 表2：DDR 控制器所支持的DDR SDRAM 计芯片类型
BITS Density Org.  Row Addr.  Col Addr.
0000 64Mb 16Mb X 4 DA[11:0] DA[9:0]
	128Mb 16Mb X 8
0001 64Mb 8Mb X 8 DA[11:0] DA[8:0]
	128Mb 8Mb X 16 
0010 64Mb 4Mb X 16 DA[11:0] DA[7:0]
	0011 128Mb 32Mb X 4 DA[11:0] DA[11],DA[9:0]
0100 256Mb 64Mb X 4 DA[12:0] DA[11],DA[9:0]
	512Mb 64Mb X 8 
0101 256Mb 32Mb X 8 DA[12:0] DA[9:0]
	 512Mb 32Mb X 16
0110 256Mb 16Mb X 16 DA[12:0] DA[8:0]
0111 512Mb 128Mb X 4 DA[12:0] DA[12:11],DA[9:0]
1000 1Gb 256Mb X 4 DA[13:0] DA[12:11],DA[9:0]
1001 1Gb 128Mb X 8 DA[13:0] DA[11],DA[9:0]
1010 1Gb 64Mb X 16 DA[13:0] DA[9:0]
*/
#define DDR_TYPE (5<<22)

/*bit 21:10 SDRAM刷新操作之间计数（主频100MHz）：
	780       7.8us
	1560      15.6us
	SDRAM刷新操作之间计数（主频133MHz）：
	1040      7.8us
	2080      15.6us
	SDRAM刷新操作之间计数（主频166MHz）：
	1300      7.8us
	2600      15.6us
*/ 
#define DDR_tREF  (100<<10)

/*bit 9 行地址有效到列地址有效之间需经过的计数
	1’b0   2 cycles（DDR100）
	1’b1   3 cycles（DDR266、DDR333）*/ 
#define DDR_TRCD (1<<9)

/*bit 8:7 AUTO_REFRESH到ACTIVE之间需经过的计数
	2’b00  Null 
	2’b01  8 cylces （DDR100）
	2’b10  10 cycles（DDR266）
	2’b11  12 cycles（DDR333）
*/ 
#define DDR_TRPC (1<<7)

/*bit 6 ACTIVE到PRECHARGE之间需经过的计数
	1’b0   5 cycles（DDR100）
	1’b1   7 cycles（DDR266、DDR333）  
*/ 
#define DDR_TRAS (0<<6) 

/*bit 5:4 从读命令到第一个数据到来需经过的计数
	2’b00  1.5 cycles
	2’b01  2 cycles
	2’b10  2.5 cycles
	2’b11  3 cycles
*/
#define DDR_TCAS (3<<4)

/*bit 3 写操作最后一个数据到PRECHARGE之间需经过的计数
	1’b0   2 cycles（DDR100）
	1’b1   3 cycles（DDR266、DDR333）
*/ 
#define DDR_TWR (0<<3)

/*bit 2 PRECHARGE命令执行时间计数
	1’b0   2 cycles（DDR100）
	1’b1   3 cycles（DDR266、DDR333）
*/ 
#define DDR_TRP (1<<2)

/*bit 1:0 ACTIVE与ACTIVE/AUTO_REFRESH命令之间计数
	2’b00  Null
	2’b01  7 cycles（DDR100）
	2’b10  9 cycles（DDR266）
	2’b11  10cycles（DDR333）
	注（由于precharge和ras cas的延时加起来正好满足这个延时，
	所以在DDR控制器里没有具体考虑这个参数）
*/ 
#define DDR_TRC (1<<0) 

#define SDCFG_DATA (DDR_DQS_SELECT|      \
	                DDR_DIMM_DIC|        \
				    DDR_DIMM_MODULE_NUM| \
				    DDR_IS_SEQ|          \
					DDR_TYPE|            \
					DDR_tREF|            \
					DDR_TRCD|            \
					DDR_TRPC|            \
					DDR_TRAS|            \
					DDR_TCAS|            \
					DDR_TWR|             \
					DDR_TRP|             \
					DDR_TRC)

#define SDSPCFG_VAR_0 0x00 /*1 clock delayed to sample after DQS valide*/
#define SDSPCFG_VAR_1 0x01 /*2 clock delayed to sample after DQS valide*/
#define SDSPCFG_VAR_2 0x02 /*3 clock delayed to sample after DQS valide*/

#define REG_SDCFG_BASE_ADDR     0x1ff00000  /*sdcfg baseaddr*/							
#define REG_SDCFG_ADDR 			0x1ff00008  /*DDR SDRAM parameter configuration reg*/
#define REG_SDSPCFG_ADDR 		0x1ff00030  /*sample point configuration reg*/

#define REG_SDWDCFG_BASE0_ADDR  0x1ff00010 /*memory window 0 baseaddr reg*/
#define REG_SDWDCFG_SIZE0_ADDR  0x1ff00018 /*memory window 0 size reg*/  
#define REG_SDWDCFG_BASE1_ADDR  0x1ff00020 /*memory window 1 baseaddr reg*/
#define REG_SDWDCFG_SIZE1_ADDR  0x1ff00028 /*memory window 1 size reg*/

#define OFFSET_SDCFG 		  (REG_SDCFG_ADDR - REG_SDCFG_BASE_ADDR)  /*DDR SDRAM parameter configuration reg offset*/
#define OFFSET_SDSPCFG 		  (REG_SDSPCFG_ADDR - REG_SDCFG_BASE_ADDR)  /*sample point configuration reg offset*/
#define OFFSET_SDWDCFG_BASE0  (REG_SDWDCFG_BASE0_ADDR - REG_SDCFG_BASE_ADDR) /*memory window 0 baseaddr reg offset*/
#define OFFSET_SDWDCFG_SIZE0  (REG_SDWDCFG_SIZE0_ADDR - REG_SDCFG_BASE_ADDR) /*memory window 0 size reg offset*/  
#define OFFSET_SDWDCFG_BASE1  (REG_SDWDCFG_BASE1_ADDR - REG_SDCFG_BASE_ADDR) /*memory window 1 baseaddr reg offset*/
#define OFFSET_SDWDCFG_SIZE1  (REG_SDWDCFG_SIZE1_ADDR - REG_SDCFG_BASE_ADDR) /*memory window 1 size reg offset*/

#define REG_SDCFG_BASE_ADDR_UCA PHYS_TO_UNCACHED(REG_SDCFG_BASE_ADDR)
#define REG_SDCFG_ADDR_UCA      PHYS_TO_UNCACHED(REG_SDCFG_ADDR) 
#define REG_SDSPCFG_ADDR_UCA    PHYS_TO_UNCACHED(REG_SDSPCFG_ADDR)

#define REG_SDWDCFG_BASE0_ADDR_UCA  PHYS_TO_UNCACHED(REG_SDWDCFG_BASE0_ADDR) 
#define REG_SDWDCFG_SIZE0_ADDR_UCA  PHYS_TO_UNCACHED(REG_SDWDCFG_SIZE0_ADDR) 
#define REG_SDWDCFG_BASE1_ADDR_UCA  PHYS_TO_UNCACHED(REG_SDWDCFG_BASE1_ADDR) 
#define REG_SDWDCFG_SIZE1_ADDR_UCA  PHYS_TO_UNCACHED(REG_SDWDCFG_SIZE1_ADDR)

#endif 






