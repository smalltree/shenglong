/* config.h - Loongson2ebox configuration header */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

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
01b,21nov02,zmm  Fix comments, some defines, and other cleanups.
01a,26sep02,zmm  Written.
*/

/*
This file contains the configuration parameters for the CP7000G BSP.
*/

#ifndef	__INCconfigh
#define	__INCconfigh

#ifdef __cplusplus
extern "C" {
#endif

/* BSP version/revision identification, before configAll.h */

#define BSP_VER_1_1     1
#define BSP_VER_1_2     1
#define BSP_VERSION     "1.2"   
#define BSP_REV         "/0"   /* increment with each revision */ 

/* includes */
#include "configAll.h"

#include "Lsn2eCpciSBC.h"

#define CACHE_LINE_SIZE		32	/* All caches have 32 byte linesize */

#ifndef LLONG
#define LLONG long long 
#endif

#ifndef ULLONG
#define ULLONG unsigned long long 
#endif

/* configure networking */
#undef INCLUDE_WINDML
#define INCLUDE_END
#undef INCLUDE_FEI_END
#define INCLUDE_GEI8254X_END      /* undef by yinwx for ETX */
#define INCLUDE_WINDVIEW	/* WindView target facilities */
/*configure WDB */
#ifndef INCLUDE_WDB
#define INCLUDE_WDB
#endif
#define INCLUDE_LOG_STARTUP
/*#define INCLUDE_STANDALONE_SYM_TBL*/ /*!note:if defined,compile time, undefined reference to `standTblSize'and`standTbl'*/
									   /* compiled-in symbol table */	
#define SNTP_PORT 	123
#define INCLUDE_PING
#define INCLUDE_TELNET
#define INCLUDE_SNTPC 
#define INCLUDE_FTP_SERVER	/* ftp server */
#define INCLUDE_NET_SHOW
/*#define DEFAULT_STREAMS_SOCKET*/ /*note:if defined, networking abnormal*/
#define	INCLUDE_TCP_DEBUG
#define INCLUDE_MIB2_ALL /* All of MIB 2 */

#define INCLUDE_LOADER          /* object module loading */
#define INCLUDE_NET_SYM_TBL

#define INCLUDE_UART
#define INCLUDE_GPIO
#define INCLUDE_WATCHDOG
/* configure ATA */
 #define INCLUDE_SII0680 
#ifndef  INCLUDE_FLOATING_POINT
#define INCLUDE_FLOATING_POINT
#endif
#if 0
#ifndef  INCLUDE_SPY 
#define INCLUDE_SOFTAUXTIMER
#define INCLUDE_SPY 
#endif
#endif
#define INCLUDE_POSIX_FTRUNC
/* configure c++  */
#define INCLUDE_CPLUS		/* include C++ support */
#define INCLUDE_CPLUS_IOSTREAMS	/* include basic iostreams classes */
#define INCLUDE_CPLUS_STL	/* include Standard Template Library core */
#define INCLUDE_CPLUS_STRING      /* include string class */
#define INCLUDE_CPLUS_STRING_IO   /* include i/o for string class */
#define INCLUDE_CPLUS_COMPLEX     /* include complex number class */
#define INCLUDE_CPLUS_COMPLEX_IO  /* include i/o for complex number class */
#define INCLUDE_CPLUS_IOSTREAMS_FULL  /* include all of iostreams */
/* CPU-specific facilities */
#undef USER_D_CACHE_MODE
#define USER_D_CACHE_MODE   CACHE_COPYBACK 


/* miscellaneous definitions */
#if 1
/* "gei(0,0)host:vxWorks h=10.0.0.100 e=10.0.0.123:ffff0000 u=sbc pw=pwd" */
#define DEFAULT_BOOT_LINE \
"gei(0,0)host:vxWorks h=10.0.0.100 e=10.0.0.105:ffff0000 u=sbc pw=pwd s=startup.txt"
/* "gei(0,0)host:vxWorks h=10.0.0.100 e=10.0.0.105:ffff0000 u=usr pw=pwd f=0x80 s=startup.txt" */
 /* "ata=0,0(0,0)host:/ata0/vxWorks.st h=10.0.0.100 e=10.0.0.105 u=target"  */

#endif
#if 0
#define DEFAULT_BOOT_LINE \
"ata=0,0(0,0)host:/ata0/boot/vxWorks h=10.0.0.100 e=10.0.0.105 u=usr pw=pwd tn=target s=startup.txt"
#endif

#define INCLUDE_STARTUP_SCRIPT

#undef  NUM_TTY
#define NUM_TTY			N_SIO_CHANNELS
#undef  INCLUDE_TYCODRV_5_2 


#define SYS_CLK_RATE_MIN  1	/* minimum system clock rate */
#define SYS_CLK_RATE_MAX  5000	/* maximum system clock rate */

#if 0  /*comented by wangfq*/
#define AUX_CLK_RATE_MIN  1	/* minimum auxiliary clock rate */
#define AUX_CLK_RATE_MAX  10000	/* maximum auxiliary clock rate */
#endif /*end commented*/

/* Define our own boot offset to be compatible with other monitors (PMON) */

#if 0 /*commented by wangfq, NVRAM not  needed, instead flash can be used for bootline store*/
#undef  NV_BOOT_OFFSET
#define NV_RAM_SIZE	BBRAM_SIZE
#define NV_BOOT_OFFSET  (NV_RAM_SIZE - 512)
#define NV_RAM_ADR	((volatile char *) BBRAM_ADRS)
#endif /*end commented, replaced by the following*/
/*added by wangfq*/
#define INCLUDE_SHELL           /* interactive c-expression interpreter */
#if 0
#define INCLUDE_SHELL_BANNER
#define INCLUDE_SHELL_INTERP_CMD
#define INCLUDE_SHELL_INTERP_C
#endif
#if 1
#define INCLUDE_IPATTACH

#endif

#define INCLUDE_SHOW_ROUTINES   /* show routines for system facilities*/
#define INCLUDE_SYM_TBL         /* symbol table package */

#if 0
#define INCLUDE_STANDALONE_SYM_TBL /* compiled-in symbol table */
#endif
#define INCLUDE_STAT_SYM_TBL
#define INCLUDE_DEMO
	#undef CONSOLE_TTY
	#define CONSOLE_TTY 0 /* Modified 2009-04-15 */

	#undef ISR_STACK_SIZE
	#define ISR_STACK_SIZE          (50000)  /* size of ISR stack, in bytes */
#undef NV_BOOT_OFFSET
/*#undef INCLUDE_CACHE_SUPPORT*/
#undef INCLUDE_EXC_SHOW
/*#undef INCLUDE_NET_INIT*/
/*end added */

#define VGA_BE_RAMBASE 0xd0000000  /** Here added for x86 VGA Emu !! 2008-10-29 **/
/*#define	INCLUDE_PC_CONSOLE*/
#ifdef  INCLUDE_PC_CONSOLE
#define N_VIRTUAL_CONSOLES 1
#define STATUS_8042 0x64
#define DATA_8042 0x60
#define COMMAND_8042 0x64
#define ENGLISH_KBD 1

#define BEEP_PITCH_L 10
#define BEEP_TIME_L 10
#define BEEP_PITCH_S 2
#define BEEP_TIME_S 2

#define CTRL_SEL_REG 0x3D4 
#define CTRL_VAL_REG 0x3D5
#define CTRL_MEM_BASE 0xb00b8000
#define COLOR_MODE TRUE
#define DEFAULT_ATR 0x0007
#define CHR 2

#ifndef	PC_CONSOLE
#define	PC_CONSOLE		0
#endif
#endif

/* memory constants */

#define LOCAL_MEM_LOCAL_ADRS	0x80000000	
#define LOCAL_MEM_SIZE			0x10000000/*0x10000000*/		
#define DRAM_SIZE		sdramSize	

/* measure memory dynamically - defined by default */

#undef LOCAL_MEM_AUTOSIZE /*modifed by wangfq, original #define*/

/* User reserved memory, See sysMemTop */

#define USER_RESERVED_MEM       0   /*modified by wangfq, original 0x01800000*/ 

/*
 * The constants ROM_TEXT_ADRS, ROM_SIZE, RAM_LOW_ADRS and
 * RAM_HIGH_ADRS are defined in config.h, and MakeSkel.
 * All definitions for these constants must be identical.
 */

#define ROM_TEXT_ADRS		0xbfc00000      /* base address of ROM */ /*modified by wangfq,original GAL_BOOTCS_BASE*/
#define ROM_BASE_ADRS		ROM_TEXT_ADRS
#define ROM_SIZE			BONITO_BOOT_SIZE      /* 512KB ROM space */ /*modified by wangfq, original 0x00080000 directly*/

#define RAM_LOW_ADRS		0x80200000      /* RAM address for kernel */ /*modified by wangfq,originally 0x80010000*/
#define RAM_HIGH_ADRS		0x80400000      /* RAM address for ROM boot */

#if 0
#define INCLUDE_USB                  /* Main USB Component */
#define  INCLUDE_USB_INIT             /* USB Initialization */
#undef  INCLUDE_UHCI                 /* UHCI Controller Driver */
#undef  INCLUDE_UHCI_INIT            /* UHCI Initialization */
#define INCLUDE_OHCI                 /* OHCI Controller Driver */
#define  INCLUDE_OHCI_INIT            /* OHCI Initialization */
#define  INCLUDE_OHCI_PCI_INIT        /* OHCI PCI Initialization */
#undef  INCLUDE_USBTOOL              /* usbTool Application */
#undef  INCDLUE_USB_AUDIO_DEMO       /* USB Audio Demo */
#undef  INCLUDE_USB_MOUSE            /* USB Mouse Driver */
#undef  INCLUDE_USB_MOUSE_INIT       /* Mouse Driver Initialization */
#define  INCLUDE_USB_KEYBOARD         /* USB Keyboard Driver */
#define  INCLUDE_USB_KEYBOARD_INIT    /* Keyboard Driver Initialization */
#undef INCLUDE_USB_PRINTER          /* USB Printer Driver */
#undef  INCLUDE_USB_PRINTER_INIT     /* Printer Driver Initialization */
#undef INCLUDE_USB_SPEAKER          /* USB Speaker Driver */
#undef  INCLUDE_USB_SPEAKER_INIT     /* Speaker Driver Initialization */
#define  INCLUDE_USB_MS_BULKONLY      /* USB Bulk Driver */
#define  INCLUDE_USB_MS_BULKONLY_INIT /* Bulk Driver Initialization */
#undef  INCLUDE_USB_PEGASUS_END      /* USB Pegasus Network Driver */
#undef  INCLUDE_USB_PEGASUS_END_INIT /* Pegaus Driver Initialization */

#undef INCLUDE_USB_MS_CBI
#undef INCLUDE_USB_MS_CBI_INIT

#define INCLUDE_DOSFS
#define INCLUDE_DOSFS_MAIN
#define INCLUDE_DOSFS_CHKDSK
#define INCLUDE_DOSFS_FMT
#define INCLUDE_FS_MONITOR
#define INCLUDE_ERF
#define INCLUDE_XBD
#define INCLUDE_DEVICE_MANAGER
#define INCLUDE_XBD_PART_LIB
#define INCLUDE_XBD_BLK_DEV

/*  USB Parameters */
 
#define BULK_DRIVE_NAME "/bd"                /* Bulk Drive Name */
#define CBI_DRIVE_NAME "/cbid"               /* CBI Drive Name */
#define PEGASUS_IP_ADDRESS "90.0.0.3"        /* Pegasus IP Address */
#define PEGASUS_DESTINATION_ADDRESS "90.0.0.53" /* Pegasus Destination Addres
s */
#define PEGASUS_NET_MASK 0xffffff00          /* Pegasus Net Mask */
#define PEGASUS_TARGET_NAME "host"           /* Pegasus Target Name */
#endif


#undef BOOT_LINE_OFFSET
#define BOOT_LINE_OFFSET        0xf000 /*originally 0x700 or 0x600,changed by wangfq*/


#undef BOOT_LINE_ADRS
#define BOOT_LINE_ADRS	((char *)(LOCAL_MEM_LOCAL_ADRS+BOOT_LINE_OFFSET))


/* Console baud rate reconfoguration. */

#undef  CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 115200	/* Reconfigure default baud rate */ /*modified by wangfq,originally 9600*/

#define INCLUDE_PCI	
#define PCI_MAX_DEV 21

#define MUX_MAX_BINDS 32 /*modifid by wangfq,original 16 in standard file*/

/*added by wangfq*/
#define INCLUDE_PCI_AUTOCONF
#define INCLUDE_PCI_CFGSHOW
#ifndef PCI_CFG_TYPE
#   ifdef INCLUDE_PCI_AUTOCONF
#      define PCI_CFG_TYPE PCI_CFG_AUTO
#   else
#      define PCI_CFG_TYPE PCI_CFG_FORCE
#   endif /* INCLUDE_PCI_AUTOCONF */
#endif /* PCI_CFG_TYPE */

#if 0
#undef INCLUDE_IPIIX4PCI
#undef INCLUDE_SMCFDC37M81X
#define INCLUDE_W83527			/* added by yinwx, for super I/O, 2009-03-06 */
#undef INCLUDE_FD				/* added by yinwx */
#define INCLUDE_SII0680			/* added by yinwx, for SiI0680 IDE Controller, 2009-03-19 */
#endif
#define INCLUDE_ATA 

#ifdef INCLUDE_ATA
#define INCLUDE_DISK_UTIL       /* ls, cd, mkdir, xcopy, etc. */
#define INCLUDE_DOSFS           /* usrDosFsOld.c wrapper layer */
#define INCLUDE_DOSFS_MAIN      /* dosFsLib (2) */
#define INCLUDE_DOSFS_FAT       /* dosFs FAT12/16/32 FAT table handler */
#define INCLUDE_DOSFS_DIR_VFAT  /* Microsoft VFAT dirent handler */
#define INCLUDE_DOSFS_DIR_FIXED /* 8.3 & VxLongNames directory handler */
#define INCLUDE_DOSFS_FMT       /* dosFs2 file system formatting module */
#define INCLUDE_DOSFS_CHKDSK    /* file system integrity checking */
#define INCLUDE_CBIO            /* CBIO API module */
#define INCLUDE_DISK_CACHE      /* CBIO API disk caching layer */
#define INCLUDE_DISK_PART       /* disk partition handling code, fdisk... */
#define INCLUDE_DISK_UTIL       /* ls, cd, mkdir, xcopy, etc */
#define INCLUDE_TAR             /* tar utility */
#define INCLUDE_RAM_DISK        /* CBIO API ram disk driver */
#endif
/*end added by wangfq*/

/*add by sunchao */
#define DRV_DEBUG
/* add end */

#ifdef __cplusplus
}
#endif
#endif	/* __INCconfigh */

#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif
