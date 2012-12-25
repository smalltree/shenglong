/* cacheLsn2eLib.c - MIPS Loongson2e cache management library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
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
01n,17may02,pes  Before aborting cacheTextUpdate() on null pointer, check for
                 ENTIRE_CACHE.
01m,08may02,pes  Add protection against null pointers, zero counts, and
                 requests involving kseg1 in cacheXXXTextUpdate().
01l,16jul01,ros  add CofE comment
01k,22jun00,dra  Added cache sync support, updated DMA funcs and virt addr
                 flushing.
01j,16jun00,dra  update for idts134
01e,13jun00,dra  generalize the mmuMipsUnMap flush of virtual page addresses
                 to all Mips architectures, correct cdf file discrepancies
01d,22mar00,dra  Fixed compiler warnings.
01c,05dec96,kkk  added missing return value to cacheLsn2eFlush() and
                 cacheLsn2eInvalidate().
01b,12oct94,caf  tweaked documentation (SPR #3464).
01a,01oct93,cd   derived from cacheR3kLib.c v01h.
*/

/*
DESCRIPTION
This library contains architecture-specific cache library functions for
the MIPS Loongson2e architecture.  The Loongson2e utilizes a variable-size
instruction and data cache that operates in write-back mode.  Cache
line size also varies.  

For general information about caching, see the manual entry for cacheLib.

INCLUDE FILES: cacheLib.h

SEE ALSO: cacheLib
*/

#include	"vxWorks.h"
#include	"cacheLib.h"
#include	"memLib.h"
#include	"stdlib.h"
#include	"errno.h"
#include        "errnoLib.h"
#ifdef IS_KSEGM
#include        "memPartLib.h"
#include        "arch/mips/mmuMipsLib.h"
#endif
#include        "private/vmLibP.h"
#include        "private/funcBindP.h"

/* forward declarations */
LOCAL void *	cacheLsn2eMalloc (size_t bytes);
LOCAL STATUS	cacheLsn2eFree (void * pBuf);
LOCAL STATUS	cacheLsn2eFlush (CACHE_TYPE cache, void *	pVirtAdrs,
			       size_t bytes);
LOCAL STATUS	cacheLsn2eInvalidate (CACHE_TYPE cache, void * pVirtAdrs, 
                                    size_t bytes);
LOCAL void * 	cacheLsn2ePhysToVirt (void * address);
LOCAL void * 	cacheLsn2eVirtToPhys (void * address);
LOCAL STATUS 	cacheLsn2eTextUpdate (void * address, size_t bytes);
LOCAL STATUS	cacheLsn2ePipeFlush (void);

/* imports */

IMPORT int	cacheLsn2eEnable(void);
IMPORT int	cacheLsn2eDisable(void);

IMPORT void	sysWbFlush (void);

IMPORT void cacheLsn2eDCFlushInvalidate(void * address,
				 size_t byteCount);
IMPORT void cacheLsn2eDCFlushInvalidateAll(void);
IMPORT void cacheLsn2eICInvalidate(void * address,
			    size_t byteCount);
IMPORT void cacheLsn2eICInvalidateAll(void);

IMPORT void cacheLsn2eFlushInvalidateAll(void);
IMPORT void cacheLsn2eDCInvalidate(void * baseAdr, size_t byteCount);
IMPORT void cacheLsn2eDCInvalidateAll(void);

#ifdef IS_KSEGM
IMPORT VOIDFUNCPTR _func_mipsCacheSync;
IMPORT VOIDFUNCPTR _func_mmuMipsVirtPageFlush;
#endif /* IS_KSEGM */

IMPORT void	cacheLsn2eSync (void * vAddr, UINT len);
IMPORT void	cacheLsn2eVirtPageFlush (UINT asid, void * vAddr, UINT len);

/* globals */

#if 0
IMPORT UINT32 cacheLsn2eICacheSize;
IMPORT UINT32 cacheLsn2eDCacheSize;
IMPORT UINT32 cacheLsn2eSCacheSize;
IMPORT UINT32 cacheLsn2eICacheLineSize;
IMPORT UINT32 cacheLsn2eDCacheLineSize;
IMPORT UINT32 cacheLsn2eSCacheLineSize;
#endif

/**************************************************************************
*
* cacheLsn2eLibInit - initialize the Loongson2e cache library
*
* This routine initializes the function pointers for the Loongson2e cache
* library.  The board support package can select this cache library 
* by assigning the function pointer <sysCacheLibInit> to
* cacheLsn2eLibInit().
*
* RETURNS: OK.
*/

