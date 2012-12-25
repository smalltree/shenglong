/* fei82557End.c - END style Intel 82557 Ethernet network interface driver */

/* Copyright 1989-2003 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,21mar03,rcs  removed #include "endNetBufLib.c" SPR# 87039
01k,21mar03,rcs  unmasked interrupt after netJobAdd() failed SPR# 87038
01j,28feb03,rcs  fixed polled mode. SPR#86433
01i,24feb03,rcs  corrected for using pre-allocated memory SPR# 86352
01h,13feb03,rcs  consolidated CFD writes in fei82557Encap.
01g,13feb03,rcs  consolidated descriptor cachDmaMalloc s in fei82557InitMem().
01f,07feb03,rcs  fixed diab compiler warnings.
01e,05feb03,rcs  fixed RFD_LONG_WR double swap SPR# 86037 and added
                 documentation for fei82557ShowRxRing and fei82557GetRUStatus
01d,04feb03,pmr  fixed doc build error.
01c,31jan03,m_h  IPv6 support
01b,31jan03,jkf  removed logMsg() of pDrvCtrl value.
01a,23jan03,rcs  created from target/src/drv/end/fei82557End.c, version 02r 
*/

/*
DESCRIPTION
This module implements an Intel 82557 and 82559 Ethernet network interface 
driver. (For the sake of brevity this document will only refer to the 82557.)
This is a fast Ethernet PCI bus controller, IEEE 802.3 10Base-T and 
100Base-T compatible. It also features a glueless 32-bit PCI bus master 
interface, fully compliant with PCI Spec version 2.1. An interface to 
MII compliant physical layer devices is built-in to the card. The 82557
Ethernet PCI bus controller also includes Flash support up to 1 MByte
and EEPROM support, altough these features are not dealt with in this 
driver.

The 82557 establishes a shared memory communication system with the CPU,
which is divided into three parts: the Control/Status Registers (CSR),
the Command Block List (CBL) and the Receive Frame Area (RFA). The CSR
is on chip and is either accessible with I/O or memory cycles, whereas the
other structures reside on the host.

The CSR is the main means of communication between the device and the
host, meaning that the host issues commands through these registers
while the chip posts status changes in it, occurred as a result of those
commands. Pointers to both the CBL and RFA are also stored in the CSR.

The CBL consists of a linked list of frame descriptors through which 
individual action commands can be performed. These may be transmit 
commands as well as non-transmit commands, e.g. Configure or Multicast
setup commands. While the CBL list may function in two different modes,
only the simplified memory mode is implemented in the driver.


The RFA consists of a pair of linked list rings. The Receive Frame Descriptor
(RFD) ring and the Receive Buffer Descriptor (RBD) ring. The RFDs hold the 
status of completed DMAs. The RBDs hold the pointers to the DMA buffers,
refered to as clusters. 

When the device is initialized or restarted it is passed a pointer to an RFD.
This RFD is considered to be the "first" RFD. This RFD holds a pointer to 
one of the RBDs. This RBD is then considered the "first" RBD. All other RFDs 
only have a NULL RBD pointer, actually 0xffffffff. Once the device is started
the rings are traversed by the device independently. 

Either descriptor type RFD or RBD can have a bit set in it to indicate that 
it is the End of the List (EL). This is initially set in the RBD descriptor 
immediately before the first RBD. This acts as a stop which prevents the DMA 
engine from wrapping around the ring and encountering a used descriptor. 
This is an unallowed condition and results in the device stopping operation
without an interupt or and indication of failure. When the EL RBD is 
encountered the the device goes into the receive stall state. The driver must 
then restart the device. To reduce, if not eliminate, the occurence of this 
costly, time consuming operation, the driver continually advances the EL to 
the last cleared RBD. Then when the driver services an incomming frame it
clears the RFD RBD pair and advances the EL. If the driver is not able to 
service an incomming frame, because of a shortage of resources such as 
clusters, the driver will throw that frame away and clear the RFD RBD pair 
and advance EL.    
     
Because the rings are independently traversed by the device it is imperative
that they be kept in sync. Unfortunately, there is no indication from one
or the other as to which descriptor it is pared with. It is left to the driver
to keep track of which discriptor goes with its counter part. If this 
syncronization is lost then the performance of the driver will be greatly 
impared or worse. To keep this syncronization this driver imbeds the 
RBD descriptors in tags. To do this it utilizes memory that would otherwise 
have been wasted. The DMA engine purportedly works most efficiently when the
descriptors are on a 32 byte boundry. The descriptors are only 16 bytes so 
there are 16 bytes to work with. The RBD_TAG s have as their first 16 bytes
the RBD itself, then it holds the RFD pointer to its counter part, a pointer 
to itself, a 16 bit index, a 16 bit next index, and 4 bytes of spare. This 
arrangement allows the driver to traverse only the RBD ring and discover the 
corresponding RFD through the RBD_TAG and guaranteeing sycronization.     

The driver is designed to be moderately generic, operating unmodified
across the range of architectures and targets supported by VxWorks.  
To achieve this, this driver must be given several target-specific 
parameters, and some external support routines must be provided.  These 
parameters, and the mechanisms used to communicate them to the driver, 
are detailed below.

BOARD LAYOUT
This device is on-board.  No jumpering diagram is necessary.

EXTERNAL INTERFACE

The driver provides the standard external interface, fei82557EndLoad(), which
takes a string of colon separated parameters. The parameters should be
specified in hexadecimal, optionally preceeded by "0x" or a minus sign "-".

The parameter string is parsed using strtok_r() and each parameter is
converted from a string representation to binary by a call to
strtoul(parameter, NULL, 16).

The format of the parameter string is:

"<memBase>:<memSize>:<nTfds>:<nRfds>:<flags>:<offset>:<maxRxFrames>:
<clToRfdRatio>:<nClusters>"

In addition, the two global variables 'feiEndIntConnect' and 
'feiEndIntDisconnect' specify respectively the interrupt connect routine 
and the interrupt disconnect routine to be used depending on the BSP. 
The former defaults to intConnect() and the user can override this to use 
any other interrupt connect routine (say pciIntConnect()) in sysHwInit() 
or any device specific initialization routine called in sysHwInit(). 
Likewise, the latter is set by default to NULL, but it may be overridden 
in the BSP in the same way.
 
TARGET-SPECIFIC PARAMETERS

.IP <memBase>
This parameter is passed to the driver via fei82557EndLoad().

The Intel 82557 device is a DMA-type device and typically shares
access to some region of memory with the CPU.  This driver is designed
for systems that directly share memory between the CPU and the 82557.

This parameter can be used to specify an explicit memory region for use
by the 82557.  This should be done on targets that restrict the 82557
to a particular memory region. Since use of this parameter indicates that 
the device has limited access to this specific memory region all buffers 
and descriptors directly accessed by the device (RFDs, RBDs, CFDs, and 
clusters) must be carved from this region. Since the transmit buffers must 
reside in this region the driver will revert to using simple mode buffering 
for transmit meaning that zero copy transmit is not supported. This then 
requires that there be enough space for clusters to be attached to the CFDs. 
The minimum memory requirement is for 32 bytes for all descriptors plus
at lease two 1536 byte clusters for each RFD and one 1536 byte cluster for
each CFD. Also, it should be noted that this memory must be non-cached. 

The constant `NONE' can be used to indicate that there are no memory 
limitations, in which case the driver will allocate cache aligned memory 
for its use using memalign().  

.IP <memSize>
The memory size parameter specifies the size of the pre-allocated memory
region. If memory base is specified as NONE (-1), the driver ignores this
parameter. Otherwise, the driver checks the size of the provided memory 
region is adequate with respect to the given number of descriptors and 
clusters specified. The amount of memory allocated must be enough to hold 
the RFDs, RBDs, CFDs and clusters. The minimum memory requirement is for 
32 bytes each for all descriptors, 32 bytes each for alignment of the 
descriptor types (RFDs, RBDs, and CFDs),  plus at least two 1536 byte 
clusters for each RFD and one 1536 byte cluster for each CFD. Otherwise the 
End Load routine will return ERROR. The number of clusters can be specified 
by either passing a value in the nCluster parameter, in which case the 
nCluster value must be at least nRFDs * 2, or by setting the cluster to RFD 
ratio (clToRfdRatio) to a number equal or greater than 2.   

.IP <nTfds>
This parameter specifies the number of transmit descriptor/buffers to be
allocated. If this parameter is less than two, a default of 64 is used.

.IP <nRfds>
This parameter specifies the number of receive descriptors to be
allocated. If this parameter is less than two, a default of 128 is used.

.IP <flags>
User flags may control the run-time characteristics of the Ethernet
chip. Not implemented.

.IP <offset>
Offset used to align IP header on word boundary for CPUs that need long word
aligned access to the IP packet (this will normally be zero or two). This 
parameter is optional, the default value is zero.

.IP <deviceId>
This parameter is used to indicate the specific type of device being used,
the 82557 or subsequent.  This is used to determine if features which were 
introduced after the 82557 can be used. The default is the 82557. If this is 
set to any value other than ZERO (0), NONE (-1), or FEI82557_DEVICE_ID (0x1229)
it is assumed that the device will support features not in the 82557. 
  
.IP <maxRxFrames>
This parameter limits the number of frames the receive handler will service 
in one pass. It is intended to prevent the tNetTask from hoging the CPU and
starving applications. This parameter is optional, the default value is 
nRFDs * 2.   

.IP <clToRfdRatio>
Cluster To RFD Ratio sets the number of clusters as a ratio of nRFDs. The 
minimum setting for this parameter is 2. This parameter is optional, the 
default value is 5.

.IP <nClusters>
Number of clusters to allocate. This value must be  at least nRFD * 2.
If this value is set then the <clToRfdRatio> is ignored. This parameter is 
optional, the default is nRFDs * clToRfdRatio. 
.LP
 
EXTERNAL SUPPORT REQUIREMENTS

This driver requires one external support function:
.CS
STATUS sys557Init (int unit, FEI_BOARD_INFO *pBoard)
.CE
This routine performs any target-specific initialization
required before the 82557 device is initialized by the driver.
The driver calls this routine every time it wants to [re]initialize
the device.  This routine returns OK, or ERROR if it fails.
.LP

SYSTEM RESOURCE USAGE
The driver uses cacheDmaMalloc() to allocate memory to share with the 82557.
The size of this area is affected by the configuration parameters specified
in the fei82557EndLoad() call. 

Either the shared memory region must be non-cacheable, or else
the hardware must implement bus snooping.  The driver cannot maintain
cache coherency for the device because fields within the command
structures are asynchronously modified by both the driver and the device,
and these fields may share the same cache line.


TUNING HINTS
The adjustable parameters are:

The number of TFDs and RFDs that will be created at run-time. These 
parameters are given to the driver when fei82557EndLoad() is called. There 
is one TFD and one RFD associated with each transmitted frame and each 
received frame respectively. For memory-limited applications, decreasing 
the number of TFDs and RFDs may be desirable. Increasing the number of TFDs 
will provide no performance benefit after a certain point. Increasing the 
number of RFDs will provide more buffering before packets are dropped.  This 
can be useful if there are tasks running at a higher priority than tNetTask. 

The maximum receive frames <maxRxFrames>. This parameter will allow the 
driver to service fixed amount of in comming traffic before forcing the 
receive handler to relenquish the CPU. This prevents the possible scenerio 
of the receive handler starving the application.

The parameters <clToRfdRatio> and <nClusters> control the number of 
clusters  created which is the major portion of the memory allocated by the 
driver. For memory-limited applications, decreasing the number clusters
may be desirable. However, this also will probably result in performance
degradation.    

ALIGNMENT
Some architectures do not support unaligned access to 32-bit data items. On
these architectures (eg ARM and MIPs), it will be necessary to adjust the
offset parameter in the load string to realign the packet. Failure to do so
will result in received packets being absorbed by the network stack, although
transmit functions should work OK. Also, some architectures do not support
SNOOPING, for these architectures the utilities FLUSH and INVALIDATE are
used for cache coherency of DMA buffers (clusters). These utilities depend 
on the buffers being cache line aligned and being cache line multiple. 
Therefore, if memory for these buffers is pre-allocated then it is imperitive
that this memory be cache line aligned and being cache line multiple. 

SEE ALSO: ifLib,
.I "Intel 82557 User's Manual,"
.I "Intel 32-bit Local Area Network (LAN) Component User's Manual"
*/

#include "vxWorks.h"
#include "wdLib.h"
#include "iv.h"
#include "vme.h"
#include "lstLib.h"
#include "semLib.h"
#include "sys/times.h"
#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "errno.h"
#include "memLib.h"
#include "intLib.h"
#include "net/route.h"
#include "iosLib.h"
#include "errnoLib.h"
#include "vxLib.h"    /* from if_fei.c */
#include "private/funcBindP.h"

#include "cacheLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "msgQLib.h"

#include "net/systm.h"
#include "net/if_subr.h"

#include "drv/end/fei82557End.h"
#include "drv/pci/pciIntLib.h"
#undef ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"
#include "end.h"

#define    END_MACROS
#include "endLib.h"

#ifdef WR_IPV6
#include "adv_net.h"
#endif /*WR_IPV6*/

/* defines */

/* Driver debug control */
#if 0
#define DRV_DEBUG557 /*wangfq*/
#else
#undef DRV_DEBUG557
#endif

/* Driver debug control */
#ifdef DRV_DEBUG557
#define DRV_DEBUG_OFF		0x0000
#define DRV_DEBUG_RX		0x0001
#define DRV_DEBUG_TX		0x0002
#define DRV_DEBUG_POLL		(DRV_DEBUG_POLL_RX | DRV_DEBUG_POLL_TX)
#define DRV_DEBUG_POLL_RX    	0x0004
#define DRV_DEBUG_POLL_TX    	0x0008
#define DRV_DEBUG_LOAD		0x0010
#define DRV_DEBUG_IOCTL		0x0020
#define DRV_DEBUG_INT		0x0040
#define DRV_DEBUG_START    	0x0080
#define DRV_DEBUG_DUMP    	0x0100
#define DRV_DEBUG_PHY		0x0200
#define DRV_DEBUG_ALL    	0xffff

int	fei82557Debug = DRV_DEBUG_ALL;

#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)			\
    do {								\
    if (fei82557Debug & FLG)						\
	if (_func_logMsg != NULL)					\
	    _func_logMsg (X0, (int)X1, (int)X2, (int)X3, (int)X4,	\
			    (int)X5, (int)X6);				\
    } while (0)

#else /* DRV_DEBUG557 */

#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)
#define DRV_PRINT(FLG, X)

#endif /* DRV_DEBUG557 */

/* general macros for reading/writing from/to specified locations */

#if (_BYTE_ORDER == _BIG_ENDIAN)
 
#define FEI_SWAP_LONG(x)        LONGSWAP(x)
#define FEI_SWAP_WORD(x)        (MSB(x) | LSB(x) << 8)
 
#else
 
#define FEI_SWAP_LONG(x)        (x)
#define FEI_SWAP_WORD(x)        (x)
 
#endif /* _BYTE_ORDER == _BIG_ENDIAN */

/* Cache and PCI-bus related macros */

#define FEI_VIRT_TO_SYS(virtAddr)					    \
	(FEI_LOCAL_TO_SYS (((UINT32) FEI_VIRT_TO_PHYS (virtAddr))))

#define FEI_VIRT_TO_PHYS(virtAddr)					    \
	CACHE_DRV_VIRT_TO_PHYS (&pDrvCtrl->cacheDmaFuncs, (char *)(virtAddr))

#define FEI_LOCAL_TO_SYS(physAddr)					    \
	LOCAL_TO_SYS_ADDR (pDrvCtrl->unit, (physAddr))

#define FEI_SYS_TO_VIRT(physAddr)                                           \
        (FEI_SYS_TO_LOCAL (((UINT32) FEI_PHYS_TO_VIRT (physAddr))))

#define FEI_PHYS_TO_VIRT(physAddr)                                          \
        CACHE_DRV_PHYS_TO_VIRT (&pDrvCtrl->cacheDmaFuncs, (char *)(physAddr))

#define FEI_SYS_TO_LOCAL(virtAddr)                                          \
        SYS_TO_LOCAL_ADDR (pDrvCtrl->unit, (virtAddr))

#define FEI_CACHE_INVALIDATE(address, len)				    \
        CACHE_DRV_INVALIDATE (&pDrvCtrl->cacheFuncs, (address), (len))

#define FEI_CACHE_FLUSH(address, len)				    \
        CACHE_DRV_FLUSH (&pDrvCtrl->cacheFuncs, (address), (len))


/* driver flags */
#define FEI_OWN_MEM	0x01		/* internally provided memory */
#define FEI_INV_NCFD    0x02		/* invalid nCFDs provided */
#define FEI_INV_NRFD    0x04		/* invalid nRFDs provided */
#define FEI_POLLING	0x08		/* polling mode */
#define FEI_PROMISC	0x20    	/* promiscuous mode */
#define FEI_MCAST	0x40    	/* multicast addressing mode */
#define FEI_MCASTALL	0x80    	/* all multicast addressing mode */
#define FEI_MEMOWN	0x10    	/* device mem allocated by driver */
 
#define FEI_FLAG_CLEAR(clearBits)					\
    (pDrvCtrl->flags &= ~(clearBits))

#define FEI_FLAG_SET(setBits)						\
    (pDrvCtrl->flags |= (setBits))

#define FEI_FLAG_GET()							\
    (pDrvCtrl->flags)

#define FEI_FLAG_ISSET(setBits)						\
    (pDrvCtrl->flags & (setBits))

/* shortcuts */
#define END_FLAGS_ISSET(setBits)					\
    ((&pDrvCtrl->endObj)->flags & (setBits))

#define FEI_VECTOR(pDrvCtrl)						\
    ((pDrvCtrl)->board.vector)

#define FEI_INT_ENABLE(pDrvCtrl)					\
    ((int)(pDrvCtrl)->board.intEnable)

#define FEI_INT_DISABLE(pDrvCtrl)					\
    ((int)(pDrvCtrl)->board.intDisable)

#define FEI_INT_ACK(pDrvCtrl)						\
    ((int)(pDrvCtrl)->board.intAck)

#define RFD_SLUSH	8
#define CL_OVERHEAD	4		/* prepended cluster header */
#define CL_RFD_SIZE	(RFD_DESC_SIZE + CL_OVERHEAD + RFD_SLUSH)		

#define FEI_SAFE_MEM(pDrvCtrl)						\
    ((pDrvCtrl)->memSize)

#define FEI_RFD_MEM(pDrvCtrl)						\
    (CL_RFD_SIZE * (pDrvCtrl)->nRFDs)


#define FEI_CFD_MEM(pDrvCtrl)						\
    (CFD_SIZE * (pDrvCtrl)->nCFDs)

#ifdef INCLUDE_RFC_2233

#define FEI_HADDR(pEnd)                                                 \
                ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress)

#define FEI_HADDR_LEN(pEnd)                                             \
                ((pEnd)->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength)
#else

/* Old RFC 1213 mib2 interface */

#define FEI_HADDR(pEnd)                                                 \
        ((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define FEI_HADDR_LEN(pEnd)                                             \
        ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

#endif /* INCLUDE_RFC_2233 */

/* Control Status Register definitions, some of them came from if_fei.h */

#define CSR_STAT_OFFSET		SCB_STATUS	/* CSR status byte */
#define CSR_ACK_OFFSET		0x01		/* CSR acknowledge byte */
#define CSR_COMM_OFFSET		SCB_CMD		/* CSR command byte */
#define CSR_INT_OFFSET		0x03		/* CSR Interrupt byte */
#define CSR_GP_OFFSET		SCB_POINTER	/* CSR General Pointer */
#define CSR_PORT_OFFSET		SCB_PORT	/* CSR PORT Register */
#define CSR_FLASH_OFFSET	SCB_FLASH	/* CSR FLASH Register */
#define CSR_EEPROM_OFFSET	SCB_EEPROM	/* CSR EEPROM Register */
#define CSR_MDI_OFFSET		SCB_MDI		/* CSR MDI Register */
#define CSR_RXBC_OFFSET		SCB_EARLYRX	/* CSR early RCV Byte Count */

/* Control Status Registers read/write macros */

/* Control Status Registers write macros */

/*
 * CSR_BYTE_WR, CSR_WORD_WR, and CSR_LONG_WR have a CACHE_PIPE_FLUSH() 
 * and CSR_BYTE_RD (CSR_INT_OFFSET, (UINT8) tmp) embedded in them.
 * The CSR_BYTE_RD is to force the write data through any write posting queues
 * it may be stuck while crossing the pci. The CACHE_PIPE_FLUSH() is necessary
 * to ensure the proper order of execution. The choice of reading the interrupt
 * mask register is not significant except that it is a convient location 
 * which has no side effects when read.
 */  

#define CSR_BYTE_WR(offset, value)					\
    {									\
    UINT8 tmp;  							\
    FEI_BYTE_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value));      \
    CACHE_PIPE_FLUSH();							\
    CSR_BYTE_RD (CSR_INT_OFFSET, tmp); 	                                \
    }	

#define CSR_WORD_WR(offset, value)					\
    {									\
    UINT8 tmp; 								\
    FEI_WORD_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value));	\
    CACHE_PIPE_FLUSH();							\
    CSR_BYTE_RD (CSR_INT_OFFSET, tmp);                                  \
    } 

#define CSR_LONG_WR(offset, value)					\
    {									\
    UINT8 tmp; 								\
    FEI_LONG_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value));	\
    CACHE_PIPE_FLUSH();							\
    CSR_BYTE_RD (CSR_INT_OFFSET, tmp);                                  \
    } 

/* this is a special case, as the device will read it as an address */

#define CSR_GP_WR(value)						\
    do {								\
    volatile UINT32 temp = FEI_VIRT_TO_SYS (value);			\
    									\
    CSR_LONG_WR (CSR_GP_OFFSET, (temp));				\
    } while (0)

/* Control Status Registers read macros */

#define CSR_BYTE_RD(offset, value)					\
    FEI_BYTE_RD ((UINT32 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

#define CSR_WORD_RD(offset, value)					\
    FEI_WORD_RD ((UINT32 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

#define CSR_LONG_RD(offset, value)					\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

/* FD rings available */
 
#define CFD_FREE                0x01            /* CFD free ring */
#define CFD_USED                0x02            /* CFD used ring */
#define RFD_FREE                0x04            /* RFD free ring */
 
#define CFD_COMM_WORD           0x01            /* CFD command word */
#define CFD_STAT_WORD           0x02            /* CFD status word */
#define RFD_COMM_WORD           0x04            /* RFD command word */
#define RFD_STAT_WORD           0x08            /* RFD status word */
 
#define CFD_ACTION              0x01            /* generic action command */
#define CFD_TX                  0x02            /* transmit command */

/* frame descriptors macros: these are generic among RFDs and CFDs */

#define FD_FLAG_ISSET(fdStat, bits)                                     \
    (((UINT16) (fdStat)) & ((UINT16) (bits)))

/* get the pointer to the appropriate frame descriptor ring */

#define FREE_CFD_GET(pCurrFD)						\
    ((pCurrFD) = pDrvCtrl->pFreeCFD)

#define USED_CFD_GET(pCurrFD)						\
    ((pCurrFD) = pDrvCtrl->pUsedCFD)

/* command frame descriptors write macros */

#define CFD_BYTE_WR(base, offset, value)				\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define CFD_WORD_WR(base, offset, value)				\
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define CFD_LONG_WR(base, offset, value)				\
    FEI_LONG_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

/* this is a special case, as the device will read as an address */

#define CFD_NEXT_WR(base, value)					\
    do {								\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS (value);		\
    									\
    CFD_LONG_WR ((UINT32) (base), CFD_NEXT_OFFSET, (temp));		\
    } while (0)

/* this is a special case, as the device will read as an address */

#define CFD_TBD_WR(base, value)						\
    do {								\
    volatile UINT32 temp;                                               \
    temp = (value == TBD_NOT_USED) ? value :                            \
                   ((UINT32) FEI_VIRT_TO_SYS ((UINT32) (value)));       \
    CFD_LONG_WR ((UINT32) (base), CFD_TBD_ADDR_OFFSET, (temp));		\
    } while (0)

/* receive frame descriptors write macros */

#define RFD_BYTE_WR(base, offset, value)				\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define RFD_WORD_WR(base, offset, value)				\
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define RFD_LONG_WR(base, offset, value)				\
    FEI_LONG_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

/* this is a special case, as the device will read as an address */

#define RFD_NEXT_WR(base, value)					\
    do {								\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS ((UINT32) (value));	\
    									\
    RFD_LONG_WR ((UINT32) (base), RFD_NEXT_OFFSET, (temp));		\
    } while (0)

/* this is a special case, as the device will read as an address */

#define RFD_RBD_WR(base, value)						\
    do {								\
    volatile UINT32 temp;                                               \
    temp = (value == RBD_NULL_ADDR) ? value :                            \
                  ((UINT32) FEI_VIRT_TO_SYS ((UINT32) (value)));        \
    RFD_LONG_WR ((UINT32) (base), RFD_RBD_OFFSET, (temp));		\
    } while (0)


/* receive buffer descriptors write macros */

#define RBD_BYTE_WR(base, offset, value)                                \
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (base) + (offset)),               \
             (value))

#define RBD_WORD_WR(base, offset, value)                                \
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)),               \
             (value))

#define RBD_LONG_WR(base, offset, value)                                \
    FEI_LONG_WR ((UINT32 *) ((UINT32) (base) + (offset)),               \
             (value))

#ifdef notdef
#define RBD_LONG_WR(base, offset, value)                                \
    do {                                                                \
    volatile UINT32 myVal = value;                                      \
    volatile UINT16 *tempVal;                                           \
                                                                        \
    tempVal = (UINT16 *)&myVal;                                         \
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)),               \
             (tempVal[0]));                                             \
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset) + 2),           \
             (tempVal[1]));                                             \
    } while (0)
#endif

/* these are special cases, as the device will read an address */

#define RBD_NEXT_WR(base, value)                                        \
    do {                                                                \
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS ((UINT32) (value)); \
                                                                        \
    RBD_LONG_WR ((UINT32) (base), RBD_NEXT_OFFSET, (temp));             \
    } while (0)

#define RBD_BUF_WR(base, value)                                        \
    do {                                                                \
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS ((UINT32) (value)); \
                                                                        \
    RBD_LONG_WR ((UINT32) (base), RBD_BUFFER_OFFSET, (temp));             \
    } while (0)

#define RBD_BUF_RD(base, value)                                        \
    do {                                                               \
    volatile UINT32 temp;                                              \
    RBD_LONG_RD ((UINT32) (base), RBD_BUFFER_OFFSET, (temp));          \
    value = (CL_BUF_ID) FEI_SYS_TO_VIRT ((UINT32) (temp));                \
    } while (0)

/* command frame descriptors read macros */

#define CFD_BYTE_RD(base, offset, value)				\
    FEI_BYTE_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define CFD_WORD_RD(base, offset, value)				\
    FEI_WORD_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define CFD_LONG_RD(base, offset, value)				\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define CFD_POINT_RD(base, offset, value, type)                         \
    {                                                                   \
    volatile UINT32 temp;                                               \
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)),(temp));       \
    value = (type)temp;                                                 \
    }

/* this is a special case, as the device will read as an address */

#define CFD_NEXT_RD(base, value)					\
    CFD_LONG_RD ((UINT32) (base), CFD_SW_NEXT_OFFSET, (value))

/* receive frame descriptors read macros */

#define RFD_BYTE_RD(base, offset, value)				\
    FEI_BYTE_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define RFD_WORD_RD(base, offset, value)				\
    FEI_WORD_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define RFD_LONG_RD(base, offset, value)				\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

/* receive buffer descriptors read macros */

#define RBD_BYTE_RD(base, offset, value)                                \
    FEI_BYTE_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define RBD_WORD_RD(base, offset, value)                                \
    FEI_WORD_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

#define RBD_LONG_RD(base, offset, value)                                \
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

/* various command frame descriptors macros */

#define CFD_PKT_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) cfdBase + CFD_PKT_OFFSET))

#define RFD_PKT_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) cfdBase + RFD_PKT_OFFSET))

#define CFD_IA_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) (cfdBase) + CFD_IA_OFFSET))

#define CFD_MC_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) (cfdBase) + CFD_MC_OFFSET))

#define CFD_CONFIG_WR(address, value)					\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (address)),          		\
	     (value))

#define I82557_INT_ENABLE(value)					\
    {									\
    UINT8 temp;								\
    CACHE_PIPE_FLUSH();                                                 \
    CSR_BYTE_RD (CSR_INT_OFFSET, temp);				\
    CSR_BYTE_WR (CSR_INT_OFFSET, (temp & ~value));		\
    }	

#define I82557_INT_DISABLE(value)					\
    {									\
    UINT8 temp;								\
    CACHE_PIPE_FLUSH();                                                 \
    CSR_BYTE_RD (CSR_INT_OFFSET, temp);				\
    CSR_BYTE_WR (CSR_INT_OFFSET, (temp | value));		\
    }	

/* extern */

IMPORT POOL_FUNC *     _pEndNetPoolFuncTbl; 
IMPORT int ffsMsb ();


FUNCPTR feiEndIntConnect = (FUNCPTR) intConnect;
FUNCPTR feiEndIntDisconnect = (FUNCPTR) NULL;

/* locals */

/* The definition of the driver control structure */

typedef struct drv_ctrl
    {
    END_OBJ        	endObj;		/* base class */
    int		   	unit;		/* unit number */
    FUNCPTR             pSendRtn;
    int	    		nRFDs;		/* number of RFDs on DMA ring  */
    int	    		nRBDs;		/* number of RBDs on DMA ring  */
    int	    		nCFDs;		/* how many CFDs to create */
    char *              pRfdBase;       /* RFD allocation base */
    char *              pRbdBase;       /* RBD allocation base */
    char *              pCfdBase;       /* CFD allocation base */
    char *		pClusterBase;	/* cluster pool base */
    int			nClusters;	/* number of clusters to create */
    int                 clToRfdRatio;   /* Ratio of clusters to RFDs */
    ULONG		clMemSize;	/* cluster pool size */
    char *		pMclBlkMemArea;	/* clBlk mBlk memory area pointer */
    volatile CSR_ID	pCSR;		/* pointer to CSR base */
    volatile CFD_ID	pFreeCFD;	/* current free CFD */
    volatile CFD_ID	pUsedCFD;	/* first used CFD */
    volatile RFD_ID     pRFD;		/* current Receive Frame Descriptor */
    volatile RBD_ID	pRBD;		/* current Receive Buffer Descriptor */
    RFD_TAG *           rfdTags;        /* Array of RFD_TAGs */
    RBD_TAG *           rbdTags;        /* Array of RBD_TAGs */
    int                 rbdIndex;       /* current RX index  */
    int                 rfdIndex;       /* current RX index  */
    RBD_TAG *           eLRbdTag;       /* RBD that currently has EL bit set */
    RBD_TAG *           startRbdTag;    /* RBD that RU was last started at */
    INT8		flags;		/* driver state */
    BOOL		attached;	/* interface has been attached */
    volatile BOOL	rxHandle;	/* rx handler scheduled */
    BOOL		txHandle;	/* tx handler scheduled */
    BOOL		txStall;	/* tx handler stalled - no CFDs */
    UINT                maxRxFrames;	/* max frames to Receive in one job */
    BOOL		rxJobQued;	/* fei82557RecvHandler() queing flag */
    CACHE_FUNCS		cacheFuncs;	/* cache descriptor */
    CACHE_FUNCS		cacheDmaFuncs;	/* cache descriptor */
    CACHE_FUNCS		cacheUserFuncs;	/* cache descriptor */
    FEI_BOARD_INFO  	board;		/* board specific info */
    CL_POOL_ID  	pClPoolId;	/* cluster pool identifier */
    int			offset;		/* Alignment offset */
    UINT                deviceId;	/* PCI device ID */
    END_ERR             lastError;      /* Last error passed to muxError */
    UINT                errorNoBufs;    /* cluster exhaustion */
    WDOG_ID             txRetryWDId;    /* Tx restart watchdog */
    UINT16  		event;		/* storage for interrupt events */ 
    } DRV_CTRL;

#ifdef DRV_DEBUG557 

void feiPoolShow
    (
    int unit
    )
    {
    DRV_CTRL *pDrvCtrl = (DRV_CTRL *)endFindByName ("fei", unit);

    netPoolShow (pDrvCtrl->endObj.pNetPool);

    }

#endif  /* DRV_DEBUG557 */

/* Function declarations not in any header files */

IMPORT STATUS    sys557Init (int unit, FEI_BOARD_INFO *pBoard);

/* forward function declarations */

LOCAL int	fei82557ClkRate = 0;

LOCAL STATUS    fei82557InitParse (DRV_CTRL *pDrvCtrl, char *initString);
LOCAL STATUS    fei82557InitMem (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Encap (DRV_CTRL *pDrvCtrl, CFD_ID pCFD,
                        M_BLK *pMblkHead);
LOCAL STATUS    fei82557Send (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);
LOCAL STATUS    fei82557GatherSend (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);
LOCAL STATUS    fei82557CopySend (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);

LOCAL UINT16	fei82557Action (DRV_CTRL *pDrvCtrl, UINT16 action);
LOCAL STATUS    fei82557PhyInit (DRV_CTRL *pDrvCtrl);
LOCAL STATUS	fei82557Stop (DRV_CTRL *pDrvCtrl);
LOCAL STATUS 	fei82557Reset (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557SCBCommand (DRV_CTRL *pDrvCtrl, UINT8 cmd, 
			       BOOL addrValid, UINT32 *addr);
LOCAL STATUS    fei82557Diag (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557IASetup (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Config (DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557MCastListForm (DRV_CTRL *pDrvCtrl, CFD_ID pCFD);
LOCAL void	fei82557ConfigForm (DRV_CTRL *pDrvCtrl, CFD_ID pCFD);
LOCAL int	fei82557MDIPhyLinkSet (DRV_CTRL *pDrvCtrl, int phyAddr);
LOCAL STATUS    fei82557NOP (DRV_CTRL *pDrvCtrl);
LOCAL void      fei82557CFDFree ( DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557FDUpdate (DRV_CTRL *pDrvCtrl, UINT8 fdList);
LOCAL STATUS	fei82557MDIPhyConfig (DRV_CTRL *pDrvCtrl, int phyAddr);
LOCAL void	fei82557Int (DRV_CTRL *pDrvCtrl);
LOCAL void      fei82557NoResource(DRV_CTRL *pDrvCtrl);
LOCAL void      fei82557MuxTxRestart( END_OBJ *pEndObj);
LOCAL void      fei82557RecvHandler (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Restart (DRV_CTRL *  pDrvCtrl);
LOCAL int	fei82557MDIRead (DRV_CTRL *pDrvCtrl, int regAddr,
			    int phyAddr, UINT16 *retVal);
LOCAL int	fei82557MDIWrite (DRV_CTRL *pDrvCtrl, int regAddr,
			    int phyAddr, UINT16 writeData);
/* debug routines, not normally compiled */
      STATUS    fei82557ErrCounterDump (DRV_CTRL *pDrvCtrl, UINT32 *memAddr);
      STATUS    fei82557DumpPrint (int unit);
LOCAL void	fei82557TxRestart (END_OBJ *pEndObj);
/* END Specific interfaces. */

END_OBJ *	fei82557EndLoad (char *initString);    
LOCAL STATUS    fei82557Start (DRV_CTRL *pDrvCtrl);
LOCAL STATUS	fei82557Unload (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Stop (DRV_CTRL *pDrvCtrl);
LOCAL int       fei82557Ioctl (DRV_CTRL *pDrvCtrl, UINT32 cmd, caddr_t data);
LOCAL STATUS    fei82557Send (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557MCastAddrAdd (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    fei82557MCastAddrDel (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    fei82557MCastAddrGet (DRV_CTRL *pDrvCtrl,
                                        MULTI_TABLE *pTable);
LOCAL STATUS    fei82557PollSend (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557PollReceive (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557PollStart (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557PollStop (DRV_CTRL *pDrvCtrl);


/* 
 * Define the device function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS netFuncs = 
    {
    (FUNCPTR)fei82557Start,		/* start func. */		 
    (FUNCPTR)fei82557Stop,		/* stop func. */
    (FUNCPTR)fei82557Unload,		/* unload func. */		
    (FUNCPTR)fei82557Ioctl,		/* ioctl func. */		 
    (FUNCPTR)fei82557Send,      	/* send func. */		  
    (FUNCPTR)fei82557MCastAddrAdd,    	/* multicast add func. */	 
    (FUNCPTR)fei82557MCastAddrDel,    	/* multicast delete func. */      
    (FUNCPTR)fei82557MCastAddrGet,    	/* multicast get fun. */	  
    (FUNCPTR)fei82557PollSend,    	/* polling send func. */	  
    (FUNCPTR)fei82557PollReceive,    	/* polling receive func. */
    endEtherAddressForm,   	/* put address info into a NET_BUFFER. */
    endEtherPacketDataGet, 	/* get pointer to data in NET_BUFFER. */
    endEtherPacketAddrGet  	/* Get packet addresses. */
    };		

/*******************************************************************************
*
* fei82557EndLoad - initialize the driver and device
*
* This routine initializes both, driver and device to an operational state
* using device specific parameters specified by <initString>.
*
* The parameter string, <initString>, is an ordered list of parameters each
* separated by a colon. The format of <initString> is,
* "<unit>:<memBase>:<memSize>:<nCFDs>:<nRFDs>:<flags>:<offset>:<deviceId>:
*  <maxRxFrames>:<clToRfdRatio>:<nClusters>"
*
*
* The 82557 shares a region of memory with the driver.  The caller of this
* routine can specify the address of this memory region, or can specify that
* the driver must obtain this memory region from the system resources.
*
* A default number of transmit/receive frames of 32 and 128 respectively and 
* can be selected by passing zero in the parameters <nTfds> and <nRfds>. In 
* other cases, the number of frames selected should be greater than two.
*
* All optional parameters can be set to their default value by specifing 
* NONE (-1) as their value. 
*
* The <memBase> parameter is used to inform the driver about the shared
* memory region.  If this parameter is set to the constant "NONE," then this
* routine will attempt to allocate the shared memory from the system.  Any
* other value for this parameter is interpreted by this routine as the address
* of the shared memory region to be used. The <memSize> parameter is used
* to check that this region is large enough with respect to the provided
* values of both transmit/receive frames.
*
* If the caller provides the shared memory region, then the driver assumes
* that this region is non-cached. 
*
* If the caller indicates that this routine must allocate the shared memory
* region, then this routine will use memalign() to allocate some cache aligned 
* memory. 
*
* The <memSize> parameter specifies the size of the pre-allocated memory
* region. If memory base is specified as NONE (-1), the driver ignores this
* parameter. Otherwise, the driver checks the size of the provided memory
* region is adequate with respect to the given number of RFDs, RBDs, CFDs, and
* clusters specified. The number of clusters required will be at least equal 
* to (nRFDs * 2) + nCFDs. Otherwise the End Load routine will return ERROR. 
* The number of clusters can be specified by either passing a value in the 
* nCluster parameter, in which case the nCluster value must be at least 
* nRFDs * 2, or by setting the cluster to RFD ratio (clToRfdRatio) to a number 
* equal or greater than 2.
* 
* 
* The <nTfds> parameter specifies the number of transmit descriptor/buffers 
* to be allocated. If this parameter is less than two, a default of 64 is used.
*
* The <nRfds> parameter specifies the number of receive descriptors to be
* allocated. If this parameter is less than two or NONE (-1) a default of 
* 128 is used.
* 
* The <flags> parameter specifies the user flags may control the run-time 
* characteristics of the Ethernet chip. Not implemented.
* 
* The <offset> parameter is used to align IP header on word boundary for CPUs 
* that need long word aligned access to the IP packet (this will normally be 
* zero or two). This parameter is optional, the default value is zero.
*
* The <deviceId> parameter is used to indicate the specific type of device 
* being used, the 82557 or subsequent.  This is used to determine if features 
* which were introduced after the 82557 can be used. The default is the 82557. 
* If this is set to any value other than ZERO (0), NONE (-1), or 
* FEI82557_DEVICE_ID (0x1229) it is assumed that the device will support 
* features not in the 82557.
*
* The <maxRxFrames> parameter limits the number of frames the receive handler 
* will service in one pass. It is intended to prevent the tNetTask from 
* monoploizing the CPU and starving applications. This parameter is optional, 
* the default value is nRFDs * 2.
* 
* The <clToRfdRatio> parameter sets the number of clusters as a ratio of nRFDs. 
* The minimum setting for this parameter is 2. This parameter is optional, the
* default value is 5.
* 
* The <nClusters> parameter sets the number of clusters to allocate. This value 
* must be  at least nRFD * 2.  If this value is set then the <clToRfdRatio> is 
* ignored. This parameter is optional, the default is nRFDs * clToRfdRatio.
* 
* RETURNS: an END object pointer, or NULL on error.
*
* SEE ALSO: ifLib,
* .I "Intel 82557 User's Manual"
*/

END_OBJ* fei82557EndLoad
    (
    char *initString      /* parameter string */
    )
    {
    DRV_CTRL *	pDrvCtrl;       /* pointer to DRV_CTRL structure */
    UCHAR   	enetAddr[6];	/* ethernet address */
    UINT32	speed;
    UINT32	scbStatus;
    char        bucket[2];

    DRV_LOG (DRV_DEBUG_LOAD, ("Loading end\n"), 1, 2, 3, 4, 5, 6);

    if (initString == NULL)
	return (NULL);

    if (initString[0] == 0)
	{
	bcopy ((char *)DEV_NAME, (void *)initString, DEV_NAME_LEN);
	return (0);
	}

    /* allocate the device structure */

    pDrvCtrl = (DRV_CTRL *) calloc (sizeof (DRV_CTRL), 1);

    if (pDrvCtrl == NULL)
	return (NULL);

    /* Parse InitString */
printf("FEI initstring = %s\n", initString);
    if (fei82557InitParse (pDrvCtrl, initString) == ERROR)
	goto errorExit;	

    /* sanity check the unit number */
    if (pDrvCtrl->unit < 0 )
	goto errorExit;    

    /* Initialize pDrvCtrl->rbdIndex */
    pDrvCtrl->rbdIndex = 0;
    pDrvCtrl->rfdIndex = 0;

    /* 
     * initialize the default parameter for the Physical medium 
     * layer control user has his chance to override in the BSP, 
     * just be CAREFUL 
     */

    pDrvCtrl->board.phyAddr  = 1;
    pDrvCtrl->board.phySpeed = PHY_AUTO_SPEED; /*wangfq*/
    pDrvCtrl->board.phyDpx   = PHY_AUTO_DPX;
    pDrvCtrl->board.others   = 0;
    pDrvCtrl->board.tcbTxThresh = FEI_TCB_TX_THRESH;

    /* callout to perform adapter init */

    if (sys557Init (pDrvCtrl->unit, &pDrvCtrl->board) == ERROR)
	goto errorExit;

    /* get CSR address from the FEI_BOARD_INFO structure */

    if ((pDrvCtrl->pCSR = (CSR_ID) pDrvCtrl->board.baseAddr) == NULL)
	goto errorExit;

    /* probe for memory-mapped CSR */

    CACHE_PIPE_FLUSH();
    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if (vxMemProbe ((char *) &scbStatus, VX_READ, 2,
		    &bucket[0]) != OK)
	{
	DRV_LOG (DRV_DEBUG_LOAD,
		   (": need MMU mapping for address 0x%x\n"),
		   (UINT32) pDrvCtrl->pCSR, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* memory initialization */

    if (fei82557InitMem (pDrvCtrl) == ERROR)
	goto errorExit;

    I82557_INT_DISABLE(SCB_C_M);

    /* initialize the Physical medium layer */

    if (fei82557PhyInit (pDrvCtrl) != OK)
	{
        DRV_LOG (DRV_DEBUG_LOAD,"Check line connection.\n",0,0,0,0,0,0); 
	}

    speed = ((((int) pDrvCtrl->board.phySpeed) == PHY_100MBS) ?
		FEI_100MBS : FEI_10MBS);

    if (fei82557ClkRate == 0)
	fei82557ClkRate = sysClkRateGet ();

    /* Create TX restart watchdog ID */

    pDrvCtrl->txRetryWDId = wdCreate();

    if(pDrvCtrl->txRetryWDId == NULL)
        {
	DRV_LOG (DRV_DEBUG_LOAD, "failed to create TX watchdog ID\n",
                 0, 0, 0, 0, 0, 0);
        goto errorExit;
        }

    /* 
     * reset the chip: this should be replaced by a true 
     * adapter reset routine, once the init code is here.
     */

    if (fei82557Reset (pDrvCtrl) != OK)
	goto errorExit;

    /* CU and RU should be idle following fei82557Reset() */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CULDBASE, TRUE, 0x0) == ERROR)
	goto errorExit;

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_RULDBASE, TRUE, 0x0) == ERROR)
	goto errorExit;

    pDrvCtrl->attached = TRUE;

    /* get our ethernet hardware address */

    bcopy ((char *)&pDrvCtrl->board.enetAddr,
	   (char *)&enetAddr[0],
	   FEI_ADDR_LEN);

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Load...\n
			 ADRR: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n "), 
			enetAddr[0],
			enetAddr[1],
			enetAddr[2],
			enetAddr[3],
			enetAddr[4],
			enetAddr[5]);

    /* endObj initializations */

    if (END_OBJ_INIT (&pDrvCtrl->endObj, (DEV_OBJ*) pDrvCtrl,
		      DEV_NAME, pDrvCtrl->unit, &netFuncs,
		      "Intel 82557 Ethernet Enhanced Network Driver") == ERROR)
	goto errorExit;

#ifdef INCLUDE_RFC_2233

    /* Initialize MIB-II entries (for RFC 2233 ifXTable) */
    pDrvCtrl->endObj.pMib2Tbl = m2IfAlloc(M2_ifType_ethernet_csmacd,
                                          (UINT8*) enetAddr, 6,
                                          ETHERMTU, speed,
                                          DEV_NAME, pDrvCtrl->unit);

    if (pDrvCtrl->endObj.pMib2Tbl == NULL)
        {
        logMsg ("%s%d - MIB-II initializations failed\n",
                (int)DEV_NAME, pDrvCtrl->unit,0,0,0,0);
        goto errorExit;
        }
        
    /* 
     * Set the RFC2233 flag bit in the END object flags field and
     * install the counter update routines.
     */

    m2IfPktCountRtnInstall(pDrvCtrl->endObj.pMib2Tbl, m2If8023PacketCount);

    /*
     * Make a copy of the data in mib2Tbl struct as well. We do this
     * mainly for backward compatibility issues. There might be some
     * code that might be referencing the END pointer and might
     * possibly do lookups on the mib2Tbl, which will cause all sorts
     * of problems.
     */

    bcopy ((char *)&pDrvCtrl->endObj.pMib2Tbl->m2Data.mibIfTbl,
                   (char *)&pDrvCtrl->endObj.mib2Tbl, sizeof (M2_INTERFACETBL));

    /* Mark the device ready */

    END_OBJ_READY (&pDrvCtrl->endObj,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST |
                   END_MIB_2233);

#else
    /* Old RFC 1213 mib2 interface */

    if (END_MIB_INIT (&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd,
                      (u_char *) &enetAddr[0], FEI_ADDR_LEN,
                      ETHERMTU, speed) == ERROR)
        goto errorExit;

    /* Mark the device ready */

    END_OBJ_READY (&pDrvCtrl->endObj,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST);

#endif /* INCLUDE_RFC_2233 */

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Load... Done \n"), 1, 2, 3, 4, 5, 6);

    return (&pDrvCtrl->endObj);

errorExit:

    DRV_LOG (DRV_DEBUG_LOAD, "fei82557Load failed\n", 0,0,0,0,0,0);

    fei82557Unload (pDrvCtrl);
    free ((char *) pDrvCtrl);
    return NULL;

    }

/*******************************************************************************
*
* fei82557Unload - unload a driver from the system
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557Unload
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    
    DRV_LOG (DRV_DEBUG_LOAD, ("Unloading end..."), 1, 2, 3, 4, 5, 6);

#ifdef INCLUDE_RFC_2233
    /* Free MIB-II entries */
    m2IfFree(pDrvCtrl->endObj.pMib2Tbl);
    pDrvCtrl->endObj.pMib2Tbl = NULL;
#endif /* INCLUDE_RFC_2233 */

    pDrvCtrl->attached = FALSE;

    /* free lists */

    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

    /* free allocated memory if necessary */

    if ((FEI_FLAG_ISSET (FEI_OWN_MEM)) && 
	(pDrvCtrl->pClusterBase != NULL))
	cacheDmaFree (pDrvCtrl->pClusterBase);

    /* free allocated memory if necessary */

    cfree (pDrvCtrl->pMclBlkMemArea);

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Unload... Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);
    }

/*******************************************************************************
*
* fei82557InitParse - parse parameter values from initString
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS fei82557InitParse
    (
    DRV_CTRL *	pDrvCtrl,      		/* pointer to DRV_CTRL structure */
    char *	initString		/* parameter string */
    )

    {
    char *  tok;		/* an initString token */
    char *  holder = NULL;	/* points to initString fragment beyond tok */

    tok = strtok_r (initString, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->unit = atoi (tok);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->pClusterBase = (char *)strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->clMemSize = strtoul (tok, NULL, 16);

    /* passing nCFDs is optional. The default is 64 */     
    pDrvCtrl->nCFDs = 64;
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->nCFDs = strtoul (tok, NULL, 16);

    /* passing nRFDs is optional. The default is 128 */     
    pDrvCtrl->nRFDs = 128;
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->nRFDs = strtoul (tok, NULL, 16);
    pDrvCtrl->nRBDs = pDrvCtrl->nRFDs;

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->flags = atoi (tok);
	

    /* offset value is optional, default is zero */
    pDrvCtrl->offset = 0;
    tok = strtok_r (NULL, ":", &holder);
    if (tok != NULL)
	pDrvCtrl->offset = atoi (tok);
	#if 1
pDrvCtrl->offset = 0x02; /*wangfq*//*Note!!!!!*/
	#endif

    /* device ID is optional, default is zero */
    pDrvCtrl->deviceId = 0;
    tok = strtok_r (NULL, ":", &holder);
    if (tok != NULL)
	pDrvCtrl->deviceId = atoi (tok);
	#if 1
pDrvCtrl->deviceId = 0x1209; /*wangfq*/
	#endif

    /* passing maxRxFrames is optional. The default is 128  */
    pDrvCtrl->maxRxFrames = pDrvCtrl->nRFDs * 2;   
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->maxRxFrames = strtoul (tok, NULL, 16);

    /* passing clToRfdRatio is optional. The default is 5  */
    pDrvCtrl->clToRfdRatio = 5;
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->clToRfdRatio = strtoul (tok, NULL, 16);

    if (pDrvCtrl->clToRfdRatio < 2 )
        pDrvCtrl->clToRfdRatio = 2;

    /* passing nClusters is optional. The default is  nRFDs * clToRfdRatio */
    pDrvCtrl->nClusters = pDrvCtrl->nRFDs * pDrvCtrl->clToRfdRatio;
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->nClusters = strtoul (tok, NULL, 16);

    if (!pDrvCtrl->nCFDs || pDrvCtrl->nCFDs <= 2)
	{
	FEI_FLAG_SET (FEI_INV_NCFD);
	pDrvCtrl->nCFDs = DEF_NUM_CFDS;
	}

    if (!pDrvCtrl->nRFDs || pDrvCtrl->nRFDs <= 2)
	{
	FEI_FLAG_SET (FEI_INV_NRFD);
	pDrvCtrl->nRFDs = DEF_NUM_RFDS;
	}

    if (pDrvCtrl->nClusters < (pDrvCtrl->nRFDs * 2))
        pDrvCtrl->nClusters = pDrvCtrl->nRFDs * 2;

    DRV_LOG (DRV_DEBUG_LOAD,
	    "fei82557EndLoad: unit=%d pClusterBase=0x%x memSize=0x%x\n",
	    pDrvCtrl->unit, (int) pDrvCtrl->pClusterBase,
	    (int) pDrvCtrl->clMemSize, 0,0,0);
    DRV_LOG (DRV_DEBUG_LOAD,
	    "fei82557EndLoad: nCFDs=%d nRFDs=%d flags=%d offset=%d\n",
	    pDrvCtrl->nCFDs, pDrvCtrl->nRFDs, pDrvCtrl->flags,
	    pDrvCtrl->offset, 0, 0);

    return (OK);
    }

/*******************************************************************************
*
* fei82557InitMem - initialize memory
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS fei82557InitMem
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    CFD_ID		pCFD;	   	/* pointer to CFDs */
    RFD_ID		pRFD;	   	/* pointer to RFDs */
    RFD_TAG *   	pRfdTag;
    RBD_ID		pRBD;	   	/* pointer to RFDs */
    RBD_TAG *   	pRbdTag;
    UINT32		clSize;	   	/* size of allocation for clusters */
    UINT32		clusterSize;   	/* size of allocation for clusters */
    UINT32		memSize;   	/* size of allocation passed by BSP */
    UINT32		nSubtract = 1; 	/* number of RFDs to subtract */
    UINT32		rfdSize;   	/* size of allocation for RFDs */
    UINT32		rbdSize;   	/* size of allocation for RBDs */
    UINT32		cfdSize;   	/* size of allocation for CFDs */
    int			ix;		/* index counter */
    BOOL		firstCFD = TRUE;/* the first CFD? */
    int                 firstOrLast;
char * a;    

    M_CL_CONFIG	 fei82557MclBlkConfig =
    {
	/*
	 *  mBlkNum  	clBlkNum     	memArea     memSize
	 *  -----------  	----		-------     -------
	 */
	    0,		0, 		NULL, 	    0
    };

    CL_DESC	 fei82557ClDescTbl [] =
    {
	/*
	 *  clusterSize  		num    	memArea     memSize
	 *  -----------  		----	-------     -------
	 */

	{CLUSTER_SIZE,			0,  	NULL,       0}    

    };

    int fei82557ClDescTblNumEnt = (NELEMENTS (fei82557ClDescTbl));

    /* initialize the netPool */
a=memalign(32,128);printf("===%x===\n", a);
    if ((pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL)
	return (ERROR);
#if 1
   /* this driver can't handle write incoherent caches */
    if (!CACHE_DMA_IS_WRITE_COHERENT ()) 
	{
	DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: shared 
			          memory not cache coherent\n"),
			          1, 2, 3, 4, 5, 6);
	return (ERROR);
	}
#endif
    switch ((int) pDrvCtrl->pClusterBase)
	{
	case NONE :			     /* we must obtain it */

            clSize = (pDrvCtrl->nClusters * CLUSTER_SIZE);
printf("pDrvCtrl->nClusters is %d *\n", pDrvCtrl->nClusters);
            /* Add one to rfdSize, rbdSize, & cfdSize to accomodate alignment */
            rfdSize = ((pDrvCtrl->nRFDs + 1) * RFD_DESC_SIZE);
            rbdSize = ((pDrvCtrl->nRBDs + 1) * RBD_SIZE);
            cfdSize = ((pDrvCtrl->nCFDs + 1) * CFD_SIZE); /* CFD_SIZE = 1568 */

            /* Align on 32 byte boundries */
            if ((pDrvCtrl->pRfdBase = cacheDmaMalloc (rfdSize + rbdSize + 
                                                      cfdSize)) == NULL)
                {
                return (ERROR);
                }
printf("pDrvCtrl->pRfdBase is %.8x *\n", (char *)pDrvCtrl->pRfdBase);
            memset (pDrvCtrl->pRfdBase, 0, (rfdSize + rbdSize + cfdSize));

            pDrvCtrl->pRfdBase = (char *)ROUND_UP((UINT32) pDrvCtrl->pRfdBase, 
                                                  32);

            pDrvCtrl->rfdTags = (RFD_TAG *)pDrvCtrl->pRfdBase;
            pDrvCtrl->pRFD = (RFD_ID)pDrvCtrl->pRfdBase;

            pDrvCtrl->pRbdBase = (char *)((int)pDrvCtrl->pRfdBase + rfdSize);
            pDrvCtrl->rbdTags = (RBD_TAG *)pDrvCtrl->pRbdBase;

            pDrvCtrl->pCfdBase = (char *)((int)pDrvCtrl->pRbdBase + rbdSize);

            pDrvCtrl->pClusterBase = memalign (_CACHE_ALIGN_SIZE, clSize); 
printf("pDrvCtrl->pClusterBase is %.8x *\n", (char *)pDrvCtrl->pClusterBase);
	    if (pDrvCtrl->pClusterBase == NULL)      /* no memory available */
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: could not 
					    obtain memory\n"),
					    1, 2, 3, 4, 5, 6);
		return (ERROR);
		}
            /* zero the shared memory */
            memset(pDrvCtrl->pClusterBase,0,clSize);

	    pDrvCtrl->clMemSize = clSize;

	    FEI_FLAG_SET (FEI_OWN_MEM);

	    pDrvCtrl->cacheDmaFuncs = cacheDmaFuncs;
            pDrvCtrl->cacheFuncs = cacheUserFuncs;
DRV_LOG (DRV_DEBUG_LOAD, ("pDrvCtrl->cacheDmaFuncs = cacheDmaFuncs\npDrvCtrl->cacheFuncs = cacheUserFuncs\n"),
					    1, 2, 3, 4, 5, 6);
            pDrvCtrl->pSendRtn = (FUNCPTR)fei82557GatherSend; 

	    break;

	default :			       /* the user provided an area */

	    if (pDrvCtrl->clMemSize == 0) 
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: not enough 
					   memory\n"),
					   1, 2, 3, 4, 5, 6);
		return (ERROR);
		}

	    /* 
	     * check the user provided enough memory with reference
	     * to the given number of receive/transmit frames, if any.
	     */

           while ((pDrvCtrl->nRBDs * RBD_SIZE) > (nSubtract * CLUSTER_SIZE))
               {
               nSubtract++;
               }
            rfdSize = ((pDrvCtrl->nRFDs + 1) * RFD_DESC_SIZE);
            rbdSize = ((pDrvCtrl->nRBDs + 1) * RBD_SIZE);
            cfdSize = (pDrvCtrl->nCFDs * (CFD_SIZE + CLUSTER_SIZE)); 
            clusterSize = (((pDrvCtrl->nRFDs + 1) * 2) * CLUSTER_SIZE); 

            memSize = rfdSize + rbdSize + cfdSize + clusterSize;

            if (pDrvCtrl->clMemSize < memSize) 
	        {
	        DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: not enough 
	                                  memory\n"), 1, 2, 3, 4, 5, 6);
	        return (ERROR);
	        }

            /* zero the shared memory */
            memset(pDrvCtrl->pClusterBase,0,pDrvCtrl->clMemSize);

            memSize = pDrvCtrl->clMemSize - (rfdSize + rbdSize + cfdSize);

            pDrvCtrl->nClusters = ((memSize / CLUSTER_SIZE) - nSubtract); 

            pDrvCtrl->pRfdBase = pDrvCtrl->pClusterBase;
            pDrvCtrl->rfdTags = (RFD_TAG *)pDrvCtrl->pRfdBase; 
            pDrvCtrl->pRFD = (RFD_ID)pDrvCtrl->pRfdBase;

            pDrvCtrl->pRbdBase = (char *)((int)pDrvCtrl->pRfdBase + rfdSize);
            pDrvCtrl->rbdTags = (RBD_TAG *)pDrvCtrl->pRbdBase; 

            pDrvCtrl->pClusterBase = (char *)((int)pDrvCtrl->pRbdBase + 
                                                   rbdSize);

            pDrvCtrl->pCfdBase = (char *)((int)pDrvCtrl->pClusterBase + 
                                (pDrvCtrl->nClusters * CLUSTER_SIZE));  

	    FEI_FLAG_CLEAR (FEI_OWN_MEM);
	    pDrvCtrl->cacheDmaFuncs = cacheNullFuncs; 
            pDrvCtrl->cacheFuncs = cacheDmaFuncs;

            pDrvCtrl->pSendRtn = (FUNCPTR)fei82557CopySend; 

	    break;
	}

    /* pool of mblks */

    if (fei82557MclBlkConfig.mBlkNum == 0)
	fei82557MclBlkConfig.mBlkNum = pDrvCtrl->nClusters * 10;

    /* pool of clusters */

    if (fei82557ClDescTbl[0].clNum == 0)
	{
	fei82557ClDescTbl[0].clNum = pDrvCtrl->nClusters;
	fei82557ClDescTbl[0].clSize = CLUSTER_SIZE;
	}

    fei82557ClDescTbl[0].memSize = pDrvCtrl->clMemSize;  
    fei82557ClDescTbl[0].memArea = pDrvCtrl->pClusterBase;

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: pClusterBase %p nClusters %d 
                              cluster size %x\n"),
                              (int)pDrvCtrl->pClusterBase,pDrvCtrl->nClusters,
                              CLUSTER_SIZE,0,0,0); 

    /* pool of cluster blocks */

    if (fei82557MclBlkConfig.clBlkNum == 0)
	fei82557MclBlkConfig.clBlkNum = (fei82557ClDescTbl[0].clNum * 2);

    /* get memory for mblks */

    if (fei82557MclBlkConfig.memArea == NULL)
	{
	/* memory size adjusted to hold the netPool pointer at the head */

        fei82557MclBlkConfig.memSize = (((fei82557MclBlkConfig.mBlkNum + 1) *
                                       MBLK_SIZE)  +
                                       ((fei82557MclBlkConfig.clBlkNum + 1) *
                                       CLBLK_SIZE));

	if ((fei82557MclBlkConfig.memArea = 
                                 (char *) (memalign (_CACHE_ALIGN_SIZE, 
                                           fei82557MclBlkConfig.memSize)) +
                                          (MBLK_SIZE - sizeof(long)))
	    == NULL)
	    return (ERROR);

	/* store the pointer to the cluster block area */
	pDrvCtrl->pMclBlkMemArea = fei82557MclBlkConfig.memArea;
	}
printf("pDrvCtrl->pMclBlkMemArea is %.8x *\n", (char *)pDrvCtrl->pMclBlkMemArea);
    /* init the mem pool */

    if (netPoolInit (pDrvCtrl->endObj.pNetPool, &fei82557MclBlkConfig, 
		     &fei82557ClDescTbl[0], fei82557ClDescTblNumEnt, 
                     _pEndNetPoolFuncTbl) == ERROR)
	return (ERROR);

    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool,
					       CLUSTER_SIZE, FALSE)) == NULL)
	return (ERROR);


    /* carve up the shared-memory region */

    /*
     * N.B.
     * We are setting up the CFD ring as a ring of TxCBs, tied off with a
     * CFD_C_SUSP as frames are copied into the data buffers.  The
     * susp/resume model is used because the links to the next CFDs do
     * not change -- it is a fixed ring.  Also note that if a CFD is needed
     * for anything else (e.g., DIAG, NOP, DUMP, CONFIG, or IASETUP comands),
     * then the commands will use the current CFD in the ring.  After the
     * command is complete, it will be set back to a TxCB by fei82557Action().
     */

    /* First ready CFD pointer */
    pCFD = pDrvCtrl->pFreeCFD = (CFD_ID) pDrvCtrl->pCfdBase;

    /* initialize the CFD ring */

    for (ix = 0; ix < pDrvCtrl->nCFDs; ix++)
	{
	CFD_WORD_WR (pCFD, CFD_STAT_OFFSET,
		     (CFD_S_COMPLETE | CFD_S_OK));
    
	/* tie the current CFD to the next one */
	CFD_NEXT_WR (pCFD, ((UINT32) pCFD + CFD_SIZE));
	CFD_LONG_WR (pCFD, CFD_SW_NEXT_OFFSET, ((UINT32) pCFD + CFD_SIZE));
    
	if (!firstCFD)
	    {
	    /* Previous CFD pointer */
	    CFD_LONG_WR (pCFD, CFD_PREV_OFFSET,
			 ((UINT32) pCFD - CFD_SIZE));
	    }
	else
	    {
	    /* remember this CFD */
	    pDrvCtrl->pFreeCFD = pCFD;

	    /* tie the first CFD to the last one */
	    CFD_LONG_WR (pCFD, CFD_PREV_OFFSET,
			 ((UINT32) pCFD + (CFD_SIZE * (pDrvCtrl->nCFDs - 1))));

	    firstCFD = FALSE;
	    }

	/* no TBDs used */
	CFD_TBD_WR (pCFD, (UINT32) TBD_NOT_USED);
	CFD_BYTE_WR (pCFD, CFD_NUM_OFFSET, 0);
        CFD_LONG_WR (pCFD, CFD_MBLK_OFFSET, 0);

	/* set the thresh value */
	CFD_BYTE_WR (pCFD, CFD_THRESH_OFFSET, 
		     pDrvCtrl->board.tcbTxThresh);

	/* bump to the next CFD */
	pCFD = (CFD_ID) ((UINT32) pCFD + CFD_SIZE);
	}

    pCFD = (CFD_ID) ((UINT32) pCFD - CFD_SIZE);

    /* tie the last CFD to the first one */
    CFD_NEXT_WR (pCFD, ((UINT32) pDrvCtrl->pFreeCFD));
    CFD_LONG_WR (pCFD, CFD_SW_NEXT_OFFSET, ((UINT32) pDrvCtrl->pFreeCFD));

    /* set the used CFDs ring to the free one */
    pDrvCtrl->pUsedCFD = pDrvCtrl->pFreeCFD;


    /* Initialize the RFDs and RBDs */

    for (ix = 0; ix < pDrvCtrl->nRFDs; ix++)
        {
        pRFD = (RFD_ID)((int)pDrvCtrl->pRfdBase + (ix * RFD_DESC_SIZE));
        pRBD = (RBD_ID)((int)pDrvCtrl->pRbdBase + (ix * RBD_SIZE));

        pRbdTag = (RBD_TAG *)pRBD;
        pRbdTag->pRBD = pRBD;                     
        pRbdTag->pRFD = pRFD;
        pRbdTag->index = ix;

       if ((pRbdTag->pMblk = netTupleGet(pDrvCtrl->endObj.pNetPool,CLUSTER_SIZE,
                                         M_DONTWAIT, MT_DATA,0)) == NULL)
            {
            return (ERROR);
            }

        pRbdTag->pMblk->mBlkHdr.mData = 
                                 (char *)((int)pRbdTag->pMblk->mBlkHdr.mData +
                                          pDrvCtrl->offset);
DRV_LOG (DRV_DEBUG_LOAD, ("pRbdTag->pMblk->mBlkHdr.mData = %x  pDrvCtrl->offset = %x\n"),
				pRbdTag->pMblk->mBlkHdr.mData, pDrvCtrl->offset, 0, 0, 0, 0); /*wangfq*/

        RBD_BUF_WR (pRBD, (UINT32)pRbdTag->pMblk->mBlkHdr.mData);

        pRfdTag = (RFD_TAG *)pRFD;
        pRfdTag->index = ix;
        pRfdTag->nextIndex = (ix + 1) % pDrvCtrl->nRFDs;
        pRfdTag->status = 0;
        pRfdTag->pRFD = pRFD;

        if (ix == 0)
            firstOrLast = FIRST_RFD;
        else if (ix == pDrvCtrl->nRFDs - 1)
            firstOrLast = LAST_RFD;
        else
            firstOrLast = MID_RFD;

        switch (firstOrLast)
            {
            case FIRST_RFD: /* first */

                /* Set first RFD's RBD address pointer to first RBD */
                RFD_RBD_WR (pRFD, (UINT32)pDrvCtrl->pRbdBase);

                /* Set the RFD's next pointer to the next RFD */
                RFD_NEXT_WR (pRFD, ((int)pRFD + RFD_DESC_SIZE));

                pRbdTag->next = pRbdTag->index + 1; 

                /* Set the RBD's next pointer to the next RBD */
                RBD_NEXT_WR (pRBD, ((int)pRBD + RBD_SIZE));
                RBD_WORD_WR (pRBD, RBD_CFG_OFFSET, CLUSTER_SIZE);

                break;

            case LAST_RFD:  /* last */

                /* Set the RFD's RBD pointer to 0xffffffff */        
                RFD_RBD_WR (pRFD, RBD_NULL_ADDR);

                /* Set last RFD's next pointer to the first RFD */ 
                RFD_NEXT_WR (pRFD, (UINT32)pDrvCtrl->pRfdBase);

                pRbdTag->next = 0;  

                /* Set last RBD's next pointer to the first RBD */
                RBD_NEXT_WR (pRBD, (UINT32)pDrvCtrl->pRbdBase);
                RBD_WORD_WR (pRBD, RBD_CFG_OFFSET, (CLUSTER_SIZE | RBD_C_EL));
   
                pDrvCtrl->eLRbdTag = pRbdTag;

                break; 

            default :            

                /* Set the RFD's RBD pointer to 0xffffffff */        
                RFD_RBD_WR (pRFD, RBD_NULL_ADDR);

                /* Set the RFD's next pointer to the next RFD */
                RFD_NEXT_WR (pRFD, ((int)pRFD + RFD_DESC_SIZE));

                pRbdTag->next = pRbdTag->index + 1; 

                /* Set the RBD's next pointer to the next RBD */
                RBD_NEXT_WR (pRBD, ((int)pRBD + RBD_SIZE));
                RBD_WORD_WR (pRBD, RBD_CFG_OFFSET, CLUSTER_SIZE);
            }

        /* clear the status field */
	RFD_WORD_WR (pRFD, RFD_STAT_OFFSET, (UINT16) 0);

	/* Set the actual count field to zero */
        RFD_WORD_WR (pRFD, RFD_COUNT_OFFSET, 0);

	/*
         * Set the size field to zero so data is not written right 
         * after the RFD but to into the cluster pointed to by the RBD
         */

        RFD_WORD_WR (pRFD, RFD_SIZE_OFFSET, 0);

        /* Set the Flexible mode */
        RFD_BYTE_WR (pRFD, RFD_COMM_OFFSET, RFD_C_FLEX);  
        }

    /* Flush the write pipe */
    CACHE_PIPE_FLUSH ();

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557InitMem... Done\n"),
				0, 0, 0, 0, 0, 0);

    return OK;
    }

