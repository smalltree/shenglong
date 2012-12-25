/* Malta.h - Malta platform header */

/* Copyright 2000-2001 Wind River Systems, Inc. */

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
01k,18sep02,jmt  Modified to support 5kf
01j,02nov01,mdg  Add support for 4kec processor. Add ATA/IDE support. Add PCI
                 Autoconfig Boot-Only Support.
01i,14aug01,pes  Update Interrupt vector macros
01h,06aug01,pes  Add masks to DEFAULT_SR to prevent unwanted interrupts from
                 non-existant sources (CoreHI and CoreLO).
01g,16jul01,pes  Add CoE Copyright comment
01f,28jun01,tlc  Add INITIAL_SR macro.
01e,15jun01,pes  Lots of cleanup work. Rearrangement, adding numerous
                 constants to support new functionality, removal of unused
                 definitions.
01d,02may01,pes  Numerous cleanups and added definitions.
01c,11apr01,pes  Initial Checkin/Development.
01b,15jan01,zmm  Vortex updates
01a,11sep00,dra	 Written from BSP provided by MIPS
*/

/*
This file contains I/O addresses and related constants for the
Malta Platform Board.  From the Malta(TM) User's Manual:

    The design is composed of two parts, the Malta Board, which
    is 6U CompactPCI form factor and holds the CPU-independent
    parts of the ircuitry, and a "Core" Card, which holds the
    CPU plus its System Controller and fast SDRAM memory.

This file does not contain definitions for the Core Card.
*/