STATUS cacheLsn2eLibInit
    (
    CACHE_MODE	instMode,	/* instruction cache mode */
    CACHE_MODE	dataMode	/* data cache mode */

#if 0
    UINT32	iCacheSize,
    UINT32	iCacheLineSize,
    UINT32	dCacheSize,
    UINT32	dCacheLineSize,
    UINT32	sCacheSize,
    UINT32	sCacheLineSize
#endif
    )
    {
#if 0
    cacheLsn2eICacheSize = iCacheSize;
    cacheLsn2eDCacheSize = dCacheSize;
    cacheLsn2eSCacheSize = sCacheSize;
    cacheLsn2eICacheLineSize = iCacheLineSize;
    cacheLsn2eDCacheLineSize = dCacheLineSize;
    cacheLsn2eSCacheLineSize = sCacheLineSize;
#endif

    cacheLib.enableRtn = cacheLsn2eEnable;
    cacheLib.disableRtn = cacheLsn2eDisable;

    cacheLib.lockRtn = NULL;			/* cacheLock */
    cacheLib.unlockRtn = NULL;			/* cacheUnlock */

    cacheLib.flushRtn = cacheLsn2eFlush;		/* cacheFlush() */
    cacheLib.pipeFlushRtn = cacheLsn2ePipeFlush;	/* cachePipeFlush() */
    cacheLib.textUpdateRtn = cacheLsn2eTextUpdate;/* cacheTextUpdate() */

    cacheLib.invalidateRtn = cacheLsn2eInvalidate;/* cacheInvalidate() */
    cacheLib.clearRtn = cacheLsn2eInvalidate;	/* cacheClear() */

    cacheLib.dmaMallocRtn = (FUNCPTR) cacheLsn2eMalloc;	/* cacheDmaMalloc() */
    cacheLib.dmaFreeRtn = cacheLsn2eFree;			/* cacheDmaFree() */

#ifdef IS_KSEGM
    if (!IS_KSEGM(cacheLsn2eLibInit))
	{
	cacheLib.dmaVirtToPhysRtn = (FUNCPTR) cacheLsn2eVirtToPhys;
	cacheLib.dmaPhysToVirtRtn = (FUNCPTR) cacheLsn2ePhysToVirt;
	_func_mipsCacheSync = (VOIDFUNCPTR) cacheLsn2eSync;
	}
    else
	{
	_func_mipsCacheSync = (VOIDFUNCPTR) KM_TO_K0(cacheLsn2eSync);
	_func_mmuMipsVirtPageFlush = (VOIDFUNCPTR) cacheLsn2eVirtPageFlush;
	_func_mmuMipsVirtPageFlush = NULL;
	}
#else /* IS_KSEGM */
    cacheLib.dmaVirtToPhysRtn = (FUNCPTR) cacheLsn2eVirtToPhys;
    cacheLib.dmaPhysToVirtRtn = (FUNCPTR) cacheLsn2ePhysToVirt;
#endif /* IS_KSEGM */

    cacheDataMode	= dataMode;		/* save dataMode for enable */
    cacheDataEnabled	= TRUE;			/* d-cache is currently on */
    cacheMmuAvailable	= TRUE;			/* mmu support is provided */

    cacheFuncsSet ();				/* update cache func ptrs */
#if 0
		cacheDmaFuncs.flushRtn = cacheLib.flushRtn;
	    cacheDmaFuncs.invalidateRtn	= cacheLib.invalidateRtn;
	    cacheDmaFuncs.virtToPhysRtn	= cacheLib.dmaVirtToPhysRtn;
	    cacheDmaFuncs.physToVirtRtn	= cacheLib.dmaPhysToVirtRtn;

	    cacheDmaMallocRtn		= cacheLib.dmaMallocRtn;
	    cacheDmaFreeRtn		= cacheLib.dmaFreeRtn;

		cacheUserFuncs.flushRtn = cacheLib.flushRtn;
	    cacheUserFuncs.invalidateRtn	= cacheLib.invalidateRtn;
	   /* cacheUserFuncs.virtToPhysRtn	= cacheLib.dmaVirtToPhysRtn;
	    cacheUserFuncs.physToVirtRtn	= cacheLib.dmaPhysToVirtRtn;*/
#endif
    return (OK);
    }