/**************************************************************************
*
* fei82557Start - start the device
*
* This routine starts the 82557 device and brings it up to an operational
* state.  The driver must have already been attached with the fei82557Load()
* routine.
*
* RETURNS: OK, or ERROR if the device could not be initialized.
*/

LOCAL STATUS fei82557Start
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    RFD_ID       pRFD;
    int		 retVal;

    DRV_LOG (DRV_DEBUG_START, ("Starting end...\n"), 1, 2, 3, 4, 5, 6);

    /* must have been attached */
    if (!pDrvCtrl->attached)
	return (ERROR);

    /* reset the chip */

    if (fei82557Reset (pDrvCtrl) == ERROR)
	return (ERROR);

    /* connect the int handler */
printf("VECTOR=%d!!!!!!!!!\n",(pDrvCtrl)->board.vector);
    SYS_INT_CONNECT (pDrvCtrl, fei82557Int, (int) pDrvCtrl, &retVal);
    if (retVal == ERROR)
	return (ERROR);

    /* acknowledge interrupts */

    SYS_INT_ACK (pDrvCtrl);

    /* enable chip interrupts after fei82557Reset disabled them */

    I82557_INT_ENABLE(SCB_C_M); 
 
    /* enable system interrupts after fei82557Reset disabled them */

    SYS_INT_ENABLE (pDrvCtrl);

    /* run diagnostics */

    if (fei82557Diag (pDrvCtrl) == ERROR)    
	return (ERROR);

    /* setup address */

    if (fei82557IASetup (pDrvCtrl) == ERROR)
	return (ERROR);

    if (END_FLAGS_ISSET (IFF_MULTICAST))
        FEI_FLAG_SET (FEI_MCAST);
 
    /* configure chip */

    if (fei82557Config (pDrvCtrl) == ERROR)
	return (ERROR);

    /* set some flags to default values */
    pDrvCtrl->rxHandle = FALSE;
    pDrvCtrl->txHandle = FALSE;
    pDrvCtrl->txStall = FALSE;

    /* Flush the write pipe */
    CACHE_PIPE_FLUSH ();

    /* put CU into suspended state */

    if (fei82557NOP (pDrvCtrl) == ERROR)
	return (ERROR);

    /* mark the interface as up */
    END_FLAGS_SET (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    /* startup the receiver */
    pRFD = (RFD_ID)pDrvCtrl->pRfdBase; 

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_RUSTART, TRUE, pRFD)
	== ERROR)
	return (ERROR);

    DRV_LOG (DRV_DEBUG_START, ("Starting end... Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);

    }

/*******************************************************************************
* fei82557Encap - encapsulate an Ethernet frame in a TX descriptor
*
* This routine takes an mBlk chain containing a packet to be transmitted
* and associates its buffers with a TX DMA descriptor.
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557Encap
    (
    DRV_CTRL *  pDrvCtrl,	/* pointer to DRV_CTRL structure */
    CFD_ID 	pCFD,
    M_BLK * 	pMblkHead	/* pointer to the mBlk/cluster chain */
    )
    {
    int			len = 0;
    int			frag = 0;
    M_BLK_ID		pMblk;
    M_BLK_ID		pMblkTemp;
    UINT16		status;
    char		*pTBD;
    char *		pEnetHdr;

    /*
     * Check to see if this descriptor has a cached buffer pointer
     * tacked onto it which hasn't been released yet. This can happen
     * if we queue many frames to transmit, and then try to queue more
     * before fei82557CFDFree() gets a chance to run. In this case,
     * we should release the buffers ourselves, provided that the
     * descriptor has been marked by the chip as having been processed
     * successfully.
     */

    CFD_POINT_RD (pCFD, CFD_MBLK_OFFSET, pMblk, M_BLK_ID);

    if (pMblk != NULL)
        {
	CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
    	if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
            return (ENOBUFS);

        netMblkClChainFree (pMblk); 
        }

    /*
     * start packing the M_BLKs in this chain into the fragment pointers.
     * stop when we run out of fragments or hit the end of the M_BLK chain.
     */
    pMblk = pMblkHead;

    for (pMblk = pMblkHead, frag = 0; pMblk != NULL; pMblk = pMblk->m_next)
        {
        if (pMblk->mBlkHdr.mLen != 0)
            {
            if (frag == TBD_MAX_FRAGS)
                break;

            len += pMblk->mBlkHdr.mLen;

            CFD_LONG_WR (pCFD, CFD_TBD_OFFSET +
                         (frag * TBD_SIZE) + TBD_ADDR_OFFSET,
                         FEI_VIRT_TO_SYS ((UINT32)pMblk->mBlkHdr.mData));

            FEI_CACHE_FLUSH (pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen); 
            /*	{
            		int i;
					for ( i = 0; i < pMblk->mBlkHdr.mLen; i++)
					 	* (char *)K0_TO_K1(pMblk->mBlkHdr.mData + i) = pMblk->mBlkHdr.mData[i];
            	}*/ /*wangfq*/

            CFD_LONG_WR (pCFD, CFD_TBD_OFFSET +
                         (frag * TBD_SIZE) + TBD_LEN_OFFSET,
                         pMblk->mBlkHdr.mLen |
                         (pMblk->m_next == NULL ? TBD_LASTFRAG : 0));

            frag++;
            }
        }

    /*
     * Handle special case: we used up all our fragments,
     * but we have more M_BLKs left in the chain. Copy the
     * data into an M_BLK cluster. Note that we don't
     * bother clearing the values in the other fragment
     * pointers/counters; it wouldn't gain us anything,
     * and would waste cycles.
     */

    if (pMblk != NULL)
        {
        /* get a mBlk/clBlk/cluster tuple */
        if ((pMblkTemp = netTupleGet (pDrvCtrl->endObj.pNetPool,
                                      ETHERMTU + EH_SIZE,
                                      M_DONTWAIT, MT_DATA, FALSE)) == NULL)
            {
            DRV_LOG (DRV_DEBUG_TX, "Out of Tx M Blocks!\n", 1, 2, 3, 4, 5, 6);
            return (ENOBUFS);
            }

        len = netMblkToBufCopy (pMblkHead, pMblkTemp->mBlkHdr.mData, NULL);

        pMblkTemp->mBlkPktHdr.len = pMblkTemp->mBlkHdr.mLen = len;

        netMblkClChainFree (pMblkHead); /* free the given mBlk chain */

        pMblkHead = pMblkTemp;
        CFD_LONG_WR (pCFD, CFD_TBD_OFFSET + TBD_ADDR_OFFSET,
                     FEI_VIRT_TO_SYS ((UINT32)pMblkHead->mBlkHdr.mData));

        CFD_LONG_WR (pCFD, CFD_TBD_OFFSET + TBD_LEN_OFFSET,
                     pMblkHead->mBlkHdr.mLen |
                     (pMblkHead->m_next == NULL ? TBD_LASTFRAG : 0));
        }

    pEnetHdr = mtod (pMblkHead, char *);

#ifdef INCLUDE_RFC_2233

    /* RFC 2233 mib2 counter update for outgoing packet */

    if (pDrvCtrl->endObj.pMib2Tbl != NULL)
        {
        pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn(pDrvCtrl->endObj.pMib2Tbl,
                                                 M2_PACKET_OUT, pEnetHdr, len);
        }
#endif /* INCLUDE_RFC_2233 */

    /* set up the current CFD */
    CFD_LONG_WR (pCFD, CFD_STAT_OFFSET, (CFD_C_CID | CFD_C_XMIT |
                                         CFD_C_SUSP | TBD_CTL_SF) << 16);


    /* set the thresh value  &  Mark how many TBDs were used. */
    CFD_LONG_WR (pCFD, CFD_COUNT_OFFSET, (frag << 24) |
                 (pDrvCtrl->board.tcbTxThresh << 16));

    /* Write physical address of TBD array. */
    pTBD = (char *)pCFD;
    pTBD += CFD_TBD_OFFSET;
    CFD_TBD_WR (pCFD, (UINT32)pTBD);

    /*
     * Save a pointer to the mbuf for later.
     * XXX assumes 32-bit pointers.
     */
    CFD_LONG_WR (pCFD, CFD_MBLK_OFFSET, (UINT32)pMblkHead);

    /* logMsg("fei82557Encap done \n",0,0,0,0,0,0); */

    return(OK);
}

