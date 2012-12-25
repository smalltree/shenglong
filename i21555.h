/* i21555.h - Intel 21554/21555 PCI-to-PCI Bridge Interface Controller */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */

/*
modification history
--------------------
01d,10may02,tlc  Add C++ header protection.
01c,20mar02,agf  revalidation code review results
01b,16jul01,agf  add CoE notice
01a,26sep00,bvn	 created.
*/


#ifndef	__INCi21555h
#define	__INCi21555h

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file contains constants for the I21555 PCI-to-PCI Interface 
 * controller.  
 */

#ifdef	_ASMLANGUAGE
#define CASTINT
#else
#define CASTINT (volatile unsigned int *)
#endif	/* _ASMLANGUAGE */

#define LTABLE_SIZE		64
#define LTABLE_MASK		0xf000000
#define LTABLE_SHFT		24
#define LTABLE_LUT_SEL		(1 << 12)
#define LTABLE_DATA_RESERVED	0xf6
#define LTABLE_DATA_CTRL_MASK	0x5
#define LTABLE_DATA_CTRL_PF	0x8
#define LTABLE_DATA_CTRL_EN	0x1
#define LTABLE_DATA_BASE_MASK	0xffffff00
#define PRIMARY_ACCESS_LOCKOUT	(1 << 10)
#define CSR_SIZE		0x1000
#define SUB_HEADER_OFFSET	0x40
#define REG_BAR_SIZE_MASK	0xFFFFFFC0
#define REG_BAR_RESERVED	0x00000030
#define REG_SETUP_RESERVED	0x00000030
#define REG_TRAN_RESERVED	0x0000003F
#define REG_SETUP_ENABLE	0x80000000
#define REG_SETUP_SIZE_MASK	0x7FFFFFE0
#define REG_SETUP_IO_SPACE	CPCI_IO_SPACE
#define REG_SETUP_MEM_SPACE	CPCI_MEM_SPACE
#define REG_SETUP_PREFETCH	CPCI_PF_MEM_SPACE

#define CFG_HDR2(x)		(x < 0x40 ? (SUB_HEADER_OFFSET + x): x)

/* For use with translation calls */
#define CPCI_MEM_SPACE		0	/* PCI Memory Space */
#define CPCI_IO_SPACE		1	/* PCI I/O Space  */
#define CPCI_PF_MEM_SPACE	8	/* PCI Prefetchable Memory Space */
#define CPCI_CNFG_SPACE		3	/* PCI Configuration Space */

/* Compact PCI interrupt level definitions */
#define CPCI_INTA		0	/* INTA interrupt */
#define CPCI_INTB		1	/* INTB interrupt */
#define CPCI_INTC		2	/* INTC interrupt */
#define CPCI_INTD		3	/* INTD interrupt */

/* Window definitions */
#define I21555_IO_WINDOW	D_MEM1_BAR	/* I/O Window */	
#define I21555_MEM_WINDOW1	D_MEM0_BAR	/* MEM Window 1 */ 
                                                /* Not this window also places */
                                                /* the CSR in the lower 4K */
                                                /* If setting up the       */
                                                /* primary interface       */
#define I21555_MEM_WINDOW2	D_MEM2_BAR	/* MEM Window 2 */ 
#define I21555_MEM_WINDOW3	D_MEM3_BAR 	/* MEM Window 3 */	

#define IS_SECONDARY_INTERFACE	0
#define IS_PRIMARY_INTERFACE	1

#define MAILBOXSIZE		256
#define MAILBOX_INT_MASK	(MAILBOXSIZE - 1)

/* Status of window setting routines */
#define BRIDGE_WIN_NOT_ENABLED		1   /* When Primary is on local bus */
                                            /* and setting up for CPCI access */
#define BRIDGE_WIN_SIZE_TOO_LARGE	2   /* Not enough local PCI memory */
                                            /* resource to map the window */
#define BRIDGE_INVALID_INTERFACE	3   /* Routine called cannot map */
                                            /* window because the interface */
                                            /* is not on the right side */
                                            /* See functions :          */
                                            /* sysI21555MasterSSet(),   */
                                            /* sysI21555MasterPSet(),   */
                                            /* sysI21555SlavePSet(),    */
                                            /* sysI21555SlaveSSet(),    */
#define BRIDGE_INVALID_WINDOW		4   /* Invalid window parameter */
#define BRIDGE_INVALID_SIZE		5   /* Size is not a power of 2 */
#define BRIDGE_INVALID_BASE_SIZE	6   /* <baseAddr> & (<size> - 1) != 0 */

/*
 * I21555 Configuration Register definitions
 */
#define		VEN_DEV_ID	0x0000		/* Device/Vendor ID */
#define		COMMAND		0x0004		/* Command/Status */
#define		REVISION	0x0008		/* Revision/Class Code */
#define		BIST		0x000C		/* BiST/CLS/MLT/Header Type */