/**************************************************************************
*
* cacheLsn2eMalloc - allocate a cache-safe buffer, if possible
*
* This routine will attempt to return a pointer to a section of memory
* that will not experience any cache coherency problems.
*
* INTERNAL
* This function is complicated somewhat because the cache operates in
* copyback mode and we need to avoid problems from writebacks of adjacent
* cached lines; we also need to remember the pointer returned by malloc so
* that we can free it if required.
*
* RETURNS: A pointer to the non-cached buffer, or NULL.
*/

LOCAL void * cacheLsn2eMalloc
    (
    size_t bytes 
    )
    {
    void * pDmaBuffer;
    
#ifdef IS_KSEGM
    int    pageSize;

    /* check for non-memory mapped case */
    if (IS_KSEG0(cacheLsn2eMalloc))
	{
#endif
	int	allocBytes;
	void  * pBuffer;

        /* Round up the allocation size so that we can store a "back pointer"
         * to the allocated buffer, align the buffer on a cache line boundary
         * and pad the buffer to a cache line boundary.
         * sizeof(void *) 		for "back pointer"
         * _CACHE_ALIGN_SIZE-1	for cache line alignment
         * _CACHE_ALIGN_SIZE-1	for cache line padding
         */
        allocBytes = sizeof (void *) +
	  	    (_CACHE_ALIGN_SIZE - 1) +
		    bytes +
		    (_CACHE_ALIGN_SIZE - 1);

        if ((pBuffer = (void *)malloc (allocBytes)) == NULL)
	    return (NULL);

        /* Flush any data that may be still sitting in the cache */
	cacheLsn2eDCFlushInvalidate (pBuffer, allocBytes);

	pDmaBuffer = pBuffer;

	/* allocate space for the back pointer */
	pDmaBuffer = (void *)((int)pDmaBuffer + sizeof (void *));

	/* Now align to a cache line boundary */
	pDmaBuffer = (void *)CACHE_ROUND_UP (pDmaBuffer);

	/* Store "back pointer" in previous cache line using CACHED location */
	*(((void **)pDmaBuffer)-1) = pBuffer;

	return ((void *)K0_TO_K1(pDmaBuffer));
#ifdef IS_KSEGM
       	}
    
    /* memory-mapped case */

    if ((pageSize = VM_PAGE_SIZE_GET ()) == ERROR)
	return (NULL);

    /* make sure bytes is a multiple of pageSize */

    bytes = bytes / pageSize * pageSize + pageSize;

    pDmaBuffer = (void *)IOBUF_ALIGNED_ALLOC (bytes, pageSize);
    if (pDmaBuffer == NULL)
	return (NULL);

    VM_STATE_SET (NULL, pDmaBuffer, bytes,
		  MMU_ATTR_CACHE_MSK, MMU_ATTR_CACHE_OFF);

    return (pDmaBuffer);
#endif /* IS_KSEGM */
    }
    

/**************************************************************************
*
* cacheLsn2eFree - free the buffer acquired by cacheMalloc ()
*
* This routine restores the non-cached buffer to its original state
* and does whatever else is necessary to undo the allocate function.
*
* RETURNS: OK, or ERROR if not able to undo cacheMalloc() operation
*/

LOCAL STATUS cacheLsn2eFree
    (
    void * pBuf 
    )
    {
    void      * pCacheBuffer;
    
#ifdef IS_KSEGM
    BLOCK_HDR * pHdr;		/* pointer to block header */
    STATUS	status = OK;	/* return value */
    /* Check for unmapped case */
    if (IS_KSEG1(pBuf))
	{
#endif /* IS_KSEGM */
	pCacheBuffer = (void *)K1_TO_K0(pBuf);
	pCacheBuffer = (void *)((int)pCacheBuffer - sizeof (void *));
	free (*(void **)pCacheBuffer);
#ifdef IS_KSEGM
	}
    else
	{
	if (vmLibInfo.vmLibInstalled)
	    {
	    pHdr = BLOCK_TO_HDR (pBuf);

	    /*
	     * XXX - cache mode is set back to the default one. This may be
	     * a problem since we do not know if the original cache mode was either 
	     * COPY_BACK or WRITETHROUGH.
	     */

	    status = VM_STATE_SET (NULL, pBuf, BLOCK_SIZE (pHdr),
				   MMU_ATTR_CACHE_MSK, MMU_ATTR_CACHE_DEFAULT);
	    }
	IOBUF_FREE (pBuf);		/* free buffer after modified */

	return (status);
	}
#endif /* IS_KSEGM */
    return (OK);
    }