/*******************************************************************************
* fei82557TxRestart - place muxTxRestart on netJobRing
*
* RETURNS: N/A
*/

LOCAL void fei82557TxRestart
    (
    END_OBJ *  pEndObj	/* pointer to DRV_CTRL structure */
    )
    {
    /*  logMsg("fei82557TxRestart \n",0,0,0,0,0,0); */

        netJobAdd ((FUNCPTR) fei82557MuxTxRestart, (int) pEndObj, 0, 0, 0, 0);
        return;
    }

/*******************************************************************************
*
* fei82557MuxTxRestart - Calls muxTxRestart frep netJobRing
*
*
* RETURNS: N/A
*/

LOCAL void fei82557MuxTxRestart
    (
    END_OBJ *  pEndObj  /* pointer to DRV_CTRL structure */
    )
    {
    muxTxRestart(pEndObj);
    }

/*******************************************************************************
* fei82557Send - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557Send
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK *     pMblk           /* pointer to the mBlk/cluster pair */
    )

    {
    return(pDrvCtrl->pSendRtn (pDrvCtrl,pMblk));
    }


/*******************************************************************************
* fei82557GatherSend - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557GatherSend
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK * 	pMblk           /* pointer to the mBlk/cluster pair */
    )

    {
    volatile CFD_ID 	pCFD;
    volatile UINT32 	pPrevCFD;
    volatile UINT16	command;
    volatile UINT16	status;
    volatile UINT16	scbStatus;

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...\n"), 1, 2, 3, 4, 5, 6);

    /* interlock with fei82557CFDFree */

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    /* check for CFDs availability */

    /* get the current free CFD */

    FREE_CFD_GET (pCFD);

    /* read the CFD status word */
    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
 
    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
	{
	DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...NO CFDS \n"), 1, 2, 3, 4, 5, 6);

	/* set to stall condition */

	pDrvCtrl->txStall = TRUE;
       
        CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

        if ((scbStatus & SCB_S_CUMASK) == SCB_S_CUSUSP)
	    {
	    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUSTART, TRUE, pCFD) 
            	== ERROR)
		{
		DRV_LOG (DRV_DEBUG_TX,("CU Start failed \n"), 0, 0, 0, 0, 0, 0);
		} 
	    }
        wdStart(pDrvCtrl->txRetryWDId, 1,
                (FUNCPTR) fei82557TxRestart, (int) (&pDrvCtrl->endObj));

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	return (END_ERR_BLOCK);
	}

    if (fei82557Encap (pDrvCtrl, pCFD, pMblk) != OK)
        {
        wdStart(pDrvCtrl->txRetryWDId, 1,
                (FUNCPTR) fei82557TxRestart, (int) (&pDrvCtrl->endObj));

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	return (END_ERR_BLOCK);
        }

    /* this is a transmit command */

    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_TX);

    /* tie to the previous CFD */

    CFD_LONG_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD);
    CFD_WORD_RD ((CFD_ID)pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR ((CFD_ID)pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

#ifndef INCLUDE_RFC_2233

    /* Old RFC 1213 mib2 interface */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

#endif /* INCLUDE_RFC_2233 */

    /* kick the CU if needed */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE,(UINT32) 0) 
        == ERROR)
        {
        DRV_LOG (DRV_DEBUG_TX, ("Could not send packet\n"), 
			          1, 2, 3, 4, 5, 6);

        END_TX_SEM_GIVE (&pDrvCtrl->endObj);

        return (ERROR);
        }

    /* interlock with fei82557CFDFree */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);
    }

/*******************************************************************************
* fei82557CopySend - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557CopySend
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK *     pMblk           /* pointer to the mBlk/cluster pair */
    )

    {
    volatile CFD_ID     pCFD;
    volatile UINT32     pPrevCFD;
    int                 len = 0;
    volatile UINT16     command;
    volatile UINT16     status;
    char *              pEnetHdr;
    volatile UINT16     scbStatus;

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...\n"), 1, 2, 3, 4, 5, 6);

    /* interlock with fei82557CFDFree */

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    /* check for CFDs availability */

    /* get the current free CFD */

    FREE_CFD_GET (pCFD);

    /* Make cache consistent with memory */

    FEI_CACHE_INVALIDATE (pCFD, 4);

    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
        {

        DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...NO CFDS \n"), 1, 2, 3, 4, 5, 6);

        /* set to stall condition */


        pDrvCtrl->txStall = TRUE;

        CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

        if ((scbStatus & SCB_S_CUMASK) == SCB_S_CUSUSP)
            {
            if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUSTART, TRUE, pCFD)
                == ERROR)
                {
                DRV_LOG (DRV_DEBUG_TX,("CU Start failed \n"), 0, 0, 0, 0, 0, 0);
                }
            }

        END_TX_SEM_GIVE (&pDrvCtrl->endObj);

        return (END_ERR_BLOCK);
        }

    pEnetHdr = (char *) (CFD_PKT_ADDR (pCFD));
    len = netMblkToBufCopy (pMblk, (char *) pEnetHdr, NULL);
    netMblkClChainFree (pMblk);

    len = max (ETHERSMALL, len);

#ifndef INCLUDE_RFC_1213

    /* New RFC 2233 mib2 interface */

    /* RFC 2233 mib2 counter update for outgoing packet */

    if (pDrvCtrl->endObj.pMib2Tbl != NULL)
        {
        pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn(pDrvCtrl->endObj.pMib2Tbl,
                                                 M2_PACKET_OUT, pEnetHdr, len);
        }
#endif /* INCLUDE_RFC_1213 */

    /* set up the current CFD */

    CFD_WORD_WR (pCFD, CFD_COUNT_OFFSET, ((len & 0x3fff) | TCB_CNT_EOF));
    CFD_WORD_WR ((CFD_ID)pCFD, CFD_COMM_OFFSET, (CFD_C_XMIT | CFD_C_SUSP));
    CFD_WORD_WR ((CFD_ID)pCFD, CFD_STAT_OFFSET, 0);

    /* set the thresh value */

    CFD_BYTE_WR (pCFD, CFD_THRESH_OFFSET,
                 pDrvCtrl->board.tcbTxThresh);

    /* no TBDs used */

    CFD_BYTE_WR (pCFD, CFD_NUM_OFFSET, 0);

    /* this is a transmit command */

    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_TX);

    /* tie to the previous CFD */

    CFD_LONG_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD);
    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

#ifdef INCLUDE_RFC_1213

    /* Old RFC 1213 mib2 interface */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

#endif /* INCLUDE_RFC_1213 */

    /* kick the CU if needed */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE,(UINT32) 0)
        == ERROR)
        {
        DRV_LOG (DRV_DEBUG_TX, ("Could not send packet\n"),
                                  1, 2, 3, 4, 5, 6);

        END_TX_SEM_GIVE (&pDrvCtrl->endObj);

        return (ERROR);
        }

    /* interlock with fei82557CFDFree */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);

    }

/*******************************************************************************
*
* fei82557Int - entry point for handling interrupts from the 82557
*
* The interrupting events are acknowledged to the device, so that the device
* will de-assert its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask.
*
* RETURNS: N/A
*/

LOCAL void fei82557Int
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {

    UINT8 mask;

    /* read and save CSR status word */
    CSR_WORD_RD (CSR_STAT_OFFSET, pDrvCtrl->event);
    CSR_BYTE_RD (CSR_INT_OFFSET, mask);

    if ((pDrvCtrl->event & SCB_S_STATMASK) == 0)
	{
	/* This device was not the cause of the shared int */

	return;  
	}

    /* clear chip level interrupt pending, use byte access */
    CSR_BYTE_WR (CSR_ACK_OFFSET, ((pDrvCtrl->event & SCB_S_STATMASK) >> 8));

    /* board level interrupt acknowledge */
    SYS_INT_ACK (pDrvCtrl);

    /* handle transmit interrupts */

    if ((pDrvCtrl->event & (SCB_S_CNA|SCB_S_CX)) && 
        (pDrvCtrl->txHandle == FALSE))
	{
	pDrvCtrl->txHandle = TRUE;

	netJobAdd ((FUNCPTR) fei82557CFDFree, (int) pDrvCtrl, 0, 0, 0, 0);
	}

    /* handle receive interrupts */

    if (pDrvCtrl->event & (SCB_S_FR | SCB_S_RNR) && !(mask & SCB_C_M))
        {

        /* Disable receive interrupts in the device for now */


        I82557_INT_DISABLE(SCB_C_M);
		/*SYS_INT_DISABLE (pDrvCtrl);*//*wangfq*/

        /* Test if fei82557RecvHandler() is on netJobRing? */

        if(!pDrvCtrl->rxJobQued)
            {
            /* fei82557RecvHandler() is not on netJobRing so put it on. */

            if ((netJobAdd ((FUNCPTR) fei82557RecvHandler, (int) pDrvCtrl,
                       0, 0, 0, 0)) != ERROR)
                {
                pDrvCtrl->rxJobQued = TRUE;
                }
            else
                {
                logMsg("The netJobRing is full. 1\n",0,0,0,0,0,0);

                I82557_INT_ENABLE(SCB_C_M);
                return;
                }
            }
        }
    }

/******************************************************************************
*
* fei82557RecvHandler - service task-level interrupts for receive frames
*
* This routine is run in netTask's context.  The ISR scheduled
* this routine so that it could handle receive packets at task level.
*
* RETURNS: N/A
*/

LOCAL void fei82557RecvHandler 
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    M_BLK_ID            pNewMblk    = NULL;
    RBD_TAG *		pRbdTag = NULL;
    int                 loopCounter = 0;
    volatile UINT16     rbdStatus;
    volatile UINT16     scbStatus;
 
    DRV_LOG (DRV_DEBUG_RX, "fei82557RecvHandler\n", 0, 0, 0, 0, 0, 0);

    pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];
    RBD_WORD_RD(pRbdTag->pRBD, RBD_STAT_OFFSET, rbdStatus);

	DRV_LOG (DRV_DEBUG_RX, "pRbdTag->pRBD = %x  &rbdStatus = %x rbdStatus = %d\n", pRbdTag->pRBD, &rbdStatus, rbdStatus, 0, 0, 0);

	while (rbdStatus != RBD_STATUS_FREE)  
        {
        if ((pNewMblk = netTupleGet(pDrvCtrl->endObj.pNetPool,
                                    CLUSTER_SIZE, M_DONTWAIT, 
                                    MT_DATA,0)) != NULL)
            {
            pNewMblk->mBlkHdr.mData = (char *)((int)pNewMblk->mBlkHdr.mData + 
                                                pDrvCtrl->offset);
            }
        else
            {
            RFD_WORD_WR (pRbdTag->pRFD, RFD_STAT_OFFSET, (UINT16) 0);
            RBD_WORD_WR (pRbdTag->pRBD, RBD_STAT_OFFSET,0);
            pDrvCtrl->rbdIndex = pRbdTag->next;

            fei82557NoResource(pDrvCtrl);
    
            break;
            }

        /* 
         * get the actual received bytes number 
         *
         * Note that the size of clusters in this driver will always
         * be sufficient to hold a complete frame. Therefore, the 
         * actual byte count in the RBD will be the same as in the RFD.
         * and the byte count can be taken from the RBD instead of the
         * RFD. 
         */ 

        pRbdTag->pMblk->mBlkHdr.mLen         = rbdStatus & ~0xc000; 

        DRV_LOG (DRV_DEBUG_RX, "pRBD + RBD_COUNT_OFFSET= %x\n", 
                rbdStatus & ~0xc000, 0,0,0,0,0);

        pRbdTag->pMblk->mBlkHdr.mFlags       |= M_PKTHDR;
        pRbdTag->pMblk->mBlkPktHdr.len       = pRbdTag->pMblk->mBlkHdr.mLen;
		DRV_LOG (DRV_DEBUG_RX, "0\n", 0,0,0,0,0,0); /*wangfq*/
        /* Invalidate cluster so DMA data initially read from memory */
        FEI_CACHE_INVALIDATE(pRbdTag->pMblk->pClBlk->clNode.pClBuf, 
                             pRbdTag->pMblk->mBlkHdr.mLen); /*wangfq*/
#if 0
			{
				int i;
				for (i = 0; i < pRbdTag->pMblk->mBlkHdr.mLen; i++)
					pRbdTag->pMblk->pClBlk->clNode.pClBuf[i] = * (char *)(K0_TO_K1(pRbdTag->pMblk->pClBlk->clNode.pClBuf + i));
        	}
#endif
#if 0                             
 		DRV_LOG (DRV_DEBUG_RX, "1,pRbdTag->pMblk->pClBlk->clNode.pClBuf = %x\n", (ULONG)(pRbdTag->pMblk->pClBlk->clNode.pClBuf),0,0,0,0,0); /*wangfq*/
		DRV_LOG (DRV_DEBUG_RX, "1,pRbdTag->pMblk->mBlkHdr.mLen = %d\n", (ULONG)(pRbdTag->pMblk->mBlkHdr.mLen),0,0,0,0,0); /*wangfq*/
            {
				int i;
		for ( i = 0; i < (ULONG)(pRbdTag->pMblk->mBlkHdr.mLen); i++ )
			{
			DRV_LOG (DRV_DEBUG_RX, "pClBuf[%d] = %02x\n",i, (unsigned char)(pRbdTag->pMblk->pClBlk->clNode.pClBuf[i]),0,0,0,0); /*wangfq*/
			}
			}
#endif
#if 0
fei82557DumpPrint(0);/*wangfq*/
fei82557ShowRxRing(0);/*wangfq*/
#endif
#if 0
netStackSysPoolShow();/*wangfq*/
netStackDataPoolShow();/*wangfq*/
arpShow();/*wangfq*/
routestatShow();/*wangfq*/
#endif
#if 0
inetstatShow();/*wangfq*/
netPoolShow(&pDrvCtrl->endObj.pNetPool);/*wangfq*/
mbufShow();/*wangfq*/

ipstatShow(0);/*wangfq*/

netPoolShow(&pDrvCtrl->endObj.pNetPool);/*wangfq*/
#endif
/*tcpstatShow();*/ /*wangfq*/
/*netStackDataPoolShow();*//*wangfq*/
		/* Send the received frame to the stack */
		#if 1
        END_RCV_RTN_CALL (&pDrvCtrl->endObj, pRbdTag->pMblk);
		#endif
		DRV_LOG (DRV_DEBUG_RX, "2\n", 0,0,0,0,0,0); /*wangfq*/

#ifdef INCLUDE_RFC_2233

        /* RFC 2233 mib2 counter update for incoming packet */

        if (pDrvCtrl->endObj.pMib2Tbl != NULL)
            {
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn(
                                             pDrvCtrl->endObj.pMib2Tbl,
                                             M2_PACKET_IN,
                                             pRbdTag->pMblk->mBlkHdr.mData,
                                             pRbdTag->pMblk->mBlkHdr.mLen);
            }
#else
        /* Old RFC 1213 mib2 interface */
        END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);
		DRV_LOG (DRV_DEBUG_RX, "3\n", 0,0,0,0,0,0); /*wangfq*/