#define		D_MEM0_BAR	0x0010		/* Primary CSR/Memory 0 BAR */
#define		P_CSR_MEM_BAR	D_MEM0_BAR	/* Primary CSR/Memory 0 BAR */
#define		P_CSR_IO_BAR	0x0014		/* Primary CSR I/O BAR */
#define		D_MEM1_BAR	0x0018		/* Downstream Memory 1 BAR */
#define		D_MEM2_BAR	0x001C		/* Downstream Memory 2 BAR */
#define		D_MEM3_BAR	0x0020		/* Downstream Memory 3 BAR */
#define		D_MEM3U_BAR	0x0024		/* Downstream Memory 3 BAR Upper 32 bits */
#define		SUBSYSTEM_ID	0x002C		/* Subsystem (Vendor) ID */
#define		P_EROM_BAR	0x0030		/* Primary Expansion ROM BAR */
#define		ENC_CAP_PTR	0x0034		/* Enhanced Capabilities Ptr */
#define		INTERRUPT	0x003C		/* Int MIN_GNT/MAX_LAT */

#define		S_CSR_MEM_BAR	D_MEM0_BAR     	/* Secondary CSR MEM BAR */
#define		S_CSR_IO_BAR	P_CSR_IO_BAR   	/* Secondary CSR I/O BAR */
#define		U_MEM0_BAR	D_MEM1_BAR   	/* Upstream MEM 0 BAR */
#define		U_IO_BAR	U_MEM0_BAR   	/* Upstream I/O BAR */
#define		U_MEM1_BAR	D_MEM2_BAR   	/* Upstream MEM 1 BAR */
#define		U_MEM2_BAR	D_MEM3_BAR   	/* Upstream MEM 2 BAR */

#define		D_CONF_ADDR	0x0080		/* Downstream Config Addr */
#define		D_CONF_DATA	0x0084		/* Downstream Config Data */
#define		U_CONF_ADDR	0x0088		/* Upstream Config Addr */
#define		U_CONF_DATA	0x008C		/* Upstream Config Data */

#define		CONFIG_OWN	0x0090		/* Config Own/CSR       */
#define		D_MEM0_TRAN	0x0094		/* Downstream MEM 0 Translated Base */
#define		D_MEM1_TRAN	0x0098		/* Downstream MEM 1 Translated Base */
#define		D_IO_TRAN	D_MEM1_TRAN	/* Downstream I/O Translated Base */
#define		D_MEM2_TRAN	0x009C		/* Downstream MEM 2 Translated Base */
#define		D_MEM3_TRAN	0x00A0		/* Downstream MEM 3 Translated Base */
#define		U_MEM0_TRAN	0x00A4		/* Upstream MEM 0 Translated Base */
#define		U_IO_TRAN	U_MEM0_TRAN	/* Upstream I/O Translated Base */
#define		U_MEM1_TRAN	0x00A8		/* Upstream MEM 1 Translated Base */
#define		D_MEM0_SETUP	0x00AC		/* Downstream MEM 0 Setup */
#define		D_MEM1_SETUP	0x00B0		/* Downstream MEM 1 Setup */
#define		D_IO_SETUP	D_MEM1_SETUP	/* Downstream I/O Setup */
#define		D_MEM2_SETUP	0x00B4		/* Downstream MEM 2 Setup */
#define		D_MEM3_SETUP	0x00B8		/* Downstream MEM 3 Setup */
#define		D_MEM3U_SETUP	0x00BC		/* Downstream MEM 3 Upper 32 Bits Setup */
#define		P_EROM_SETUP	0x00C0		/* Pimary Expansion ROM Setup */
#define		U_MEM0_SETUP	0x00C4		/* Upstream MEM 0 Setup */
#define		U_IO_SETUP	U_MEM0_SETUP	/* Upstream I/O Setup */
#define		U_MEM1_SETUP	0x00C8		/* Upstream MEM 0 Setup */
#define		CHIP_CONTROL	0x00CC		/* Chip Control 0/1 */
#define		CHIP_STATUS	0x00D0		/* Chip Status/Arbiter Control */
#define		SERR_DISABLE	0x00D4		/* Primary/Secondary SERR# Disable*/
#define		RESET_CONTROL	0x00D8		/* Reset Control */
#define		POWER_CAP	0x00DC		/* Power Management ECP ID/Next Ptr/Capabilities */
#define		POWER_CTRL	0x00E0		/* Power Management Control/Status */
#define		VPD_SETUP	0x00E4		/* VPD Cap ID/Next Ptr/Address */
#define		VPD_DATA	0x00E8		/* VPD Data */
#define		HOT_SWAP	0x00EC		/* Hot-Swap Cap ID/Nex Ptr/Control */