#ifndef __INCmaltah
#define __INCmaltah
#ifdef __cplusplus
extern "C" {
#endif	 /* __cplusplus */

/* #define BUS			BUS_TYPE_PCI */	/* no off-board bus interface */

#undef	INCLUDE_EGL	/* configAll.h defines this for MIPS */

/* override default value of 32 pci devices */
/* #define PCI_MAX_DEV		22 *//* last device number on GT64120 chip */

/* config reg value to be written at cold reset */

#define KSEG0_CACHE_MODE	3		/* cached */

/* initial status register */
/*
#define INITIAL_SR              (SR_CU0 | SR_BEV)
*/
/* mask off core card interrupts */
/*
#define DEFAULT_SR		(SR_CU1 | SR_FR | (SR_IMASK0 & \
					   ~(SR_IBIT7 | SR_IBIT6)) | SR_IE)
*/
/* Register Manipulating Definitions */
#if (_BYTE_ORDER == _BIG_ENDIAN)
#define GTREG(X)		LONGSWAP(X)
#else /* BYTE_ORDER != _BIG_ENDIAN */
#define GTREG(X)		(X)
#endif /* BYTE_ORDER */

/* interrupt priority */

#define INT_PRIO_MSB		TRUE	/* interrupt priority msb highest */

/* Mask Generation Macro */
#ifndef MSK
#define MSK(n)			  ((1 << (n)) - 1)
#endif

/* Byte & word swaps */

#if (_BYTE_ORDER == _BIG_ENDIAN)
/* swap byte within a word */
#define PBYTESWAP(x)		(MSB(x) | (LSB(x) << 8))
#define PSWAP(x)		LONGSWAP(x)	/* swap the long word */
#else /* _BYTE_ORDER == _LITTLE_ENDIAN */
#define PBYTESWAP(x)		(x)
#define PSWAP(x)		(x)
#endif /* _BYTE_ORDER == _BIG_ENDIAN */

/************************************************************************
 *  Malta physical address MAP (512 MByte)
*************************************************************************/

#define MALTA_SYSTEMRAM_BASE      0x00000000  /* System RAM:          */
#define MALTA_SYSTEMRAM_SIZE      0x08000000  /*   128 MByte          */
 
#define MALTA_PCIMEM0_BASE        0x08000000  /* PCI 0 memory:        */
#define MALTA_PCIMEM0_SIZE        0x08000000  /*   128 MByte          */
 
#define MALTA_PCIIO0_BASE         0x1fd00000  /* PCI 0 I/O:           */
#define MALTA_PCIIO0_SIZE         0x00100000  /*   64 MByte           */
#define MALTA_PCIIO0_16BIT_OFFSET 0x4000      /* Start 16-bit I/O at  */
					      /* 16K                  */
#define MALTA_PCIIO0_16BIT_SIZE   0x4000      /* Allow 16k            */

#define MALTA_PCIIO0_32BIT_OFFSET 0x8000      /* Start 32-bit I/O at  */
					      /* 32K                  */
#define MALTA_PCIIO0_32BIT_SIZE   0x8000      /* Allow 32K            */

#define MALTA_CORECTRL_BASE       0x14000000  /* Core control:        */
#define MALTA_CORECTRL_SIZE       0x00001000  /*     4 KByte          */

#define MALTA_RESERVED_BASE1      0x1C000000  /* Reserved:            */
#define MALTA_RESERVED_SIZE1      0x02000000  /*    48 MByte          */

#define MALTA_RESERVED_BASE2      0x1E400000  /* Reserved:            */
#define MALTA_RESERVED_SIZE2      0x00C00000  /*    12 MByte          */

#define MALTA_FPGA_BASE           0x1F000000  /* FPGA:                */
#define MALTA_FPGA_SIZE           0x00C00000  /*    12 MByte          */

#define MALTA_BOOTROM_BASE        0x1FC00000  /* Boot ROM:            */
#define MALTA_BOOTROM_SIZE        0x00400000  /*     4 MByte          */

/************************************************************************
 *  Malta FPGA, register address map:
 *  REVISION: FIXED AT 0x1FC00010 on any Malta baseboard
*************************************************************************/

#define MALTA_REVISION            0x1FC00010 /* REVISION              */
#define MALTA_SWITCH              0x1F000200 /* SWITCH                */
#define MALTA_STATUS              0x1F000208 /* STATUS                */
#define MALTA_JMPRS               0x1F000210 /* JMPRS                 */
#define MALTA_NMISTATUS           0x1F000024 /* NMISTATUS	      */
#define MALTA_NMIACK              0x1F000104 /* NMIACK                */
#define MALTA_LEDBAR              0x1F000408 /* LEDBAR       bit 7:0  */
#define MALTA_ASCIIWORD           0x1F000410 /* ASCIIWORD    bit 32:0 */
#define MALTA_ASCIIPOS0           0x1F000418 /* ASCIIPOS0    bit 7:0  */
#define MALTA_ASCIIPOS1           0x1F000420 /* ASCIIPOS1    bit 7:0  */
#define MALTA_ASCIIPOS2           0x1F000428 /* ASCIIPOS2    bit 7:0  */
#define MALTA_ASCIIPOS3           0x1F000430 /* ASCIIPOS3    bit 7:0  */
#define MALTA_ASCIIPOS4           0x1F000438 /* ASCIIPOS4    bit 7:0  */
#define MALTA_ASCIIPOS5           0x1F000440 /* ASCIIPOS5    bit 7:0  */
#define MALTA_ASCIIPOS6           0x1F000448 /* ASCIIPOS6    bit 7:0  */
#define MALTA_ASCIIPOS7           0x1F000450 /* ASCIIPOS7    bit 7:0  */
#define MALTA_SOFTRES             0x1F000500 /* SOFTRES               */
#define MALTA_BRKRES              0x1F000508 /* BRKRES                */
#define MALTA_GPOUT               0x1F000A00 /* GPOUT                 */
#define MALTA_GPINP               0x1F000A08 /* GPINP                 */
#define MALTA_I2CINP              0x1F000b00 /* I2CINP                */
#define MALTA_I2COE               0x1F000b08 /* I2COE                 */
#define MALTA_I2COUT              0x1F000b10 /* I2COUT                */
#define MALTA_I2CSEL		  0x1F000b18 /* I2CSEL		      */


/************************************************************************
 *      Register field encodings
*************************************************************************/

/******** reg: REVISION ********/

/* field: FPGRV */
#define MALTA_REVISION_FPGRV_SHF	16
#define MALTA_REVISION_FPGRV_MSK	(MSK(8) << MALTA_REVISION_FPGRV_SHF)

/* field: CORID */
#define MALTA_REVISION_CORID_SHF	10
#define MALTA_REVISION_CORID_MSK	(MSK(6) << MALTA_REVISION_CORID_SHF)

/* field: CORRV */
#define MALTA_REVISION_CORRV_SHF	8
#define MALTA_REVISION_CORRV_MSK	(MSK(2) << MALTA_REVISION_CORRV_SHF)

/* field: PROID */
#define MALTA_REVISION_PROID_SHF	4
#define MALTA_REVISION_PROID_MSK	(MSK(4) << MALTA_REVISION_PROID_SHF)

/* field: PRORV */
#define MALTA_REVISION_PRORV_SHF	0
#define MALTA_REVISION_PRORV_MSK	(MSK(4) << MALTA_REVISION_PRORV_SHF)


/******** reg: NMISTATUS ********/

/* bit 1: SOUTHBRIDGE */
#define MALTA_NMISTATUS_SB_SHF		1
#define MALTA_NMISTATUS_SB_MSK		(MSK(1) << MALTA_NMISTATUS_SB_SHF)
#define MALTA_NMISTATUS_SB_BIT		MALTA_NMISTATUS_SB_MSK

/* bit 0: PUSHBUTTON */
#define MALTA_NMISTATUS_ONNMI_SHF	0
#define MALTA_NMISTATUS_ONNMI_MSK	(MSK(1) << MALTA_NMISTATUS_ONNMI_SHF)
#define MALTA_NMISTATUS_ONNMI_BIT	MALTA_NMISTATUS_ONNMI_MSK


/******** reg: NMIACK ********/

/* bit 0: NMI */
#define MALTA_NMIACK_ONNMI_SHF		0
#define MALTA_NMIACK_ONNMI_MSK		(MSK(1) << MALTA_NMIACK_ONNMI_SHF)
#define MALTA_NMIACK_ONNMI_BIT		MALTA_NMIACK_ONNMI_MSK


/******** reg: SWITCH ********/

/* bits 7:0: S2 */
#define MALTA_SWITCH_S2_SHF		0
#define MALTA_SWITCH_S2_MSK		(MSK(8) << MALTA_SWITCH_S2_SHF)


/******** reg: STATUS ********/

/* bit 4 : MFWR */
#define MALTA_STATUS_MFWR_SHF		4
#define MALTA_STATUS_MFWR_MSK		(MSK(1) << MALTA_STATUS_MFWR_SHF)
#define MALTA_STATUS_MFWR_BIT		MALTA_STATUS_MFWR_MSK

/* bit 3 : S54 */
#define MALTA_STATUS_S54_SHF		3
#define MALTA_STATUS_S54_MSK		(MSK(1) << MALTA_STATUS_S54_SHF)
#define MALTA_STATUS_S54_SET		MALTA_STATUS_S54_MSK

/* bit 2 : S53 */
#define MALTA_STATUS_S53_SHF		2
#define MALTA_STATUS_S53_MSK		(MSK(1) << MALTA_STATUS_S53_SHF)
#define MALTA_STATUS_S53_SET		MALTA_STATUS_S53_MSK

/* bit 1: BIGEND */
#define MALTA_STATUS_BIGEND_SHF		1
#define MALTA_STATUS_BIGEND_MSK		(MSK(1) << MALTA_STATUS_BIGEND_SHF)
#define MALTA_STATUS_BIGEND_BIT		MALTA_STATUS_BIGEND_MSK


/******** reg: JMPRS ********/

/* bit 4:2: PCICLK */
#define MALTA_JMPRS_PCICLK_SHF		2
#define MALTA_JMPRS_PCICLK_MSK		(MSK(3) << MALTA_JMPRS_PCICLK_SHF)
#define MALTA_JMPRS_PCICLK_10MHZ	7
#define MALTA_JMPRS_PCICLK_12_5MHZ	4
#define MALTA_JMPRS_PCICLK_16_67MHZ	5
#define MALTA_JMPRS_PCICLK_20MHZ	1
#define MALTA_JMPRS_PCICLK_25MHZ	2
#define MALTA_JMPRS_PCICLK_30MHZ	3
#define MALTA_JMPRS_PCICLK_33_33MHZ	0
#define MALTA_JMPRS_PCICLK_37_5MHZ	6

/* bit 1: EELOCK */
#define MALTA_JMPRS_EELOCK_SHF		1
#define MALTA_JMPRS_EELOCK_MSK		(MSK(1) << MALTA_JMPRS_EELOCK_SHF)
#define MALTA_JMPRS_EELOCK_BIT		MALTA_JMPRS_EELOCK_MSK


/******** reg: SOFTRES ********/

/* bits 7:0: RESET */
#define MALTA_SOFTRES_RESET_SHF		0
#define MALTA_SOFTRES_RESET_MSK		(MSK(8) << MALTA_SOFTRES_RESET_SHF)
#define MALTA_SOFTRES_RESET_GORESET	0x42 /* magic value to reset    */    


/******** reg: BRKRES ********/

/* bits 7:0: WIDTH */
#define MALTA_BRKRES_WIDTH_SHF		0
#define MALTA_BRKRES_WIDTH_MSK		(MSK(8) << MALTA_BRKRES_WIDTH_SHF)


/******** reg: GPOUT ********/

/* bits 7:0: OUTVAL */
#define MALTA_GPOUT_OUTVAL_SHF		0
#define MALTA_GPOUT_OUTVAL_MSK		(MSK(8) << MALTA_GPOUT_OUTVAL_SHF)


/******** reg: GPINP ********/

/* bits 7:0: INPVAL */
#define MALTA_GPINP_INPVAL_SHF		0
#define MALTA_GPINP_INPVAL_MSK		(MSK(8) << MALTA_GPINP_INPVAL_SHF)


/******** reg: I2CINP ********/

#define MALTA_I2CINP_I2CSCL_SHF		1
#define MALTA_I2CINP_I2CSCL_MSK		(MSK(1) << MALTA_I2CINP_I2CSCL_SHF)
#define MALTA_I2CINP_I2CSCL_BIT		MALTA_I2CINP_I2CSCL_MSK

#define MALTA_I2CINP_I2CSDA_SHF		0
#define MALTA_I2CINP_I2CSDA_MSK		(MSK(1) << MALTA_I2CINP_I2CSDA_SHF)
#define MALTA_I2CINP_I2CSDA_BIT		MALTA_I2CINP_I2CSDA_MSK


/******** reg: I2COE ********/

#define MALTA_I2COE_I2CSCL_SHF		1
#define MALTA_I2COE_I2CSCL_MSK		(MSK(1) << MALTA_I2COE_I2CSCL_SHF)
#define MALTA_I2COE_I2CSCL_BIT		MALTA_I2COE_I2CSCL_MSK

#define MALTA_I2COE_I2CSDA_SHF		0
#define MALTA_I2COE_I2CSDA_MSK		(MSK(1) << MALTA_I2COE_I2CSDA_SHF)
#define MALTA_I2COE_I2CSDA_BIT		MALTA_I2COE_I2CSDA_MSK


/******** reg: I2COUT ********/

#define MALTA_I2COUT_I2CSCL_SHF		1
#define MALTA_I2COUT_I2CSCL_MSK		(MSK(1) << MALTA_I2COUT_I2CSCL_SHF)
#define MALTA_I2COUT_I2CSCL_BIT		MALTA_I2COUT_I2CSCL_MSK

#define MALTA_I2COUT_I2CSDA_SHF		0
#define MALTA_I2COUT_I2CSDA_MSK		(MSK(1) << MALTA_I2COUT_I2CSDA_SHF)
#define MALTA_I2COUT_I2CSDA_BIT		MALTA_I2COUT_I2CSDA_MSK


/******** reg: I2CSEL ********/

#define MALTA_I2CSEL_FPGA_SHF		0
#define MALTA_I2CSEL_FPGA_MSK		(MSK(1) << MALTA_I2CSEL_FPGA_SHF)
#define MALTA_I2CSEL_FPGA_BIT		MALTA_I2CSEL_FPGA_MSK


/******** reg: CONFIG1 *******/
#define MALTA_CONFIG1_DA_SHF		7
#define MALTA_CONFIG1_DA_MSK		(MSK(3) << MALTA_CONFIG1_DA_SHF)
#define MALTA_CONFIG1_DL_SHF		10
#define MALTA_CONFIG1_DL_MSK		(MSK(3) << MALTA_CONFIG1_DL_SHF)
#define MALTA_CONFIG1_DS_SHF		13
#define MALTA_CONFIG1_DS_MSK		(MSK(3) << MALTA_CONFIG1_DS_SHF)

#define MALTA_CONFIG1_IA_SHF		16
#define MALTA_CONFIG1_IA_MSK		(MSK(3) << MALTA_CONFIG1_IA_SHF)
#define MALTA_CONFIG1_IL_SHF		19
#define MALTA_CONFIG1_IL_MSK		(MSK(3) << MALTA_CONFIG1_IL_SHF)
#define MALTA_CONFIG1_IS_SHF		22
#define MALTA_CONFIG1_IS_MSK		(MSK(3) << MALTA_CONFIG1_IS_SHF)

/************************************************************************
 *  Malta, EEPROM devices,  IIC-slave adresses
*************************************************************************/

#define MALTA_EEPROM_IICADR_SN		0x54 /* EEPROM with serial number   */
#define CORE_EEPROM_IICADR_SPD000	0x50 /* PC-SDRAM, 256 bytes         */

/************************************************************************
 *      Serial Presence Detect (SPD)
 *
 * Register offset addresses, access types & encodings 
 * for Serial Presence Detect (SPD). Accessed via SMBus (IIC).
 ************************************************************************/

#define SPD_ROWS			3
#define SPD_COL				4
#define SPD_MODULE_BANKS		5
#define SPD_MODULE_WIDTH		6
#define SPD_CONFIG_TYPE			11
#define SPD_RFSH_RT			12
#define SPD_SDRAM_WIDTH			13
#define SPD_EC_SDRAM			14
#define SPD_BURSTLEN			16
#define SPD_DEVICE_BANKS		17
#define SPD_CASLAT			18
#define SPD_ROW_DENSITY			31

#define SPD_ROWS_A_SHF			0
#define SPD_ROWS_A_MSK			(MSK(4) << SPD_ROWS_A_SHF)

#define SPD_COL_A_SHF			0
#define SPD_COL_A_MSK			(MSK(4) << SPD_COL_A_SHF)

#define SPD_CASLAT_2_SHF		1
#define SPD_CASLAT_2_MSK		(MSK(1) << SPD_CASLAT_2_SHF)
#define SPD_CASLAT_2_BIT		SPD_CASLAT_2_MSK

#define SPD_CASLAT_3_SHF		2
#define SPD_CASLAT_3_MSK		(MSK(1) << SPD_CASLAT_3_SHF)
#define SPD_CASLAT_3_BIT		SPD_CASLAT_3_MSK

#define SPD_BURSTLEN_8_SHF		3
#define SPD_BURSTLEN_8_MSK		(MSK(1) << SPD_BURSTLEN_8_SHF)
#define SPD_BURSTLEN_8_BIT		SPD_BURSTLEN_8_MSK

#define SPD_CONFIG_TYPE_NONE		0x0
#define SPD_CONFIG_TYPE_PARITY		0x1
#define SPD_CONFIG_TYPE_ECC		0x2

#define SPD_RFSH_RT_RATE_SHF		0
#define SPD_RFSH_RT_RATE_MSK		(MSK(7) << SPD_RFSH_RT_RATE_SHF)
#define SPD_RFSH_RT_RATE_125		5
#define SPD_RFSH_RT_RATE_62_5		4
#define SPD_RFSH_RT_RATE_31_3		3
#define SPD_RFSH_RT_RATE_15_625		0
#define SPD_RFSH_RT_RATE_7_8		2
#define SPD_RFSH_RT_RATE_3_9		1

#define SPD_SDRAM_WIDTH_W_SHF		0
#define SPD_SDRAM_WIDTH_W_MSK		(MSK(7) << SPD_SDRAM_WIDTH_W_SHF)
#define SPD_SDRAM_WIDTH_B2_SHF		7
#define SPD_SDRAM_WIDTH_B2_MSK		(MSK(1) << SPD_SDRAM_WIDTH_B2_SHF)
#define SPD_SDRAM_WIDTH_B2_BIT		SPD_SDRAM_WIDTH_B2_MSK

#define SPD_EC_SDRAM_WIDTH_SHF		0
#define SPD_EC_SDRAM_WIDTH_MSK		(MSK(7) << SPD_EC_SDRAM_WIDTH_SHF)


/************************************************************************
 *      PCI definitions
 *	vendor/device pairs for PCI devices in Malta
 ************************************************************************/

/* Galileo Technologies GT64120 System Controller */
#define GT64120_PCI_VEND		0x11AB
#define GT64120_PCI_DEV			0x4620

/* Intel 82371 iPIIX4 (South Bridge) */
#define IPIIX4_PCI_VEND			0x8086
#define IPIIX4_PCI_DEV0			0x7110 /* PCI/ISA bridge */
#define IPIIX4_PCI_DEV1			0x7111 /* IDE Controller */
#define IPIIX4_PCI_DEV2			0x7112 /* USB Controller */
#define IPIIX4_PCI_DEV3			0x7113 /* Power Management/SMBus */

/* AMD 79C973 Ethernet controller */
#define LN973_PCI_VEND			0x1022
#define LN973_PCI_DEV			0x2000

/* Crystal CS4281 Audio Controller */
#define CRYSTAL_PCI_VEND		0x1013
#define CRYSTAL_PCI_DEV			0x6005

/*  IO addresses (Physical addresses) for UINT8 access. */
#define MALTA_PCI0_IO_BASE		PHYS_TO_K1(MALTA_PCIIO0_BASE)


/************************************************************************
 *      PCI configuration
 ************************************************************************/

/* PCI Configuration Address & Data Reg Offsets */

#define PCI_CONFIG_ADDR_REG		PHYS_TO_K1((MALTA_CORECTRL_BASE+0xcf8))
#define PCI_CONFIG_DATA_REG		PHYS_TO_K1((MALTA_CORECTRL_BASE+0xcfc))

/* PCI device numbers */
#ifndef PCI_IDSEL2DEVNUM
#define PCI_IDSEL2DEVNUM(idsel)	((idsel) - 10)
#endif

/* ADP bit used as IDSEL during configuration cycles */

#define MALTA_IDSEL_SLOT1		28
#define MALTA_IDSEL_SLOT2		29
#define MALTA_IDSEL_SLOT3		30
#define MALTA_IDSEL_SLOT4		31
#define MALTA_IDSEL_PIIX4		20
#define MALTA_IDSEL_AM79C973		21
#define MALTA_IDSEL_CRYSTAL		22

#define MALTA_DEVNUM_PIIX4		PCI_IDSEL2DEVNUM(MALTA_IDSEL_PIIX4)
#define MALTA_DEVNUM_AM79C973		PCI_IDSEL2DEVNUM(MALTA_IDSEL_AM79C973)
#define MALTA_DEVNUM_CRYSTAL		PCI_IDSEL2DEVNUM(MALTA_IDSEL_CRYSTAL)
#define MALTA_DEVNUM_PCI_SLOT1		PCI_IDSEL2DEVNUM(MALTA_IDSEL_SLOT1)
#define MALTA_DEVNUM_PCI_SLOT2		PCI_IDSEL2DEVNUM(MALTA_IDSEL_SLOT2)
#define MALTA_DEVNUM_PCI_SLOT3		PCI_IDSEL2DEVNUM(MALTA_IDSEL_SLOT3)
#define MALTA_DEVNUM_PCI_SLOT4		PCI_IDSEL2DEVNUM(MALTA_IDSEL_SLOT4)

/* PIIX4 is a 4-function PCI device */
#define PIIX4_PCI_FUNCTION_BRIDGE	0
#define PIIX4_PCI_FUNCTION_IDE		1
#define PIIX4_PCI_FUNCTION_USB		2
#define PIIX4_PCI_FUNCTION_POWER	3

/* PIIX4-specific register defines */
#define PCI_BUS_LOCAL			0

#define PCI_0_CAUSE_REGISTER		GT_INTR_CAUSE_OFS
#define MASTER_ABORT_BIT		0x40000

/* PCI Configuration Reg Offsets and bit encodings */

#define PCI_SC				(PCI_CFG_COMMAND >> 2)
#define PCI_BHLC			(PCI_CFG_CACHE_LINE_SIZE >> 2)

/* Power Management Base Address register (Function 3) */
#define PCI_CFG_PIIX4_PM_BASE		(PM_BASE >> 2)

/* General configuration register */
#define PIIX4_PCI_GENCFG		0xb0
#define PCI_CFG_PIIX4_GENCFG		(PIIX4_PCI_GENCFG >> 2)

/* PIIX4 floppy configuration regsiter, added by yinwx */
#define IPIIX4_PCI_FDCMONEN		0x10		/* bit #12 - DEVRESD */
#define IPIIX4_PCI_EIOENDEV5	0x20000000	/* bit #29 - DEVRESB */
#define IPIIX4_PCI_FDCDECSEL	0xefffffff	/* bit #28 - DEVRESB */

/* Serial IRQ configuration register */
#define PIIX4_PCI_SERIRQC		0x64
#define PCI_CFG_PIIX4_SERIRQC		(PIIX4_PCI_SERIRQC >> 2)

/* RTC configuration register */
#define PIIX4_PCI_RTC			0xCB
#define PCI_CFG_PIIX4_RTC		(PIIX4_PCI_RTC >> 2)

#define PIIX4_RTC_URAM_ENABLE_SHF	2
#define PIIX4_RTC_URAM_ENABLE_MSK	(MSK(1) << PIIX4_RTC_URAM_ENABLE_SHF)
#define PIIX4_RTC_URAM_ENABLE_BIT	PIIX4_RTC_URAM_ENABLE_MSK

#define PCI_BAR_MAXCOUNT		6

#define PCI_SC_CMD_MS_SHF		1
#define PCI_SC_CMD_MS_MSK		(MSK(1) << PCI_SC_CMD_MS_SHF)
#define PCI_SC_CMD_MS_BIT		PCI_SC_CMD_MS_MSK

#define PCI_SC_CMD_BM_SHF		2
#define PCI_SC_CMD_BM_MSK		(MSK(1) << PCI_SC_CMD_BM_SHF)
#define PCI_SC_CMD_BM_BIT		PCI_SC_CMD_BM_MSK

#define PCI_SC_CMD_SERR_SHF		8
#define PCI_SC_CMD_SERR_MSK		(MSK(1) << PCI_SC_CMD_SERR_SHF)
#define PCI_SC_CMD_SERR_BIT		PCI_SC_CMD_SERR_MSK

/* BIST, Header Type, Lat timer, Cache line size (BHLC) */
#define PCI_BHLC_LT_SHF			16

/* Programmable IRQ config register */
#define PIIX4_PCI_PIRQRC		0x60
#define PCI_CFG_PIIX4_PIRQRC		(PIIX4_PCI_PIRQRC >> 2)

/* PIIX4 IRQ definitions */
#define PIIX4_PIRQRC_PCIA_IR_IRQA_SHF	0
#define PIIX4_PIRQRC_PCIA_IR_IRQA_MSK	(MSK(4) << PIIX4_PIRQRC_IR_IRQA_SHF)
#define PIIX4_PIRQRC_PCIA_IR_IRQ10	10

#define PIIX4_PIRQRC_PCIB_IR_IRQB_SHF	(0 + 8)
#define PIIX4_PIRQRC_PCIB_IR_IRQB_MSK	(MSK(4) << PIIX4_PIRQRC_IR_IRQB_SHF)
#define PIIX4_PIRQRC_PCIB_IR_IRQ10	10

#define PIIX4_PIRQRC_PCIC_IR_IRQC_SHF	(0 + 16)
#define PIIX4_PIRQRC_PCIC_IR_IRQC_MSK	(MSK(4) << PIIX4_PIRQRC_IR_IRQC_SHF)
#define PIIX4_PIRQRC_PCIC_IR_IRQ11	11

#define PIIX4_PIRQRC_PCID_IR_IRQD_SHF	(0 + 24)
#define PIIX4_PIRQRC_PCID_IR_IRQD_MSK	(MSK(4) << PIIX4_PIRQRC_IR_IRQD_SHF)
#define PIIX4_PIRQRC_PCID_IR_IRQ11	11

/* level-sensitive interrupt bits for IRQ3, 4, 10 and 11 */
#define PIIX4_ELCR1_IRQ3LEVEL_SHF	3
#define PIIX4_ELCR1_IRQ3LEVEL_MSK	(MSK(1) << PIIX4_ELCR1_IRQ3LEVEL_SHF)
#define PIIX4_ELCR1_IRQ3LEVEL_BIT	PIIX4_ELCR1_IRQ3LEVEL_MSK

#define PIIX4_ELCR1_IRQ4LEVEL_SHF	4
#define PIIX4_ELCR1_IRQ4LEVEL_MSK	(MSK(1) << PIIX4_ELCR1_IRQ4LEVEL_SHF)
#define PIIX4_ELCR1_IRQ4LEVEL_BIT	PIIX4_ELCR1_IRQ4LEVEL_MSK

#define PIIX4_ELCR2_IRQ10LEVEL_SHF	2
#define PIIX4_ELCR2_IRQ10LEVEL_MSK	(MSK(1) << PIIX4_ELCR2_IRQ10LEVEL_SHF)
#define PIIX4_ELCR2_IRQ10LEVEL_BIT	PIIX4_ELCR2_IRQ10LEVEL_MSK

#define PIIX4_ELCR2_IRQ11LEVEL_SHF	3
#define PIIX4_ELCR2_IRQ11LEVEL_MSK	(MSK(1) << PIIX4_ELCR2_IRQ11LEVEL_SHF)
#define PIIX4_ELCR2_IRQ11LEVEL_BIT	PIIX4_ELCR2_IRQ11LEVEL_MSK

/* bit to enable serial IRQ */
#define PIIX4_GENCFG_SERIRQ_SHF		16
#define PIIX4_GENCFG_SERIRQ_MSK		(MSK(1) << PIIX4_GENCFG_SERIRQ_SHF)
#define PIIX4_GENCFG_SERIRQ_BIT		PIIX4_GENCFG_SERIRQ_MSK

/* SERIRQ control bits */
#define PIIX4_SERIRQC_ENABLE_SHF	7
#define PIIX4_SERIRQC_ENABLE_MSK	(MSK(1) << PIIX4_SERIRQC_ENABLE_SHF)
#define PIIX4_SERIRQC_ENABLE_BIT	PIIX4_SERIRQC_ENABLE_MSK
#define PIIX4_SERIRQC_CONT_SHF		6
#define PIIX4_SERIRQC_CONT_MSK		(MSK(1) << PIIX4_SERIRQC_CONT_SHF)
#define PIIX4_SERIRQC_CONT_BIT		PIIX4_SERIRQC_CONT_MSK
#define PIIX4_SERIRQC_FS_SHF		2
#define PIIX4_SERIRQC_FS_MSK		(MSK(4) << PIIX4_SERIRQC_FS_SHF)
#define PIIX4_SERIRQC_FPW_SHF		0
#define PIIX4_SERIRQC_FPW_MSK		(MSK(2) << PIIX4_SERIRQC_FPW_SHF)

/************************************************************************
 Defines (port addresses, etc.) used for peripherals in Intel 82371 iPIIX4 
 ************************************************************************/
/************************************************************************
 *     Devices residing in ISA I/O space (PIIX4 function 0)
 ************************************************************************/

/* Programmable Interrupt Controller */
#define MALTA_PICA_BASE			0x20
#define MALTA_PICB_BASE			0xa0
/*
#define PIC1_BASE_ADR			MALTA_PICA_BASE
#define PIC2_BASE_ADR			MALTA_PICB_BASE
*/
#define PIC_REG_ADDR_INTERVAL		1
#define NUMBER_OF_IRQS			16

/* Programmable Interval Timer */
#define MALTA_PIT_BASE			(MALTA_PCI0_IO_BASE + 0x40)
#define TIMER_BASE_ADRS			MALTA_PIT_BASE
#define TIMER2_HZ			(14318180/12)
#define INT_LVL_TIMER0			0

#define TIMER_COUNTER0			0x40
#define TIMER_COUNTER1			0x41
#define TIMER_COUNTER2			0x42
#define TIMER_CONTROL			0x43

/*  Real Time Clock
 *  NOTE: to use XRTCADR/XRTCDAT, bit 2 in the RTCCFG register must be set.
 */
#define MALTA_RTCADR			0x70
#define MALTA_RTCDAT			0x71
#define MALTA_XRTCADR			0x72
#define MALTA_XRTCDAT			0x73


/************************************************************************
 Defines used for peripherals in SMCFDC37M81X Super I/O chip (on ISA bus)
 ************************************************************************/

/*  UARTs (Physical addresses). */
#define MALTA_SMSC_COM1_ADR		(MALTA_PCI0_IO_BASE + 0x3f8)
#define MALTA_SMSC_COM2_ADR		(MALTA_PCI0_IO_BASE + 0x2f8)
#define MALTA_SMSC_COM_DELTA		1 /* bytes between registers */
#define MALTA_UART0ADR			MALTA_SMSC_COM1_ADR
#define MALTA_UART1ADR			MALTA_SMSC_COM2_ADR

/* UART Clock rate. This frequency is valid for baud rates up to 115.2Kb */
#define BAUD_CLK_FREQ			1843200		/* 1.8432Mhz */

/*  Parallel port (Physical addresses). */
#define MALTA_SMSC_1284_ADR		0x378
#define MALTA_1284ADR			MALTA_SMSC_1284_ADR

/*  Floppy disk (Physical addresses). */
#define MALTA_SMSC_FDD_ADR		0x3f0
#define MALTA_FDDADR			MALTA_SMSC_FDD_ADR

/*  Keyboard, Mouse (Physical addresses). */
#define MALTA_SMSC_KYBD_ADR		0x60  /* Fixed 0x60, 0x64 */
#define MALTA_KYBDADR			MALTA_SMSC_KYBD_ADR
#define MALTA_SMSC_MOUSE_ADR		MALTA_SMSC_KYBD_ADR
#define MALTA_MOUSEADR			MALTA_KYBDADR

/* Needed in smcFdc37m81xInit */
#define SMSC_CONFIG_OFS			/*0x15c*/0x3f0
#define SMSC_DATA_OFS			/*0x15d*/0x3f1

#define SMSC_CONFIG_DEVNUM		0x7
#define SMSC_CONFIG_ACTIVATE	0x30
#define SMSC_CONFIG_BASEHI		0x60
#define SMSC_CONFIG_BASELO		0x61
#define SMSC_CONFIG_IRQ			0x70
#define SMSC_CONFIG_IRQ2		0x72
#define SMSC_CONFIG_DRQ			0x74	/* added by yinwx */
#define SMSC_CONFIG_MODE		0xF0

/* Needed in w83527, added by yinwx, 2009-03-06 */
#define W83527EFIR		0x2E
#define W83527EFDR		0x2F
#define W83527_CONFIG_MODE_START		0x87
#define W83527_CONFIG_MODE_EXIT			0xAA
#define W83527_CHIPCTRL_DEVSEL			0x7
#define W83527_CONFIG_DEVNUM_KYBD		5
#define W83527_CONFIG_ACTIVE			0x30
#define W83527_CONFIG_IRQ				0x70
#define W83527_CONFIG_IRQ2				0x72

#define W83527_CONFIG_ACTIVE_ENABLE		0x1

/************************************************************************
 *  Register encodings
*************************************************************************/

#define SMSC_CONFIG_KEY_START		/*0x55*/0x87  /*modified by wangfq*/
#define SMSC_CONFIG_KEY_EXIT		0xaa

#define SMSC_CONFIG_DEVNUM_FDD		0 /*0*/
#define SMSC_CONFIG_DEVNUM_COM1		2 /*4*/
#define SMSC_CONFIG_DEVNUM_COM2		3 /*5*/
#define SMSC_CONFIG_DEVNUM_PARALLEL	1 /*3*/
#define SMSC_CONFIG_DEVNUM_KYBD		5 /*7*/

#define SMSC_CONFIG_ACTIVATE_ENABLE_SHF	0
#define SMSC_CONFIG_ACTIVATE_ENABLE_MSK (MSK(1) << SMSC_CONFIG_ACTIVATE_ENABLE_SHF)
#define SMSC_CONFIG_ACTIVATE_ENABLE_BIT SMSC_CONFIG_ACTIVATE_ENABLE_MSK

#define SMSC_CONFIG_MODE_HIGHSPEED_SHF	1
#define SMSC_CONFIG_MODE_HIGHSPEED_MSK	(MSK(1) << SMSC_CONFIG_MODE_HIGHSPEED_SHF)
#define SMSC_CONFIG_MODE_HIGHSPEED_BIT	SMSC_CONFIG_MODE_HIGHSPEED_MSK

/* floppy disk (FD) */

#define FD_INT_VEC			IV_FLOPPY_VEC /* floppy interrupt vector, added by yinwx */
#define FD_INT_LVL          0x06
#define FD_DMA_BUF_ADDR		0xa03f0000/* floppy disk DMA buffer address, original 0x2000*/
#define FD_DMA_BUF_SIZE		0x3000	/* floppy disk DMA buffer size */

/*added by wangfq*/
/* keyboard  (KBD)*/
#define KBD_INT_LVL         0x01
/*end added*/

/* hard disk (IDE) */

/* #define IDE_CONFIG		0x0	*/ /* 1: uses ideTypes table */
#define IDE_INT_LVL             0x0e

/* hard disk (ATA) */

#define ATA0_IO_START0		0x1f0	/* io for ATA0 */
#define ATA0_IO_STOP0		0x1f7
#define ATA0_IO_START1		0x3f6
#define ATA0_IO_STOP1		0x3f7
#define ATA0_INT_LVL            0x0e
#define ATA0_INT_VEC		IV_PRI_IDE_VEC
#define ATA0_CONFIG		(ATA_GEO_CURRENT | ATA_PIO_AUTO | \
				 ATA_BITS_16 | ATA_PIO_MULTI)