#endif /* INCLUDE_RFC_2233 */

        /* Replace pRxCluster with pNewCluster */
        RBD_BUF_WR(pRbdTag->pRBD, (UINT32)pNewMblk->mBlkHdr.mData);
DRV_LOG (DRV_DEBUG_RX, "4\n", 0,0,0,0,0,0); /*wangfq*/
        /* Replace mBlk */
        pRbdTag->pMblk = pNewMblk;
DRV_LOG (DRV_DEBUG_RX, "5\n", 0,0,0,0,0,0); /*wangfq*/
        /* Clear the RBD's status */
        RFD_WORD_WR (pRbdTag->pRFD, RFD_STAT_OFFSET, (UINT16) 0);
        RBD_WORD_WR (pRbdTag->pRBD, RBD_STAT_OFFSET,0);
DRV_LOG (DRV_DEBUG_RX, "6\n", 0,0,0,0,0,0); /*wangfq*/
        /* Move the End of List RBD */
        RBD_WORD_WR (pRbdTag->pRBD, RBD_CFG_OFFSET,
                     (CLUSTER_SIZE | RBD_C_EL));
DRV_LOG (DRV_DEBUG_RX, "7\n", 0,0,0,0,0,0); /*wangfq*/
        RBD_WORD_WR(pDrvCtrl->eLRbdTag->pRBD,
                        RBD_CFG_OFFSET,CLUSTER_SIZE);
DRV_LOG (DRV_DEBUG_RX, "8\n", 0,0,0,0,0,0); /*wangfq*/
        pDrvCtrl->eLRbdTag = pRbdTag;

        /* Move to next RBD */
        pDrvCtrl->rbdIndex = pRbdTag->next;
        pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];
        RBD_WORD_RD(pRbdTag->pRBD, RBD_STAT_OFFSET, rbdStatus);

DRV_LOG (DRV_DEBUG_RX, "9\n", 0,0,0,0,0,0); /*wangfq*/
        /*
         * The loopCounter limits the time this loop runs to
         * pDrvCtrl->maxRxFrames so as not to starve the stack.
         */

        if (loopCounter < pDrvCtrl->maxRxFrames)
            {
            /* below maximum frames so keep going */
            loopCounter++;
			DRV_LOG (DRV_DEBUG_RX, "a\n", 0,0,0,0,0,0); /*wangfq*/
            }
        else
            {
            DRV_LOG (DRV_DEBUG_RX, "b\n", 0,0,0,0,0,0); /*wangfq*/
            break;
            }
        } /* end of while loop */
DRV_LOG (DRV_DEBUG_RX, "c\n", 0,0,0,0,0,0); /*wangfq*/
    DRV_LOG (DRV_DEBUG_RX, ("fei82557RecvHandler... Done, \n"),
                             0, 0, 0, 0, 0, 0);
   
    /* let us leave the netJobRing */

   if (rbdStatus != RBD_STATUS_FREE) 
        {
        /* Put this job back on the netJobRing and leave */ 

        if ((netJobAdd ((FUNCPTR) fei82557RecvHandler, (int) pDrvCtrl,
                        0,0,0,0)) == ERROR)
            {
            /* Very bad!! The stack is now probably corrupt. */

            logMsg("The netJobRing is full. 2\n",0,0,0,0,0,0);

            I82557_INT_ENABLE(SCB_C_M);
            return;
            }
        }
    else
        {
        DRV_LOG (DRV_DEBUG_RX, "d\n", 0,0,0,0,0,0); /*wangfq*/
        pDrvCtrl->rxJobQued = FALSE;
        }

    /* kick receiver if needed */
    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus); 

    if ((scbStatus & SCB_S_RUMASK) != SCB_S_RURDY)
        {
        DRV_LOG (DRV_DEBUG_RX, "e\n", 0,0,0,0,0,0); /*wangfq*/
        fei82557Restart(pDrvCtrl);
        }
	DRV_LOG (DRV_DEBUG_RX, "f\n", 0,0,0,0,0,0); /*wangfq*/
	/*SYS_INT_ENABLE (pDrvCtrl);*/ /*wangfq*/
	I82557_INT_ENABLE(SCB_C_M);
    } 

/*******************************************************************************
*
* fei82557Restart - Restart the device after a receive stall
*
* This routine sets the necessary default values and then restarts the 
* Receive Unit (RU) at the next RFD beyond the End of List.
*
* RETURNS: OK or ERROR 
*/

LOCAL STATUS fei82557Restart
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    RBD_TAG *    pRbdTag;

    DRV_LOG (DRV_DEBUG_START, ("Restarting end...\n"), 1, 2, 3, 4, 5, 6);

    /* set some flags to default values */
    pDrvCtrl->rxHandle = FALSE;
    pDrvCtrl->txHandle = FALSE;
    pDrvCtrl->txStall =  FALSE;
    pDrvCtrl->rbdIndex = pDrvCtrl->eLRbdTag->next;

    if (pDrvCtrl->rbdIndex != pDrvCtrl->eLRbdTag->index)
        {
        /* clear the RBD address pointer in the eLRbd */
        pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->eLRbdTag->index];
        RFD_RBD_WR (pRbdTag->pRFD, 0xffffffff);

        pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];

        /* Set the start RFD's RBD address pointer to its RBD */
        RFD_RBD_WR (pRbdTag->pRFD, (UINT32)pRbdTag->pRBD);
        }
    else
        {
        pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];
        }

    pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];

    /* Flush the write pipe */
    CACHE_PIPE_FLUSH ();

    /* startup the receiver */

    if (fei82557SCBCommand(pDrvCtrl,SCB_C_RUSTART,TRUE,pRbdTag->pRFD)
        == ERROR)
        return (ERROR);

    DRV_LOG (DRV_DEBUG_START, ("Restarting end... Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);

    }

/**************************************************************************
*
* fei82557NoResource - handles a no resource condition.
*
* This routine updates the MIB counters for a no 
* resource condition.
*
* RETURNS: N/A
*/

LOCAL void fei82557NoResource
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
#ifdef INCLUDE_RFC_2233
    /* Increment the error counter */

    if (pDrvCtrl->endObj.pMib2Tbl != NULL)
        {
        pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn(pDrvCtrl->endObj.pMib2Tbl,
                                                      M2_ctrId_ifInErrors, 1);
        }
    if (pDrvCtrl->endObj.pMib2Tbl != NULL)
        {
        pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn(pDrvCtrl->endObj.pMib2Tbl,
                                                      M2_ctrId_ifInDiscards, 1);
        }
#else

    /* Old RFC 1213 mib2 interface */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);

#endif
    }

/**************************************************************************
*
* fei82557CFDFree - free all used command frames
*
* This routine frees all used command frames and notifies upper protocols
* that new resources are available.
*
* RETURNS: N/A
*
*/

LOCAL void     fei82557CFDFree
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    volatile CFD_ID    	pUsedCFD = NULL;
    volatile UINT16	status;
    volatile UINT16	scbStatus;
    UINT8		action;
    M_BLK_ID		pMblk;

    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	{
	/* interlock with fei82557Send */

	END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

	/* read CSR status word */

	CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

	/* if CU is active, do not go on */

	if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUSUSP)
	    {
	    pDrvCtrl->txHandle = FALSE;

	    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	    return;
	    }
	}

    /* process all the used CFDs until we find either:
     * a. a non-complete frame;
     * b. a frame with the SUSPEND bit set
     */

    FOREVER
        {
        USED_CFD_GET (pUsedCFD);

        /* read the CFD status word */

        CFD_WORD_RD (pUsedCFD, CFD_STAT_OFFSET, status);
 
        /* if it's not ready, don't touch it! */

        if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
            {
            break;
            }

	/* put CFD back to a TxCB - mirrors fei82557InitMem() */

	/* no TBDs used */

	CFD_TBD_WR (pUsedCFD, (UINT32) TBD_NOT_USED);
	CFD_BYTE_WR (pUsedCFD, CFD_NUM_OFFSET, 0);

	/* set the thresh value */

	CFD_BYTE_WR (pUsedCFD, CFD_THRESH_OFFSET, 
		     pDrvCtrl->board.tcbTxThresh);

        /* correct statistic only in case of non-action command */

	CFD_BYTE_RD (pUsedCFD, CFD_ACTION_OFFSET, action);

        if ((action == CFD_TX) &&
            !(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
            {
#ifdef DRV_DEBUG557
	    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
		{
		DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Errored Frame \n"), 
					 0, 0, 0, 0, 0, 0);
		}
#endif


#ifdef INCLUDE_RFC_2233

            if (pDrvCtrl->endObj.pMib2Tbl != NULL)
                {
                pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn(pDrvCtrl->endObj.
                                                                    pMib2Tbl,
                                                          M2_ctrId_ifOutErrors,
                                                          1);
                }
#else 

            /* Old RFC 1213 mib2 interface */

            END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1);
            END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, -1);

#endif /* INCLUDE_RFC_2233 */

            /* make the errored CFD available */

	    CFD_WORD_WR (pUsedCFD, CFD_STAT_OFFSET, 
			 (CFD_S_OK | CFD_S_COMPLETE));
            }

	/* If this was a transmission, free the attached mBlk. */

        if (action == CFD_TX && pDrvCtrl->pSendRtn != fei82557CopySend)
            {
            CFD_POINT_RD (pUsedCFD, CFD_MBLK_OFFSET, pMblk, M_BLK_ID);
            CFD_LONG_WR (pUsedCFD, CFD_MBLK_OFFSET, 0);

            if (pMblk != NULL)
                {
                netMblkClChainFree (pMblk);
                }
            }

	if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	    {

	    /* soon notify upper protocols CFDs are available */

	    if (pDrvCtrl->txStall)
	        {
	        DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Restart mux \n"), 
	           	                 0, 0, 0, 0, 0, 0);

	        netJobAdd ((FUNCPTR) fei82557MuxTxRestart, (int) 
                           &pDrvCtrl->endObj,0,0,0,0);

	        pDrvCtrl->txStall = FALSE;
	        }
	    }

        /* we have finished our work if this is a suspend CFD */

        CFD_WORD_RD (pUsedCFD, CFD_COMM_OFFSET, status);

        /* advance pointer to the used CFDs (even if we are exiting) */

	fei82557FDUpdate (pDrvCtrl, CFD_USED);

        if (FD_FLAG_ISSET (status, CFD_C_SUSP))
            {
            break;
            }
        }

    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	{
        pDrvCtrl->txHandle = FALSE;

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Done \n"),
				 0, 0, 0, 0, 0, 0);
	}

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();
    }


/**************************************************************************
*
* fei82557FDUpdate - update the current frame descriptor
*
* This routine updates the pointer to the appropriate frame descriptor 
* ring with the value stored in the link field of the current frame 
* descriptor. Frame descriptor here can be either an RFD or a CFD in the
* free CFDs ring as well as in the used CFDs ring.
*
* RETURNS: N/A
*
*/

LOCAL void     fei82557FDUpdate
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT8	fdList		/* FD list selector */
    )

    {									
    volatile UINT32	fdAddr;						
    volatile FD_ID	pFD;						

    /* logMsg("fei82557FDUpdate\n",0,0,0,0,0,0); */


    switch (fdList)							
        {								
									
        case CFD_FREE:							
									
            /* CFDs ready to be processed */				
									
	    FREE_CFD_GET (pFD);					
									
            /* get the next free CFD address */				
									
	    CFD_NEXT_RD (pFD, fdAddr);					
									
            /* bump to the next free CFD */			
									
            pDrvCtrl->pFreeCFD = (FD_ID) fdAddr;			
									
            break;							
									
        case CFD_USED:							
									
            /* CFDs already processed */				
									
	    USED_CFD_GET (pFD);					
									
            /* get the next free CFD address */				
									
	    CFD_NEXT_RD (pFD, fdAddr);					
									
            /* bump to the next used CFD */				
									
            pDrvCtrl->pUsedCFD = (FD_ID) fdAddr;			
									
            break;							

        default:							
            break;							
        }								
									
    /* logMsg("fei82557FDUpdate ... Done\n",0,0,0,0,0,0); */
    }

/**************************************************************************
*
* fei82557Stop - stop the fei82557 interface
*
* This routine marks the interface as inactive, disables interrupts and 
* resets the chip. It brings down the interface to a non-operational state. 
* To bring the interface back up, fei82557Start() must be called.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557Stop
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int	retVal;

    /* disable system interrupt */

    SYS_INT_DISABLE (pDrvCtrl);     

    /* disable chip interrupt   */

    I82557_INT_DISABLE(SCB_C_M);

    /* mark the interface as down */

    END_FLAGS_CLR (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    /* reset the chip */

    fei82557Reset (pDrvCtrl);

    /* disconnect the interrupt handler */

    SYS_INT_DISCONNECT (pDrvCtrl, fei82557Int, (int)pDrvCtrl, &retVal);
    if (retVal == ERROR)
	return (ERROR);

    /* wait for the reset... */

    taskDelay (max (2, sysClkRateGet()/30));	  

    return OK;
    } 

/**************************************************************************
*
* fei82557Reset - reset the `fei82557' interface
*
* This routine resets the chip by issuing a Port Selective Reset Command
* to the chip. The Receiver And the Transmit Unit are in idle state after
* this command is performed.
*
* RETURNS: OK or ERROR, if the command was not successful.
*/

LOCAL STATUS fei82557Reset
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int	ix;

    /* issue a selective reset to the 82557 chip */

    CSR_LONG_WR (CSR_PORT_OFFSET, FEI_PORT_SELRESET);

    /* wait for the receive handler to catch up to the [reset] */

    for (ix = (FEI_INIT_TMO * fei82557ClkRate); --ix;)
	{
	if (!pDrvCtrl->rxHandle)
	    break;

	taskDelay (max(1, sysClkRateGet()/60));
	}

    if (!ix)
	return (ERROR);

    return (OK);
    }

/**************************************************************************
*
* fei82557SCBCommand - deliver a command to the 82557 via the SCB
*
* This function causes the device to execute a command. An error status is
* returned if the command field does not return to zero, from a previous
* command, in a reasonable amount of time.
*
* RETURNS: OK, or ERROR if the command field appears to be frozen.
*/

LOCAL STATUS fei82557SCBCommand
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT8   	cmd,		/* command identifier */
    BOOL    	addrValid,	/* optionally fill the GP */
    UINT32 *	pAddr		/* Frame Descriptor address */
    )

    {
    volatile UINT16     scbCommand;
    volatile UINT16     scbStatus;
    int     ix;

    CACHE_PIPE_FLUSH();

    for (ix = 0x8000; --ix;)
	{
	/* read CSR command word */

	CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);

	if ((scbCommand & SCB_CR_MASK) == 0)
	    {
	    break;
	    }
	}

    if (!ix)
	{
        CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);
	DRV_LOG (DRV_DEBUG_START,
		 ("fei82557SCBCommand: command 0x%x failed, scb 0x%x 
                   status 0x%x\n"), cmd, scbCommand, scbStatus, 0, 0, 0);

	return (ERROR);
	}

    /* 
     * writing the GP and SCB Command should be protected from preemption 
     * we attain this by disabling task context switch before we write 
     * to the general pointer and re-enabling it after we write the
     * command word. We also flush the write pipe in-between. 
     */

    /* optionally fill the GP */

    if (addrValid)
	{
	if (taskLock() == ERROR)
	    {
	    DRV_LOG (DRV_DEBUG_ALL, "fei82557SCBCommand: task lock failed\n",
		     0, 0, 0, 0, 0, 0);
	    return (ERROR);
	    }

	/* Do not do CPU to PCI translation for the base address registers. */

	if ((cmd == SCB_C_CULDBASE) || (cmd == SCB_C_RULDBASE))
	    {
	    CSR_LONG_WR (CSR_GP_OFFSET, (UINT32) (pAddr));
	    }
	else
	    {
	    CSR_GP_WR ((UINT32) pAddr);
	    }
	}

    /* read CSR command byte */

    CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);

    /* write CSR command byte */

    CSR_BYTE_WR (CSR_COMM_OFFSET, (UINT8) (scbCommand | cmd));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    if (addrValid)
	{
	if (taskUnlock() == ERROR)
	    DRV_LOG (DRV_DEBUG_ALL, ("fei82557SCBCommand: task unlock failed\n"),
		    0, 0, 0, 0, 0, 0);
	}

    return (OK);

    } 

/*******************************************************************************
* fei82557Action - execute the specified action command
*
* This routine executes the specified action
*
* We do the command contained in the CFD synchronously, so that we know
* it's complete upon return.
*
* RETURNS: The status return of the action command, or 0 on failure.
*/

LOCAL UINT16 fei82557Action
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT16	action		/* action command identifier */
    )
    {
    volatile CFD_ID 	pCFD;
    volatile CFD_ID 	pPrevCFD;
    int			ix;
    volatile UINT16	command = 0;
    volatile UINT16	status;
    volatile UINT16     scbStatus;

    /* interlock with fei82557Send */

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    /* check for CFDs availability */
 
    FREE_CFD_GET (pCFD);
 
    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
printf("Here is the pCFD value: #####%.8x#####\n", pCFD); 
    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
        {
	/* interlock with fei82557CFDFree */

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

        return (0);
        }

    /* set up the current CFD */

    CFD_WORD_WR (pCFD, CFD_COMM_OFFSET, (action | CFD_C_SUSP | CFD_C_INT));
    CFD_WORD_WR (pCFD, CFD_STAT_OFFSET, 0);
    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_ACTION);

    /* for some actions we need to do additional work */

    switch (action)
	{

	case CFD_C_MASETUP:

	    /* form the multicast address list */

	    fei82557MCastListForm (pDrvCtrl, pCFD);

	    break;

	case CFD_C_CONFIG:

	    /* fill the CFD with config bytes */

	    fei82557ConfigForm (pDrvCtrl, pCFD);

	    break;

	case CFD_C_IASETUP:

	    /* fill the CFD with our Ethernet address */

	    bcopy ((char *) FEI_HADDR (&pDrvCtrl->endObj),
		   (char *) CFD_IA_ADDR (pCFD),
		   FEI_HADDR_LEN (&pDrvCtrl->endObj));

	    break;

	default:
	    break;

	}

    /* tie to the previous CFD */

    CFD_POINT_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD, CFD_ID);
    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

#ifndef INCLUDE_RFC_2233

    /* Old RFC 1213 mib2 interface */

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