/* CSR Address Map */
typedef volatile struct i21154_csr {
 /* 3:0 */	UINT	downConfigAdr;	/* Downstream Configuration Address */
 /* 7:4 */	UINT	downConfigDat;	/* Downstream Configuration Data */
 /* b:8 */	UINT	upConfigAdr;	/* Upstream Configuration Address */
 /* f:c */	UINT	upConfigDat;	/* Upstream Configuration Data */
 /* 13:10 */	UINT	configOwn;	/* Configuration Own Byte 0 */
 /* 17:14 */	UINT	downIOAdr;	/* Downstream I/O Address */
 /* 1b:18 */	UINT	downIODat;	/* Downstream I/O Data */
 /* 1f:1c */	UINT	upIOAdr;	/* Upstream I/O Address */
 /* 23:20 */	UINT	upIODat;	/* Upstream I/O Data */
 /* 27:24 */	UINT	IOOwn;		/* I/O Own */
 /* 2b:28 */	UINT	tableOffset;	/* Look-up Table Offset (first 8 bits) */
 /* 2f:2c */	UINT	tableData;	/* Look-up Table Data */
 /* 33:30 */	UINT	i2oOPS;		/* I2O Outbound Post Status */
 /* 37:34 */	UINT	i2oOPM;		/* I2O Outbound Post Mask */
 /* 3b:38 */	UINT	i2oIPS;		/* I2O Inbound Post Status */
 /* 3f:3c */	UINT	i2oIPM;		/* I2O Inbound Post Mask */
 /* 43:40 */	UINT	i2oIQ;		/* I2O Inbound Queue */
 /* 47:44 */	UINT	i2oOQ;		/* I2O Outbound Queue */
 /* 4b:48 */	UINT	i2oIFHP;	/* I2O Inbound Free Head Pointer */
 /* 4f:4c */	UINT	i2oIPTP;	/* I2O Inbound Post Tail Pointer */
 /* 53:50 */	UINT	i2oOFTP;	/* I2O Outbound Free Tail Pointer */
 /* 57:54 */	UINT	i2oOPHP;	/* I2O Outbound Post Head Pointer */
 /* 5b:58 */	UINT	i2oIPC;		/* I2O Inbound Post Counter */
 /* 5f:5c */	UINT	i2oIFC;		/* I2O Inbound Free Counter */
 /* 63:60 */	UINT	i2oOPC;		/* I2O Outbound Post Counter */
 /* 67:64 */	UINT	i2oOFC;		/* I2O Outbound Free Counter */
 /* 6b:68 */	UINT	downMem0Tran;	/* Downstream Memory 0 Translated Base */
 /* 6f:6c */	UINT	downMem1Tran;	/* Downstream IO or Memory 1 Translated Base */
 /* 73:70 */	UINT	downMem2Tran;	/* Downstream Memory 2 Translated Base */
 /* 77:74 */	UINT	downMem3Tran;	/* Downstream Memory 3 Translated Base */
 /* 7b:78 */	UINT	upMem0Tran;	/* Upstream IO or Memory 0 Translated Base */
 /* 7f:7c */	UINT	upMem1Tran;	/* Upstream Memory 1 Translated Base */
 /* 83:80 */	UINT	chipStatus;	/* Chip Control/Status CSR */
 /* 85:84 */	UINT	chipIRQMask;	/* Chip Set/Clear IRQ Mask */
 /* 8b:88 */	UINT	upPageBoundIRQ0;/* Upstream Page Boundary IRQ 0 */
 /* 8f:8c */	UINT	upPageBoundIRQ1;/* Upstream Page Boundary IRQ 1 */
 /* 93:90 */	UINT	upPageBoundMsk0;/* Upstream Page Boundary IRQ Mask 0 */
 /* 97:94 */	UINT	upPageBoundMsk1;/* Upstream Page Boundary IRQ Mask 1 */
 /* 9b:98 */	UINT	clrIRQ;		/* Primary/Secondary Clear IRQ */
 /* 9d:9c */	UINT	setIRQ;		/* Primary/Secondary Set IRQ */
 /* a3:a0 */	UINT	clrIRQMask;	/* Primary/Secondary Clear IRQ mask */
 /* a7:a4 */	UINT	setIRQMask;	/* Primary/Secondary Set IRQ mask */
 /* ab:a8 */	UINT	scratchpad0;	/* Scratchpad 0 */
 /* af:ac */	UINT	scratchpad1;	/* Scratchpad 1 */
 /* b3:b0 */	UINT	scratchpad2;	/* Scratchpad 2 */
 /* b7:b4 */	UINT	scratchpad3;	/* Scratchpad 3 */
 /* bb:b8 */	UINT	scratchpad4;	/* Scratchpad 4 */
 /* bf:bc */	UINT	scratchpad5;	/* Scratchpad 5 */
 /* c3:c0 */	UINT	scratchpad6;	/* Scratchpad 6 */
 /* c7:c4 */	UINT	scratchpad7;	/* Scratchpad 7 */
 /* cb:c8 */	UINT	romSetup;	/* ROM Setup */
 /* cf:cc */	UINT	romCtrl;	/* ROM Address/Control */
 /* ff:d0 */	UINT	reserved2[12];	/* Reserved */
 /* 1ff:100 */	UINT	upMem2Tbl[64];	/* Upstream Memory 2 Look-up Table */
 /* fff:200 */	UINT	reserved3[896];	/* Reserved */
} I21555_CSR;

#ifdef __cplusplus
}
#endif
#endif    /* __INCi21555h */