/**************************************************************************
*
* cacheLsn2eFlush - flush all or some entries in a cache
*
* This routine flushes (writes to memory)  all or some of the entries in the
* specified cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

LOCAL STATUS	cacheLsn2eFlush
    (
    CACHE_TYPE	cache,			/* Cache to Invalidate */
    void *	pVirtAdrs,		/* Virtual Address */
    size_t	bytes 			/* Number of Bytes to Invalidate */
    )
    {
    if (IS_KSEG1(pVirtAdrs))
	return(OK);
    switch (cache)
	{
	case DATA_CACHE:
	    if (bytes == ENTIRE_CACHE)
		cacheLsn2eDCFlushInvalidateAll ();
	    else
		cacheLsn2eDCFlushInvalidate (pVirtAdrs, bytes);
	    break;
	default:
	    errno = S_cacheLib_INVALID_CACHE;
	    return (ERROR);
	    break;
        }

    return (OK);
    }

/**************************************************************************
*
* cacheLsn2eInvalidate - invalidate all or some entries in a cache
*
* This routine invalidates all or some of the entries in the
* specified cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

LOCAL STATUS	cacheLsn2eInvalidate
    (
    CACHE_TYPE	cache,			/* Cache to Invalidate */
    void *	pVirtAdrs,		/* Virtual Address */
    size_t	bytes 			/* Number of Bytes to Invalidate */
    )
    {
    if (IS_KSEG1(pVirtAdrs))
	return(OK);
    switch (cache)
	{
	case DATA_CACHE:
	    if (bytes == ENTIRE_CACHE)
		cacheLsn2eDCInvalidateAll ();
	    else
		cacheLsn2eDCInvalidate (pVirtAdrs, bytes);
	    break;
	case INSTRUCTION_CACHE:
	    if (bytes == ENTIRE_CACHE)
		cacheLsn2eICInvalidateAll ();
	    else
		cacheLsn2eICInvalidate (pVirtAdrs, bytes);
	    break;
	default:
	    errno = S_cacheLib_INVALID_CACHE;
	    return (ERROR);
	    break;
        }

    return (OK);
    }

/**************************************************************************
*
* cacheLsn2eVirtToPhys - virtual-to-physical address translation
*
* This routine may be attached to the CACHE_DRV structure virtToPhysRtn
* function pointer by cacheLsn2eMalloc().  This implies that the virtual
* memory library is not installed, and that the "cache-safe" buffer has
* been created through the use of the Loongson2e K1 segment.
*
* NOMANUAL
*/

LOCAL void * cacheLsn2eVirtToPhys
    (
    void * address                      /* Virtual address */
    )
    {
    return ((void *) K0_TO_PHYS(address));
    }

/**************************************************************************
*
* cacheLsn2ePhysToVirt - physical-to-virtual address translation
*
* This routine may be attached to the CACHE_DRV structure physToVirtRtn
* function pointer by cacheLsn2eMalloc().  This implies that the virtual
* memory library is not installed, and that the "cache-safe" buffer has
* been created through the use of the Loongson2e K1 segment.
*
* NOMANUAL
*/

LOCAL void * cacheLsn2ePhysToVirt
    (
    void * address                      /* Physical address */
    )
    {
    return ((void *) PHYS_TO_K0(address));
    }

/**************************************************************************
*
* cacheLsn2eTextUpdate - invalidate updated text section
*
* This routine invalidates the specified text section so that
* the correct updated text is executed.
*
* NOMANUAL
*/

LOCAL STATUS cacheLsn2eTextUpdate
    (
    void * address,                     /* Physical address */
    size_t bytes 			/* bytes to invalidate */
    )
    {
    if ((bytes != ENTIRE_CACHE) &&
	((address == NULL) || (bytes == 0) || IS_KSEG1(address)))
	return (OK);
    
    if (cacheLsn2eFlush (DATA_CACHE, address, bytes) != OK)
	return (ERROR);
    return (cacheLsn2eInvalidate (INSTRUCTION_CACHE, address, bytes));
    }


/**************************************************************************
*
* cacheLsn2ePipeFlush - flush Loongson2e write buffers to memory
*
* This routine forces the processor output buffers to write their contents 
* to RAM.  A cache flush may have forced its data into the write buffers, 
* then the buffers need to be flushed to RAM to maintain coherency.
* It simply calls the sysWbFlush routine from the BSP.
*
* RETURNS: OK.
*
* NOMANUAL
*/

LOCAL STATUS cacheLsn2ePipeFlush (void)
    {
    sysWbFlush ();
    return (OK);
    }