#define ATA1_IO_START0		0x170   /* io for ATA1 */
#define ATA1_IO_STOP0		0x177
#define ATA1_IO_START1		0x376
#define ATA1_IO_STOP1		0x377
#define ATA1_INT_LVL		0x0f
#define ATA1_INT_VEC		IV_SEC_IDE_VEC
#define ATA1_CONFIG		(ATA_GEO_CURRENT | ATA_PIO_AUTO | \
				 ATA_BITS_16 | ATA_PIO_MULTI)
#define ATA_SEM_TIMEOUT		5       /* timeout for ATA sync sem */
#define ATA_WDG_TIMEOUT		5       /* timeout for ATA watch dog */

/************************************************************************
 *  Interrupt information
*************************************************************************/

/* Native MIPS Interrupt assignments */
#define INT_VEC_BASE		0x60		 /* Vector Base */

#define IV_PIIX4_INTR_VEC	(INT_VEC_BASE + 0x00) /* South Bridge INTR */
#define IV_PIIX4_SMI_VEC	(INT_VEC_BASE + 0x01) /* South Bridge SMI */
#define IV_CBUS_UART_VEC	(INT_VEC_BASE + 0x02) /* CBUS UART */
#define IV_COREHI_VEC		(INT_VEC_BASE + 0x03) /* Core Card */
#define IV_CORELO_VEC		(INT_VEC_BASE + 0x04) /* Core Card */
#define IV_R4KTIMER_VEC		(INT_VEC_BASE + 0x05) /* MIPS timer interrupt */