#endif /* INCLUDE_RFC_2233 */

    /* check CU operation -- kick if needed */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUACTIVE)
        {
        if (scbStatus == SCB_S_CUIDLE)
            {
            if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUSTART, TRUE, pCFD)
		== ERROR)
                {
		DRV_LOG (DRV_DEBUG_START,
			 ("fei82557SCBCommand: command failed\n"),
			 0, 0, 0, 0, 0, 0);
                }
            }
        else
            {
            if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE, 0x0) 
		== ERROR)
                {
		DRV_LOG (DRV_DEBUG_START,
			 ("fei82557SCBCommand: command failed\n"),
			 0, 0, 0, 0, 0, 0);
                }
            }
        }

    /* wait for command to complete */

    for (ix = (FEI_ACTION_TMO * fei82557ClkRate); --ix;)
	{
	CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

	if (status & CFD_S_COMPLETE)
	    break;

	taskDelay (max(1, sysClkRateGet()/60));
	}

    if (!ix)
	{
	CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

	DRV_LOG (DRV_DEBUG_START,
		 "fei82557Action: Command %#x Status %#x CSR Status %#x\n",
				    action, status, scbStatus, 0, 0, 0);
	}

    /* interlock with fei82557CFDFree */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    return (status);
    }

/**************************************************************************
*
* fei82557MCastListForm - set up the multicast address list
*
* This routine populates the given CFD with the address list found in
* the apposite structure provided by the upper layers.
*
* RETURNS: N/A
*/

LOCAL void fei82557MCastListForm
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    CFD_ID	pCFD		/* pointer to a CFD */
    )

    {
    UINT32		mCastAddr;
    ETHER_MULTI	*	mCastNode;
    UINT16		count = 0;

    /* get the starting address for the multicast list */

    mCastAddr = (UINT32) CFD_MC_ADDR (pCFD);

    /* copy the multicast address list */

    for (mCastNode = (ETHER_MULTI *) lstFirst (&pDrvCtrl->endObj.multiList);
	 mCastNode != NULL; 
	 mCastNode = (ETHER_MULTI *) lstNext (&mCastNode->node))
	{
	bcopy ((char *) mCastNode->addr, (char *) mCastAddr, FEI_ADDR_LEN);
	mCastAddr += FEI_ADDR_LEN;
	count += FEI_ADDR_LEN;
	}

    /* set the byte counter for the list */

    CFD_WORD_WR (pCFD, CFD_MCOUNT_OFFSET, (count & ~0xc000));

    }

/**************************************************************************
*
* fei82557ConfigForm - fill the config CFD with proper values
*
* RETURNS: N/A
*/

LOCAL void fei82557ConfigForm
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    CFD_ID	pCFD		/* pointer to a CFD */
    )

    {
    UINT32 addr; 

    /* get to the starting address for config data */

    addr = (UINT32) pCFD + CFD_SIZE_HDR;

    /* set to config values recommeded by the i82557 User's Manual */

    /* YUCK !!! SO MANY MAGIC NUMBERS!!!! */


    CFD_CONFIG_WR (addr + 0, 0x16);
    CFD_CONFIG_WR (addr + 1, 0x88);
    CFD_CONFIG_WR (addr + 2, 0x00);
	
#if 0 /*wangfq*/
    if (pDrvCtrl->deviceId == 0 || pDrvCtrl->deviceId == FEI82557_DEVICE_ID)
        CFD_CONFIG_WR (addr + 3, 0x00);  /* this was set for the 82557 */
    else
#endif
        CFD_CONFIG_WR (addr + 3, 0x0f);  /* this was set for the 82558/9 */

    CFD_CONFIG_WR (addr + 4, 0x00);
    CFD_CONFIG_WR (addr + 5, 0x00);
    CFD_CONFIG_WR (addr + 6, 0x30); 
    CFD_CONFIG_WR (addr + 7, 0x03);
    CFD_CONFIG_WR (addr + 8, 0x01);	/* MII operation */
    CFD_CONFIG_WR (addr + 9, 0x00);
    CFD_CONFIG_WR (addr + 10, 0x2e);
    CFD_CONFIG_WR (addr + 11, 0x00);
    CFD_CONFIG_WR (addr + 12, 0x60);
    CFD_CONFIG_WR (addr + 13, 0x00);
    CFD_CONFIG_WR (addr + 14, 0xf2);

    if (FEI_FLAG_ISSET (FEI_PROMISC))
	{
	CFD_CONFIG_WR (addr + 15, 0x49);
	DRV_LOG (DRV_DEBUG_START, ("fei82557ConfigForm: PROMISC!!!!!!! \n"),
				    0, 0, 0, 0, 0, 0);
	}

    else
	CFD_CONFIG_WR (addr + 15, 0x48);      /* 0x49 in the old driver */

    CFD_CONFIG_WR (addr + 16, 0x00);
    CFD_CONFIG_WR (addr + 17, 0x40);
    CFD_CONFIG_WR (addr + 18, 0xf2);

    if (pDrvCtrl->board.phyDpx != PHY_HALF_DPX)
	CFD_CONFIG_WR (addr + 19, 0x80);
    else
	CFD_CONFIG_WR (addr + 19, 0x00); /* c0 force full duplex 0x80 */
 
    CFD_CONFIG_WR (addr + 20, 0x3f);

    if (FEI_FLAG_ISSET (FEI_MCASTALL))
	{
	CFD_CONFIG_WR (addr + 21, 0x0d);
	DRV_LOG (DRV_DEBUG_START, ("fei82557ConfigForm: MULTIALL \n"),
				    0, 0, 0, 0, 0, 0);
	}

    else
	CFD_CONFIG_WR (addr + 21, 0x05);      /* 0x05 in the old driver */

    }

/**************************************************************************
*
* fei82557Diag - format and issue a diagnostic command
*
* RETURNS: OK, or ERROR if the diagnostic command failed.
*/

LOCAL STATUS fei82557Diag
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    UINT16  stat;

    if (((stat = fei82557Action (pDrvCtrl, CFD_C_DIAG)) &
	(CFD_S_OK | CFD_S_DIAG_F)) != CFD_S_OK)
	{
	DRV_LOG (DRV_DEBUG_START,
		 "82557 diagnostics failed, cfdStatus 0x%x\n",
		 stat, 0, 0, 0, 0, 0);
	return (ERROR);
	}

     return (OK);
    } 

/**************************************************************************
*
* fei82557IASetup - issue an individual address command
*
* RETURNS: OK, or ERROR if the individual address command failed.
*/

LOCAL STATUS fei82557IASetup
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {

    if (!(fei82557Action (pDrvCtrl, CFD_C_IASETUP) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/**************************************************************************
*
* fei82557Config - issue a config command
*
* RETURNS: OK, or ERROR if the config command failed.
*/

LOCAL STATUS fei82557Config
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    if (!(fei82557Action (pDrvCtrl, CFD_C_CONFIG) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/*******************************************************************************
* fei82557Ioctl - interface ioctl procedure
*
* Process an interface ioctl request.
*
* RETURNS: OK, or ERROR if the config command failed.
*/

LOCAL int fei82557Ioctl
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT32        cmd,		/* command to process */
    caddr_t     data            /* pointer to data */
    )
    {
    int         error = OK;
    INT8        savedFlags;
    long        value;
    END_OBJ *   pEndObj=&pDrvCtrl->endObj;

    DRV_LOG (DRV_DEBUG_IOCTL,
             "Ioctl unit=0x%x cmd=%d data=0x%x\n",
             pDrvCtrl->unit, cmd, (int)data, 0, 0, 0);

    switch (cmd)
        {
        case EIOCSADDR:
            if (data == NULL)
                error = EINVAL;
            else
                {
                /* Copy and install the new address */

		bcopy ((char *) data,
		       (char *) FEI_HADDR (&pDrvCtrl->endObj),
		       FEI_HADDR_LEN (&pDrvCtrl->endObj));

		error = fei82557IASetup (pDrvCtrl);
                }

            break;

        case EIOCGADDR:                      
            if (data == NULL)
                error = EINVAL;
            else
		bcopy ((char *) FEI_HADDR (&pDrvCtrl->endObj),
		       (char *) data,
		       FEI_HADDR_LEN (&pDrvCtrl->endObj));

            break;

        case EIOCSFLAGS:
            value = (long) data;
            if (value < 0)
                {
                value = -value;
                value--;
                END_FLAGS_CLR (pEndObj, value);
                }
            else
                END_FLAGS_SET (pEndObj, value);

            DRV_LOG (DRV_DEBUG_IOCTL, ("endFlags=0x%x \n"),
					END_FLAGS_GET(pEndObj), 
					0, 0, 0, 0, 0);

            /* handle IFF_PROMISC */

            savedFlags = FEI_FLAG_GET();
            if (END_FLAGS_ISSET (IFF_PROMISC))
                FEI_FLAG_SET (FEI_PROMISC);
            else
                FEI_FLAG_CLEAR (FEI_PROMISC);
 
            /* handle IFF_MULTICAST */

            if (END_FLAGS_GET(pEndObj) & (IFF_MULTICAST))
                FEI_FLAG_SET (FEI_MCAST);
            else
                FEI_FLAG_CLEAR (FEI_MCAST);

            /* handle IFF_ALLMULTI */

            if (END_FLAGS_GET(pEndObj) & (IFF_ALLMULTI))
                FEI_FLAG_SET (FEI_MCASTALL);
            else
                FEI_FLAG_CLEAR (FEI_MCASTALL);

            DRV_LOG (DRV_DEBUG_IOCTL, "EIOCSFLAGS: 0x%x: 0x%x\n",
                    pEndObj->flags, savedFlags, 0, 0, 0, 0);

            if ((FEI_FLAG_GET () != savedFlags) &&
                (END_FLAGS_GET (pEndObj) & IFF_UP))
                {
		/* config down */

                END_FLAGS_CLR (pEndObj, IFF_UP | IFF_RUNNING); 

                error = fei82557Config (pDrvCtrl);

                END_FLAGS_SET (pEndObj, IFF_UP | IFF_RUNNING); /* config up */
                }

            break;

        case EIOCGFLAGS:
            DRV_LOG (DRV_DEBUG_IOCTL, "EIOCGFLAGS: 0x%x: 0x%x\n",
                    pEndObj->flags, *(long *)data, 0, 0, 0, 0);

            if (data == NULL)
                error = EINVAL;
            else
                *(long *)data = END_FLAGS_GET(pEndObj);

            break;

        case EIOCMULTIADD:
            error = fei82557MCastAddrAdd (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIDEL:
            error = fei82557MCastAddrDel (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIGET:
            error = fei82557MCastAddrGet (pDrvCtrl, (MULTI_TABLE *) data);
            break;

        case EIOCPOLLSTART:
            fei82557PollStart (pDrvCtrl);
            break;

        case EIOCPOLLSTOP:
	    DRV_LOG (DRV_DEBUG_POLL, ("IOCTL about to call fei82557PollStop\n"), 
					0, 0, 0, 0, 0, 0);

            fei82557PollStop (pDrvCtrl);
            break;

        case EIOCGMIB2:  
            if (data == NULL)
                error=EINVAL;
            else
		bcopy ((char *) &pEndObj->mib2Tbl, (char *) data,
			sizeof (pEndObj->mib2Tbl));

            break;

#ifdef INCLUDE_RFC_2233

        case EIOCGMIB2233:
            if ((data == NULL) || (pEndObj->pMib2Tbl == NULL))
                error = EINVAL;
            else
                *((M2_ID **)data) = pEndObj->pMib2Tbl;
            break;
#endif /* INCLUDE_RFC_2233 */

        default:
            DRV_LOG (DRV_DEBUG_IOCTL, ("INVALID IO COMMAND!! \n"),
					0, 0, 0, 0, 0, 0);
            error = EINVAL;
        }

    return (error);

    }

/*******************************************************************************
* fei82557MCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrAdd
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    char *      pAddr		/* address to be added */
    )
    {

    int		retVal;

    DRV_LOG (DRV_DEBUG_IOCTL, ("MCastAddrAdd\n"), 0, 0, 0, 0, 0, 0);

    retVal = etherMultiAdd (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
	{
	
        pDrvCtrl->endObj.nMulti++;
 
        if (pDrvCtrl->endObj.nMulti > N_MCAST)
            {
            etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);
            pDrvCtrl->endObj.nMulti--;
            }
        else
	    if (!(fei82557Action (pDrvCtrl, CFD_C_MASETUP) & CFD_S_OK))
		return (ERROR);
	    else
		return (OK);
	}

    return ((retVal == OK) ? OK : ERROR);

    }

/*******************************************************************************
*
* fei82557MCastAddrDel - delete a multicast address for the device
*
* This routine deletes a multicast address from the current list of
* multicast addresses.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrDel
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    char *      pAddr		/* address to be deleted */
    )
    {
    int		retVal;

    DRV_LOG (DRV_DEBUG_IOCTL, ("fei82557MCastAddrDel\n"), 0, 0, 0, 0, 0, 0);

    retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
	{
	if (!(fei82557Action (pDrvCtrl, CFD_C_MASETUP) & CFD_S_OK))
	    return (ERROR);

	pDrvCtrl->endObj.nMulti--;

	}

    return ((retVal == OK) ? OK : ERROR);
    }

/*******************************************************************************
* fei82557MCastAddrGet - get the current multicast address list
*
* This routine returns the current multicast address list in <pTable>
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrGet
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    MULTI_TABLE *pTable		/* table into which to copy addresses */
    )
    {
    DRV_LOG (DRV_DEBUG_IOCTL, ("fei82557MCastAddrGet\n"), 0, 0, 0, 0, 0, 0);

    return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
    }

/*******************************************************************************
* fei82557PollSend - transmit a packet in polled mode
*
* This routine is called by a user to try and send a packet on the
* device. It sends a packet directly on the network, without having to
* go through the normal process of queuing a packet on an output queue
* and the waiting for the device to decide to transmit it.
*
* These routine should not call any kernel functions.
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS fei82557PollSend
    (
    DRV_CTRL    *pDrvCtrl,      /* pointer to DRV_CTRL structure */
    M_BLK_ID    pMblk           /* pointer to the mBlk/cluster pair */
    )
    {
    volatile CFD_ID     pCFD;
    volatile CFD_ID     pPrevCFD;
    UINT16              command;
    volatile UINT16     status;
    volatile UINT16     scbStatus;
    BOOL                complete = FALSE;

    /* check if the command unit is active */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUSUSP)
        {
        DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: CU Active\n"),
                                     0, 0, 0, 0, 0, 0);
        return (EAGAIN);
        }

    /* check for CFDs availability */

    FREE_CFD_GET (pCFD);

    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
        {

        DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: No CFDs\n"),
                                     0, 0, 0, 0, 0, 0);

        return (EAGAIN);

        }

    if (fei82557Encap (pDrvCtrl, pCFD, pMblk) != OK)
        {
        logMsg ("polled encap failed?\n",0,0,0,0,0,0);
        return (EAGAIN);
        }

    /* this is a transmit command */

    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_TX);

    /* tie to the previous CFD */

    CFD_POINT_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD, CFD_ID);
    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

    /* kick the CU */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE,(UINT32) 0)
            == ERROR)
            {
            DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: Send Error\n"),
                                         0, 0, 0, 0, 0, 0);
            return (EAGAIN);
            }
        else
            {
            /* in the old driver ther was a second attempt
             * at transimitting the packet, but is it sensible?
             */
            }

    /* wait for command to complete */

    while (!complete)
        {
        CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

        if (status & CFD_S_COMPLETE)
            complete = TRUE;
        }

    complete = FALSE;

    /* wait for command to complete */

    while (!complete)
        {
        CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

        if ((scbStatus & SCB_S_CUMASK) == SCB_S_CUSUSP)
            complete = TRUE;
        }

    /*
     * Remove reference to the mBlk chain so fei82557CFDFree()
     * won't try to free it (caller has to do it).
     */

    CFD_LONG_WR (pCFD, CFD_MBLK_OFFSET, 0);

    /* free this CFD */

    fei82557CFDFree (pDrvCtrl);

    DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend\n"), 0, 0, 0, 0, 0, 0);

    return (OK);
    }

/*******************************************************************************
* fei82557PollReceive - receive a packet in polled mode
*
* This routine is called by a user to try and get a packet from the
* device. It returns EAGAIN if no packet is available. The caller must
* supply a M_BLK_ID with enough space to contain the received packet. If
* enough buffer is not available then EAGAIN is returned.
*
* These routine should not call any kernel functions.
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS fei82557PollReceive
    (
    DRV_CTRL    *pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK_ID    pMblk            /* pointer to the mBlk/cluster pair */
    )
    {
    volatile UINT16	rbdStatus;
    UINT16		retVal = OK;
    volatile UINT16     count;
    RBD_TAG *           pRbdTag;
    CL_BUF_ID		pData;

    DRV_LOG (DRV_DEBUG_POLL_RX, ("fei82557PollReceive\n"), 0, 0, 0, 0, 0, 0);

    if ((pMblk->mBlkHdr.mFlags & M_EXT) != M_EXT)
        return (EAGAIN);

    pRbdTag = &pDrvCtrl->rbdTags[pDrvCtrl->rbdIndex];
					
    /* read the RFD status word */		
    RBD_WORD_RD(pRbdTag->pRBD, RBD_STAT_OFFSET, rbdStatus);

    if (rbdStatus != RBD_STATUS_FREE)
        {
        /* get the actual received bytes number */
        RBD_WORD_RD(pRbdTag->pRBD, RBD_STAT_OFFSET, count);

        /*
         * Upper layer provides the buffer. If buffer is not large enough, 
         * we do not copy the received buffer.
         */

        /* Get the received cluster */
        RBD_BUF_RD(pRbdTag->pRBD,pData);

        FEI_CACHE_INVALIDATE(pRbdTag->pMblk->pClBlk->clNode.pClBuf,
                             pRbdTag->pMblk->mBlkHdr.mLen);

        if (pMblk->mBlkHdr.mLen < (count & ~0xc000))
            {
	    retVal = EAGAIN;
	    goto fei82557PollReceiveEnd;    
	    }

        pMblk->mBlkHdr.mFlags   |= M_PKTHDR;
        pMblk->mBlkHdr.mLen = count & ~0xc000;    
        pMblk->mBlkPktHdr.len   = pMblk->mBlkHdr.mLen;
        pMblk->mBlkHdr.mData += 2;

        bcopy ((char *)pData,pMblk->mBlkHdr.mData,pMblk->mBlkHdr.mLen);

#ifdef INCLUDE_RFC_2233
        /* RFC 2233 mib2 counter update for incoming packet */

        if (pDrvCtrl->endObj.pMib2Tbl != NULL)
            {
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn(pDrvCtrl->endObj.pMib2Tbl,
                                                     M2_PACKET_IN,
                                                     pMblk->mBlkHdr.mData,
                                                     pMblk->mBlkHdr.mLen);
            }
#endif /* INCLUDE_RFC_2233 */

        /* Clear the RBD's status */
        RFD_WORD_WR (pRbdTag->pRFD, RFD_STAT_OFFSET, (UINT16) 0);
        RBD_WORD_WR (pRbdTag->pRBD, RBD_STAT_OFFSET,0);

        /* Advance the rbdIndex */
        pDrvCtrl->rbdIndex = pRbdTag->next;
        }

fei82557PollReceiveEnd:

    DRV_LOG (DRV_DEBUG_POLL_RX, ("fei82557PollReceive... return \n"), 
				 0,0,0,0,0,0);

    return (retVal);
    }