/* these vectors are to handle the 8259-based interrupts */
/*#define INT_NUM_IRQ0		(INT_VEC_BASE + 0x06)*/ /* base of 8259 interrupts */
#if 1
/* #define IV_TIMER_VEC		(INT_NUM_IRQ0 + 0) *//* Timer */
/* #define IV_KBD_VEC		(INT_NUM_IRQ0 + 1)  *//* Keyboard */
#define IV_DUMMY2_VEC		(INT_NUM_IRQ0 + 2) /* Not used */
#define IV_UART1_VEC		(INT_NUM_IRQ0 + 3) /* UART 1*/
#define IV_UART0_VEC		(INT_NUM_IRQ0 + 4) /* UART 0 */
#define IV_DUMMMY5_VEC		(INT_NUM_IRQ0 + 5) /* Not used */
#define IV_FLOPPY_VEC		(INT_NUM_IRQ0 + 6) /* Floppy */
#define IV_PARPORT_VEC		(INT_NUM_IRQ0 + 7) /* Parallel port*/
#define IV_RTC_VEC		(INT_NUM_IRQ0 + 8) /* Real-time clock */
/* #define IV_I2C_VEC		(INT_NUM_IRQ0 + 9) */ /* IIC (SMBus) controller*/
#define IV_PCIAB_VEC		(INT_NUM_IRQ0 + 10) /* PCI A and B */
#define IV_PCICD_VEC		(INT_NUM_IRQ0 + 11) /* PCI C and D */
#define IV_MOUSE_VEC		(INT_NUM_IRQ0 + 12) /* Mouse */
#define IV_DUMMY13_VEC		(INT_NUM_IRQ0 + 13) /* Not used */
#define IV_PRI_IDE_VEC		(INT_NUM_IRQ0 + 14) /* Primary IDE */
#define IV_SEC_IDE_VEC		(INT_NUM_IRQ0 + 15) /* Secondary IDE */
#endif
/**** IRQ lines for Malta devices ****/