/*******************************************************************************
* fei82557PollStart - start polling mode
*
* This routine starts polling mode by disabling ethernet interrupts and
* setting the polling flag in the END_CTRL stucture.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557PollStart
    (
    DRV_CTRL    *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;

    DRV_LOG (DRV_DEBUG_POLL, ("fei82557PollStart\n"), 0, 0, 0, 0, 0, 0);

    intLevel = intLock();

    /* disable system interrupt */

    SYS_INT_DISABLE (pDrvCtrl);     

    /* disable chip interrupt   */

    I82557_INT_DISABLE(SCB_C_M);

    FEI_FLAG_SET (FEI_POLLING);

    intUnlock (intLevel);

    return (OK);

    }

/*******************************************************************************
* fei82557PollStop - stop polling mode
*
* This routine stops polling mode by enabling ethernet interrupts and
* resetting the polling flag in the END_CTRL structure.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557PollStop
    (
    DRV_CTRL    *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;

    intLevel = intLock();

    /* enable system interrupt */

    SYS_INT_ENABLE (pDrvCtrl);     

    /* enable chip interrupt   */

    I82557_INT_ENABLE(SCB_C_M);

    /* set flags */

    FEI_FLAG_CLEAR (FEI_POLLING);

    intUnlock (intLevel);

    fei82557Restart(pDrvCtrl);

    DRV_LOG (DRV_DEBUG_POLL, ("fei82557PollStop... end\n"), 0, 0, 0, 0, 0, 0);

    return (OK);
    }

/**************************************************************************
*
* fei82557PhyInit - initialize and configure the PHY device if there is one
*
* This routine initialize and configure the PHY device if there is one.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557PhyInit
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int status;
    int phyDevice;

    /* not a MII interface, no need to initialize PHY */

    if (pDrvCtrl->board.phyAddr > 31)
	return (OK);  

    /* configure the Physical layer medium if it's MII interface */

    /* starts with logical PHY 1 */

    phyDevice = (pDrvCtrl->board.phyAddr) ? pDrvCtrl->board.phyAddr : 1;
	
    status = fei82557MDIPhyLinkSet (pDrvCtrl, phyDevice);
DRV_LOG (DRV_DEBUG_LOAD,
	       	       ("fei82557MDIPhyLinkSet status = %x\n"),
		       status, 0, 0, 0, 0, 0); /*wangfq*/
    /*  Try a few more times to get a valid link */

    if (status != OK)
	{

        /* try getting a valid link with the fallback phy device value */

        phyDevice = 0;

        status = fei82557MDIPhyLinkSet (pDrvCtrl, phyDevice);

        if (status != OK)
            {

            /* 
             * Don't leave the fallback phy value configured 
             * restore the phy device value, reconfigure phy with it.
             */

            phyDevice = (pDrvCtrl->board.phyAddr) ? pDrvCtrl->board.phyAddr : 1;

            status = fei82557MDIPhyLinkSet (pDrvCtrl, phyDevice);

            if (status != OK)
                {
	        DRV_LOG (DRV_DEBUG_LOAD,
	       	       ("LINK_FAIL error, check the line connection !!!\n"),
		       0, 0, 0, 0, 0, 0);
	        return (status);
	        }
            }
        }

    /* we are here if a valid link is established */

    status = fei82557MDIPhyConfig (pDrvCtrl, phyDevice); 
DRV_LOG (DRV_DEBUG_LOAD,
	       	       ("fei82557MDIPhyConfig status = %x\n"),
		       status, 0, 0, 0, 0, 0); /*wangfq*/
    if (status == PHY_AUTO_FAIL)
	{
	/* force default speed and duplex */

	pDrvCtrl->board.phySpeed = PHY_10MBS;
	pDrvCtrl->board.phyDpx = PHY_HALF_DPX; 
	pDrvCtrl->board.others = 0;

	/* and configure it again */

	status = fei82557MDIPhyConfig (pDrvCtrl, phyDevice);  
	}
DRV_LOG (DRV_DEBUG_LOAD,
	       	       ("fei82557PhyInit status = %x\n"),
		       status, 0, 0, 0, 0, 0); /*wangfq*/
    return (status);
    } 
/**************************************************************************
*
* fei82557MDIPhyLinkSet - detect and set the link for the PHY device
*
* This routine first checks if the link has been established.  If not, it
* isolates the other one, and tries to establish the link.  PHY device 0 is
* always at PHY address 0.  PHY 1 can be at 1-31 PHY addresses.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557MDIPhyLinkSet
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int phyAddr			/* physical address */
    )

    {
    UINT16 ctlReg;
    UINT16 statusReg;
    int isoDev, i;

    /* read control register */

    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, phyAddr, &ctlReg);

    /* read again */

    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, phyAddr, &ctlReg);

    /* check if the PHY is there */

    if (ctlReg == (UINT16)0xffff)
	return (ERROR);   /* no PHY present */

    /* The PHY is there, read status register  */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* in case the status bit is the latched bit */

    if ( !(statusReg & MDI_SR_LINK_STATUS))
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    if (statusReg & MDI_SR_LINK_STATUS)
	return (OK);  /* Device found and link OK */

    /* no link is established, let's configure it */

    /* isolates the other PHY */

    isoDev = (phyAddr) ? 0 : 1;
    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, isoDev, MDI_CR_ISOLATE);

    /* wait for a while */

    taskDelay (max (2, sysClkRateGet()/30));

    /* enable the PHY device we try to configure */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, MDI_CR_SELECT);

    /* wait for a while for command take effect */

    taskDelay (max (2, sysClkRateGet()/30)); 

    /* restart the auto negotiation process, execute anyways even if
     * it has no such capability.
     */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, 
		     MDI_CR_RESTART | MDI_CR_SELECT);

    /* wait for auto-negotiation to complete */

    for (i = 0; i < 80; i++)
	{
	/* read the status register */

	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

	if (statusReg & (MDI_SR_AUTO_NEG | MDI_SR_REMOTE_FAULT) )
	    break;

	taskDelay (max (2, sysClkRateGet()/30));

	if (!(statusReg & MDI_SR_AUTO_SELECT))
	    break;  /* no such capability */
	}

    /* Read the status register */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* some of the status bits require to clear a latch */

    if (!(statusReg & MDI_SR_LINK_STATUS))
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    if (statusReg & MDI_SR_LINK_STATUS)
	return (OK);  /* Link configure done and successful */

    /* device is there, cann't establish link */

    return (PHY_LINK_ERROR);   
    } 

/**************************************************************************
*
* fei82557MDIPhyConfig - configure the PHY device
*
* This routine configures the PHY device according to the parameters
* specified by users or the default value.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557MDIPhyConfig
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int phyAddr
    )

    {
    UINT16 ctlReg = 0;
    int fullDpx=FALSE;
    int autoSelect =FALSE;
    UINT16 statusReg;
    int status, i;

    /* find out what capabilities the device has */

    /*  read status register  */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* some of the status bits require to read twice */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* The device at least has to have the half duplex and 10mb speed */

    if (statusReg & (MDI_SR_10T_FULL_DPX | MDI_SR_TX_FULL_DPX))
	fullDpx = TRUE;

    if (statusReg & MDI_SR_AUTO_SELECT)
	autoSelect = TRUE;

    DRV_LOG (DRV_DEBUG_LOAD, ("status REG = %x !!!! \n"),
				statusReg, 0, 0, 0, 0, 0);

    if (pDrvCtrl->board.phyDpx == PHY_FULL_DPX && fullDpx == TRUE)
	ctlReg |= MDI_CR_FDX;

    if (pDrvCtrl->board.phySpeed == PHY_100MBS)
	ctlReg |= MDI_CR_100;

    if (pDrvCtrl->board.phySpeed == PHY_AUTO_SPEED || 
	pDrvCtrl->board.phyDpx == PHY_AUTO_DPX )
	{
	if (autoSelect != TRUE)
	    {
	    /* set back to default */

	    pDrvCtrl->board.phySpeed = PHY_10MBS;
	    pDrvCtrl->board.phyDpx = PHY_HALF_DPX;
	    ctlReg |= (PHY_10MBS | PHY_HALF_DPX);
	    }
	else
	    {
	    ctlReg |= (MDI_CR_SELECT | MDI_CR_RESTART);
	    }
	 }

    /* or other possible board level selection */

    ctlReg |= pDrvCtrl->board.others;

    /* configure the PHY */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, ctlReg);
   
    /* wait for a while */

    taskDelay (max (2, sysClkRateGet()/30));   

    if (!(ctlReg & MDI_CR_RESTART))
	return (OK);

    /* we are here if the restart auto negotiation is selected */

    DRV_LOG (DRV_DEBUG_LOAD, ("auto NEGOTIATION STARTS !!!! \n"),
			      0, 0, 0, 0, 0, 0);

    /* wait for it done */

    for (status = PHY_AUTO_FAIL, i = 0; i < 80; i++)
	{
	/* read status register, first read clears */

	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

	if (statusReg & MDI_SR_AUTO_NEG)
	    {
	    status = OK;  /* auto negotiation completed */
	    break;
	    }

	if (statusReg & MDI_SR_REMOTE_FAULT)
	    {
	    status = PHY_AUTO_FAIL;  /* auto negotiation fails */
	    break;
	    }
	}
	      
	return (status);
} 

/**************************************************************************
*
* fei82557MDIRead - read the MDI register
*
* This routine reads the specific register in the PHY device
* Valid PHY address is 0-31
*
* RETURNS: OK or ERROR.
*
*/

static int fei82557MDIRead
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int regAddr,
    int phyAddr,
    UINT16 *retVal
    )

    {

    int			i;
    volatile UINT32	mdrValue;

    mdrValue = ((regAddr << 16) | (phyAddr << 21) | (MDI_READ << 26));

    /* write to MDI it was done differently!! */

    CSR_LONG_WR (CSR_MDI_OFFSET, mdrValue);

    /* wait for a while */

    taskDelay (max (2, sysClkRateGet()/30));    

    /* check if it's done */

    for (i = 0; i < 40; i++)
	{
	CSR_LONG_RD (CSR_MDI_OFFSET, mdrValue);

	/* check for correct completion */

	if (mdrValue & (1 << 28))
	    {
	    break;
	    }

	taskDelay (max (1, sysClkRateGet()/60));
	}

     if (i==40)
	 return (ERROR);

     *retVal = (UINT16) (mdrValue & 0xffff);

     return (OK);

     }

/**************************************************************************
*
* fei82557MDIWrite - write to the MDI register
*
* This routine writes the specific register in the PHY device
* Valid PHY address is 0-31
*
* RETURNS: OK or ERROR.
*/

static int fei82557MDIWrite
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int regAddr,
    int phyAddr,
    UINT16 writeData
    )

    {

    int			i;
    volatile UINT32	mdrValue;

    mdrValue = ((regAddr << 16) | (phyAddr << 21) | 
		(MDI_WRITE << 26) | writeData);

    /* write to MDI it was done differently!! */

    CSR_LONG_WR (CSR_MDI_OFFSET, mdrValue);

    /* wait for a while */

    taskDelay (max (2, sysClkRateGet()/30));    

    /* check if it's done */

    for (i = 0; i < 40; i++)
	{
	CSR_LONG_RD (CSR_MDI_OFFSET, mdrValue);

	/* check for correct completion */

	if (mdrValue & (1 << 28))
	    {
	    break;
	    }

	taskDelay (max (1, sysClkRateGet()/60));
	}

     if (i==40)
	 return (ERROR);

     return (OK);
     }

/**************************************************************************
*
* fei82557NOP - format and issue a NOP command
*
* RETURNS: OK, or ERROR if the NOP command failed.
*
*/

LOCAL STATUS fei82557NOP
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    if (!(fei82557Action (pDrvCtrl, CFD_C_NOP) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/**************************************************************************
*
* fei82557GetRUStatus - Return the current RU status and int mask
*
* RETURNS: N/A
*
*/

void fei82557GetRUStatus
    (
    int unit
    )
    {
    DRV_CTRL *  pDrvCtrl;       /* pointer to DRV_CTRL structure */
    UINT16 scbStatus;
    UINT8  mask;
    pDrvCtrl = (DRV_CTRL *) endFindByName("fei",unit);

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);
    CSR_BYTE_RD (CSR_INT_OFFSET, mask);

    logMsg("RU status 0x%x mask 0x%x\n",scbStatus,mask,0,0,0,0);
    }

/**************************************************************************
*
* fei82557ShowRxRing -  Show the Receive ring
*
* This routine dumps the contents of the RFDs and RBDs in the Rx ring.
*
* RETURNS: N/A
*
*/

void fei82557ShowRxRing
    (
    int unit
    )
    {
    DRV_CTRL *          pDrvCtrl;
    int                 ix;
    UINT16              status;
    RBD_ID              pRBD;
    RFD_ID              pRFD;
    RBD_TAG *           pRbdTag;
    CL_BUF_ID           pCluster;
    CL_BUF_ID           pClNext;

    pDrvCtrl = (DRV_CTRL *) endFindByName("fei", unit);

    pRFD = pDrvCtrl->pRFD;

    logMsg("pDrvCtrl->nRFDs %d\n",pDrvCtrl->nRFDs,0,0,0,0,0);
    logMsg("pDrvCtrl->pRfdBase  %p\n",(int)pDrvCtrl->pRfdBase,0,0,0,0,0);

    logMsg("pDrvCtrl->nRBDs %d\n",pDrvCtrl->nRBDs,0,0,0,0,0);
    logMsg("pDrvCtrl->pRbdBase %p\n",(int)pDrvCtrl->pRbdBase,0,0,0,0,0); 

    logMsg("pDrvCtrl->rbdIndex  %d\n",(int)pDrvCtrl->rbdIndex,0,0,0,0,0); 
    logMsg("pDrvCtrl->eLRbdTag  %p index %d\n",(int)pDrvCtrl->eLRbdTag,
            pDrvCtrl->eLRbdTag->index,0,0,0,0); 

    for (ix = 0; ix < pDrvCtrl->nRBDs; ix++)
        {
        pRbdTag  = &pDrvCtrl->rbdTags[ix];

        logMsg("\n",0,0,0,0,0,0);

        logMsg("pDrvCtrl->rbdTags[%d].index %d\n",ix,pRbdTag->index,
               0,0,0,0);
        logMsg("pDrvCtrl->rbdTags[%d].next %d\n",ix,pRbdTag->next,
               0,0,0,0);
        logMsg("pDrvCtrl->rbdTags[%d].pMblk %x\n",ix,(int)pRbdTag->pMblk,
               0,0,0,0);
        logMsg("pDrvCtrl->rbdTags[%d].pRFD %p\n",ix,(int)pRbdTag->pRFD,
               0,0,0,0);
        logMsg("pDrvCtrl->rbdTags[%d].pRBD %p\n",ix,(int)pRbdTag->pRBD,
               0,0,0,0);

        pRBD = pRbdTag->pRBD;

        RBD_WORD_RD(pRBD,RBD_STAT_OFFSET,status);
        logMsg("Status %x \n",status,0,0,0,0,0);

        RBD_WORD_RD(pRBD,RBD_EOF_OFFSET,status);
        logMsg("EOF %x \n",status,0,0,0,0,0);

        RBD_BUF_RD(pRBD,pCluster);
        pClNext = (CL_BUF_ID)pCluster->pClNext;

        logMsg("Buffer %p  pClNext %p\n",(int)pCluster,(int)pCluster->pClNext,
               0,0,0,0);

        RBD_LONG_RD(pRBD,RBD_CFG_OFFSET,status);
        logMsg("Config %x \n",status,0,0,0,0,0);

        }
    }

#ifdef DRV_DEBUG557

/******************************************************************************
*
* fei82557DumpPrint - Display statistical counters
*
* This routine displays i82557 statistical counters
*
* RETURNS: OK, or ERROR if the DUMP command failed.
*/

STATUS fei82557DumpPrint
    (
    int unit       /* pointer to DRV_CTRL structure */
    )
    {
    UINT32      dumpArray [18];
    UINT32 *    pDump;
    DRV_CTRL *  pDrvCtrl;       /* pointer to DRV_CTRL structure */
    STATUS status = OK;

    pDrvCtrl = (DRV_CTRL *) endFindByName("fei", unit);

    /* dump area must be long-word allign */

    pDump = (UINT32 *) (((UINT32) dumpArray + 4) & 0xfffffffc);

    status = fei82557ErrCounterDump (pDrvCtrl, pDump);
FEI_CACHE_INVALIDATE(pDump, 18*4); /*wangfq*/
    DRV_LOG (DRV_DEBUG_DUMP, ("\n"), 0, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx good frames:	     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx MAXCOL errors:	   %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx LATECOL errors:	  %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx underrun errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx lost CRS errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx deferred:		%d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx single collisions:       %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx multiple collisions:     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx total collisions:	%d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx good frames:	     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx CRC errors:	      %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx allignment errors:       %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx resource errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx overrun errors:	  %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx collision detect errors: %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx short frame errors:      %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    return (status);
    }

/*****************************************************************************
*
* fei82557ErrCounterDump - dump statistical counters
*
* This routine dumps statistical counters for the purpose of debugging and
* tuning the 82557.
*
* The <memAddr> parameter is the pointer to an array of 68 bytes in the
* local memory.  This memory region must be allocated before this routine is
* called.  The memory space must also be DWORD (4 bytes) aligned.  When the
* last DWORD (4 bytes) is written to a value, 0xa007, it indicates the dump
* command has completed.  To determine the meaning of each statistical
* counter, see the Intel 82557 manual.
*
* RETURNS: OK or ERROR.
*/

STATUS fei82557ErrCounterDump
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT32 *	memAddr
    )

    {
    STATUS status = OK;
    int i;

    memAddr[16]=0;    /* make sure the last DWORD is 0 */

    /* tells the 82557 where to write dump data */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CULDDUMP, TRUE, memAddr)
	== ERROR)
	status = ERROR;

    /* issue the dump and reset counter command */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUDUMPRST, 
			   FALSE, (UINT32) 0) 
	== ERROR)
	status = ERROR;

    /* waits for it done */

    for (i = 0; i < 60; i++)
    {
    if (memAddr[16] == (UINT32) 0xa007)
	break;

	taskDelay (max (2, sysClkRateGet()/30));
    }

    if (i==60)
	status = ERROR;

    return (status);
    }
#endif /* End of Debug code */