/* PCI INTA..D       */
#define MALTA_INTLINE_PCIA		10
#define MALTA_INTLINE_PCIB		10
#define MALTA_INTLINE_PCIC		11
#define MALTA_INTLINE_PCID		11

/* PCI slot 1        */
#define MALTA_INTLINE_SLOT1_A		MALTA_INTLINE_PCIA
#define MALTA_INTLINE_SLOT1_B		MALTA_INTLINE_PCIB
#define MALTA_INTLINE_SLOT1_C		MALTA_INTLINE_PCIC
#define MALTA_INTLINE_SLOT1_D		MALTA_INTLINE_PCID

/* PCI slot 2        */
#define MALTA_INTLINE_SLOT2_A		MALTA_INTLINE_PCIB
#define MALTA_INTLINE_SLOT2_B		MALTA_INTLINE_PCIC
#define MALTA_INTLINE_SLOT2_C		MALTA_INTLINE_PCID
#define MALTA_INTLINE_SLOT2_D		MALTA_INTLINE_PCIA

/* PCI slot 3        */
#define MALTA_INTLINE_SLOT3_A		MALTA_INTLINE_PCIC
#define MALTA_INTLINE_SLOT3_B		MALTA_INTLINE_PCID
#define MALTA_INTLINE_SLOT3_C		MALTA_INTLINE_PCIA
#define MALTA_INTLINE_SLOT3_D		MALTA_INTLINE_PCIB

/* PCI slot 4        */
#define MALTA_INTLINE_SLOT4_A		MALTA_INTLINE_PCID
#define MALTA_INTLINE_SLOT4_B		MALTA_INTLINE_PCIA
#define MALTA_INTLINE_SLOT4_C		MALTA_INTLINE_PCIB
#define MALTA_INTLINE_SLOT4_D		MALTA_INTLINE_PCIC

/* PIIX4/SuperIO devices (via PIIX4-resident 8259 PICs) */
#define MALTA_INTLINE_KYBD		1
#define MALTA_INTLINE_TTY1		3
#define MALTA_INTLINE_TTY0		4
#define MALTA_INTLINE_FDD		6
#define MALTA_INTLINE_1284		7
#define MALTA_INTLINE_MOUSE		12

/* Local PCI devices */
#define MALTA_INTLINE_79C973		MALTA_INTLINE_PCIB
#define MALTA_INTLINE_4281		MALTA_INTLINE_PCIC
#define MALTA_INTLINE_PIIX4_USB		MALTA_INTLINE_PCID
#define MALTA_INTLINE_PIIX4_SMB		9

/**** CPU interrupt lines used by devices ****/
#define MALTA_CPUINT_PIIX4		C0_STATUS_IM_HW0
#define MALTA_CPUINT_64120		C0_STATUS_IM_HW3

/**** CPU interrupt line used for SMI ****/
#define MALTA_CPUINT_SMI		C0_STATUS_IM_HW1


/* needed in stubUsbMipsPciLib.c */
#define INT_ENABLE(i)		sysIntEnablePIC(i)
#define INT_DISABLE(i)		sysIntDisablePIC(i)
/* end of defines needed in stubUsbMipsPciLib.c */

 /*
 * Support for determining if we're ROM based or not.  _sysInit
 * saves the startType parameter at location ROM_BASED_FLAG.
 */

#define PCI_AUTOCONFIG_FLAG_OFFSET ( 0x0A00 )
#define PCI_AUTOCONFIG_FLAG        ( *(UCHAR *)(LOCAL_MEM_LOCAL_ADRS + \
				     PCI_AUTOCONFIG_FLAG_OFFSET) )
#define PCI_AUTOCONFIG_FLAG_ADDR (LOCAL_MEM_LOCAL_ADRS + \
				     PCI_AUTOCONFIG_FLAG_OFFSET)
#define PCI_AUTOCONFIG_DONE ( PCI_AUTOCONFIG_FLAG != 0 )

#ifdef __cpluscplus
}
#endif	/* __cplusplus */
#endif	/* __INCmaltah */
