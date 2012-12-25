/* cacheLsn2eALib.s - MIPS Loongson2e cache management assembly routines */

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
01q,18jan02,agf  add explicit align directive to data section(s)
01p,06nov01,sru  reorder icache reset cache ops
01p,02aug01,mem  Diab integration
01o,16jul01,ros  add CofE comment
01n,09jul01,sru  Clear branch history buffer during IC invalidation.
01m,12feb01,sru  Adding HAZARD macros
01l,22nov00,mem  Added cacheLsn2eRomTextUpdate
01k,22jun00,dra  Added cache sync support, updated virt addr flushing.
01j,16jun00,dra  update for idts134
01i,14jun00,dra  generalize the mmuMipsUnMap flush of virtual page addresses
                 to all Mips architectures
01h,27mar00,dra  Added missing global decls.
01g,22mar00,dra  Moved cache size variables here.
01f,28feb00,dra  Add GTEXT, GDATA, FUNC macros to support omfDll & loader
01e,31jan00,dra  Suppress compiler warnings.
01d,31dec96,kkk  allow cache enable/disable for R4650.
01c,18apr96,rml  guarantee reset of cache policy reg for 4650
01b,24jan94,cd   Corrected cache initialisation for Orion/R4600 
01a,01oct93,cd   created.
*/

/*
DESCRIPTION
This library contains MIPS Loongson2e cache set-up and invalidation routines
written in assembly language.  The Loongson2e utilizes a variable-size
instruction and data cache that operates in write-back mode.  Cache
line size also varies. See also the manual entry for cacheLsn2eLib.

For general information about caching, see the manual entry for cacheLib.

INCLUDE FILES: cacheLib.h

SEE ALSO: cacheLsn2eLib, cacheLib
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "arch/mips/archMips.h"

/*
 * MIPS General Cache operations
 */
#undef Index_Invalidate_I               /*#0x0          0       0 */
#undef Index_Writeback_Inv_D            /*#0x1          0       1 */
#undef Index_Invalidate_SI              /*#0x2          0       2 */
#undef Index_Writeback_Inv_SD           /*#0x3          0       3 */
#undef Index_Load_Tag_I                 /*#0x4          1       0 */
#undef Index_Load_Tag_D                 /*#0x5          1       1 */
#undef Index_Load_Tag_SI                /*#0x6          1       2 */
#undef Index_Load_Tag_SD                /*#0x7          1       3 */
#undef Index_Store_Tag_I                /*#0x8          2       0 */
#undef Index_Store_Tag_D                /*#0x9          2       1 */
#undef Index_Store_Tag_SI               /*#0xA          2       2 */
#undef Index_Store_Tag_SD               /*#0xB          2       3 */
#undef Create_Dirty_Exc_D               /*#0xD          3       1 */
#undef Create_Dirty_Exc_SD              /*#0xF          3       3 */
#undef Hit_Invalidate_I                 /*#0x10         4       0 */
#undef Hit_Invalidate_D                 /*#0x11         4       1 */
#undef Hit_Invalidate_SI                /*#0x12         4       2 */
#undef Hit_Invalidate_SD                /*#0x13         4       3 */
#undef Hit_Writeback_Inv_D              /*#0x15         5       1 */
#undef Hit_Writeback_Inv_SD             /*#0x17         5       3 */
#undef Fill_I                           /*#0x14         5       0 */
#undef Hit_Writeback_D                  /* #0x19        6       1 */
#undef Hit_Writeback_SD                 /*#0x1B         6       3 */
#undef Hit_Writeback_I                  /*#0x18         6       0 */
#undef Lock_I				            /*#0x1C	        7	    0 */
#undef Lock_D				            /*#0x1D	        7	    1 */
#undef Hit_Set_Virtual_SI               /*#0x1E         7       2 */
#undef Hit_Set_Virtual_SD               /*#0x1F         7       3 */

/*
 * Loongson2e defined Cache operations
 */
#define	Index_Invalidate_I              0x0         /* 0       0 */
#define	Index_Writeback_Inv_D           0x1         /* 0       1 */
#define	Hit_Invalidate_I                0x10        /* 4       0 */
#define	Hit_Invalidate_D                0x11        /* 4       1 */
#define	Hit_Writeback_Inv_D             0x15        /* 5       1 */

#define	Index_Writeback_Inv_S           0x3         /* 0       3 */
#define	Hit_Invalidate_S                0x13        /* 4       3 */
#define	Hit_Writeback_Inv_S             0x17        /* 5       3 */

#define Index_Store_Tag_S               0xB         /* 2       3 */

/** !NOTE:GODSON2 has 4 way icache, but when using indexed cache op, 
 *  one op will act on all 4 ways
**/

/* defines */
/*
 * cacheop macro to automate cache operations
 * first some helpers...
 */
#define _mincache(size, maxsize) \
	bltu	size,maxsize,9f     ;\
	move	size,maxsize        ;\
9:

#define _align(minaddr, maxaddr, linesize) \
	.set noat             ; \
	subu	AT,linesize,1 ;	\
	not	AT                ; \
	and	minaddr,AT        ; \
	addu	maxaddr,-1    ; \
	and	maxaddr,AT        ; \
	.set at

/* general operations */
#define doop1(op1) \
	cache	op1,0(a0)	; \
	HAZARD_CACHE
#define doop2(op1, op2) \
	cache	op1,0(a0)	; \
	HAZARD_CACHE		; \
	cache	op2,0(a0)	; \
	HAZARD_CACHE

/* Loop operation for 4-way set */ 
/* associative index operations   */
#define doop10(op1) \
	cache	op1,0(a0)	;\
	HAZARD_CACHE		;\
	cache	op1,1(a0)	;\
	HAZARD_CACHE        ;\
	cache	op1,2(a0)	;\
	HAZARD_CACHE		;\
	cache	op1,3(a0)	;\
	HAZARD_CACHE

/* specials for cache initialisation            */
/* All cache initialization is done by index ops*/
/* 4-way set associativity is considered here */
#define doop1lw(op1) \
	lw	zero,0(a0)
#define doop1lw1(op1) \
	cache	op1,0(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,1(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,2(a0)  ; \
	HAZARD_CACHE	   ; \
	cache   op1,3(a0)  ; \
	lw	zero,0(a0)     ; \
	cache	op1,0(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,1(a0)  ; \
	HAZARD_CACHE       ; \
	cache	op1,2(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,3(a0)  ; \
	HAZARD_CACHE
#define doop121(op1,op2) \
	cache	op1,0(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,1(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,2(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,3(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op2,0(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op2,1(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op2,2(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op2,3(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,0(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,1(a0)  ; \
	HAZARD_CACHE       ; \
	cache	op1,2(a0)  ; \
	HAZARD_CACHE	   ; \
	cache	op1,3(a0)  ; \
	HAZARD_CACHE


#define _oploopn(minaddr, maxaddr, linesize, tag, ops) \
	.set	noreorder ;		\
10: 	doop##tag##ops ;	\
	bne     minaddr,maxaddr,10b ;	\
	add   	minaddr,linesize ;	\
	.set	reorder

/* finally the cache operation macros */
#define vcacheopn(kva, n, cacheSize, cacheLineSize, tag, ops) \
 	blez	n,11f ;			\
	addu	n,kva ;			\
	_align(kva, n, cacheLineSize) ; \
	_oploopn(kva, n, cacheLineSize, tag, ops) ; \
11:

#define icacheopn(kva, n, cacheSize, cacheLineSize, tag, ops) \
	_mincache(n, cacheSize);	\
 	blez	n,11f ;			\
	addu	n,kva ;			\
	_align(kva, n, cacheLineSize) ; \
	_oploopn(kva, n, cacheLineSize, tag, ops) ; \
11:

/* Cache macro for 4-way set associative cache index operations */
#define i10cacheopn(kva, n, cacheSize, cacheLineSize, tag, ops) \
	srl	cacheSize,2	; \
	_mincache(n, cacheSize)	; \
	blez	n,11f		; \
	addu	n,kva		; \
	_align(kva, n, cacheLineSize); \
	_oploopn(kva, n, cacheLineSize, tag, ops); \
11:
	
#define vcacheop(kva, n, cacheSize, cacheLineSize, op) \
	vcacheopn(kva, n, cacheSize, cacheLineSize, 1, (op))

#define icacheop(kva, n, cacheSize, cacheLineSize, op) \
	icacheopn(kva, n, cacheSize, cacheLineSize, 1, (op))

#define i10cacheop(kva, n, cacheSize, cacheLineSize, op) \
	i10cacheopn(kva, n, cacheSize, cacheLineSize, 10, (op))


	.globl GTEXT(cacheLsn2eReset)		/* low level cache init */
	.globl GTEXT(cacheLsn2eEnable)       
	.globl GTEXT(cacheLsn2eDisable)
	.globl GTEXT(cacheLsn2eRomTextUpdate)	/* cache-text-update */
    .globl GTEXT(cacheLsn2eFlushInvalidateAll) /* flush entire cache */
	.globl GTEXT(cacheLsn2eDCFlushInvalidateAll) /* flush entire data cache */
	.globl GTEXT(cacheLsn2eDCFlushInvalidate)/* flush data cache locations */
	.globl GTEXT(cacheLsn2eDCInvalidateAll)	/* invalidate entire d cache */
	.globl GTEXT(cacheLsn2eDCInvalidate)	/* invalidate d cache locations */
	.globl GTEXT(cacheLsn2eICInvalidateAll)	/* invalidate i cache locations */
	.globl GTEXT(cacheLsn2eICInvalidate)	/* invalidate i cache locations */
#if 0
	.globl GDATA(cacheLsn2eDCacheSize)	/* data cache size */
	.globl GDATA(cacheLsn2eICacheSize)	/* inst. cache size */
	.globl GDATA(cacheLsn2eSCacheSize)	/* secondary cache size */

	.globl GDATA(cacheLsn2eDCacheLineSize)    /* data cache line size */
	.globl GDATA(cacheLsn2eICacheLineSize)    /* inst. cache line size */
	.globl GDATA(cacheLsn2eSCacheLineSize)    /* secondary cache line size */
#endif
	.globl GTEXT(cacheLsn2eVirtPageFlush)	/* flush cache on MMU page unmap */
	.globl GTEXT(cacheLsn2eSync)		/* cache sync operation */

#if 0	
	.data
	.align	4
cacheLsn2eICacheSize:
	.word	0
cacheLsn2eDCacheSize:
	.word	0
cacheLsn2eSCacheSize:
	.word	0
cacheLsn2eICacheLineSize:
	.word	0
cacheLsn2eDCacheLineSize:
	.word	0
cacheLsn2eSCacheLineSize:
	.word	0
#endif
	.text
	.set	reorder

/*******************************************************************************
*
* cacheLsn2eReset - low level initialisation of the Loongson2e primary caches
*
* This routine initialises the Loongson2e primary caches to ensure that they
* have good parity.  It must be called by the ROM before any cached locations
* are used to prevent the possibility of data with bad parity being written to
* memory.
* To initialise the instruction cache it is essential that a source of data
* with good parity is available. If the initMem argument is set, this routine
* will initialise an area of memory starting at location zero to be used as
* a source of parity; otherwise it is assumed that memory has been
* initialised and already has good parity.
*
* RETURNS: N/A
*

* void cacheLsn2eReset (initMem)

*/
	.ent	cacheLsn2eReset
FUNC_LABEL(cacheLsn2eReset)
	
	/* disable all i/u and cache exceptions */
resetcache:
	mfc0	v0,C0_SR
	HAZARD_CP_READ
	and	v1,v0,SR_BEV
	or	v1,SR_DE
	mtc0	v1,C0_SR


	/* Invalidate icache tags */
	li	a0,K0BASE
	move	a2,t2		# icacheSize
	move	a3,t4		# icacheLineSize
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Invalidate_I)

	/* Invalidate dcache tags */
	li	a0,K0BASE
	move	a2,t3		# dcacheSize
	move	a3,t5		# dcacheLineSize
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_D)

	/* FIXME assumes unified I & D in scache */
	/* set invalid tag */
	mtc0	zero,C0_TAGLO
	mtc0	zero,C0_TAGHI
	HAZARD_CACHE_TAG

	
	li	a0,K0BASE
	move	a2,t6
	move	a3,t7
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Store_Tag_S)

	mtc0	v0,C0_SR
	HAZARD_CP_WRITE

	j	ra
	.end	cacheLsn2eReset

	.ent cacheLsn2eEnable
FUNC_LABEL(cacheLsn2eEnable)
	mfc0 v0,C0_CONFIG
	HAZARD_CP_READ
	and v0,0xfffffff8
	ori v0,0x3
	mtc0 v0,C0_CONFIG
	HAZARD_CP_WRITE

	j ra
	.end cacheLsn2eEnable

	.ent cacheLsn2eDisable
FUNC_LABEL(cacheLsn2eDisable)
	mfc0 v0,C0_CONFIG
	HAZARD_CP_READ
	and v0,0xfffffff8
	ori v0,0x2
	mtc0 v0,C0_CONFIG
	HAZARD_CP_WRITE

	j ra
	.end cacheLsn2eDisable
/******************************************************************************
*
* cacheLsn2eRomTextUpdate - cache text update like functionality from the bootApp
*
*	a0	i-cache size
*	a1	i-cache line size
*	a2	d-cache size
*	a3	d-cache line size
*
* RETURNS: N/A
*

* void cacheLsn2eRomTextUpdate ()

*/
	.ent	cacheLsn2eRomTextUpdate
FUNC_LABEL(cacheLsn2eRomTextUpdate)
	/* Save I-cache parameters */
	move	t0,a0
	move	t1,a1

	/* Check for primary data cache */
	blez	a2,99f

	/* Flush-invalidate primary data cache */
	li	a0,K0BASE
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_D)
99:
	/* replace I-cache parameters */
	move	a2,t0
	move	a3,t1
	
	/* Check for primary instruction cache */
	blez	a0,99f
	
	/* Invalidate primary instruction cache */
	li	a0,K0BASE
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Invalidate_I)
99:
	j	ra
	.end	cacheLsn2eRomTextUpdate

/*******************************************************************************
*
* cacheLsn2eFlushInvalidateAll - flush entire Loongson2e cache:icache,dcache,scache
*
* RETURNS: N/A
*

* void cacheLsn2eFlushInvalidateAll (void)

*/
	.ent	cacheLsn2eFlushInvalidateAll
FUNC_LABEL(cacheLsn2eFlushInvalidateAll)

	li	a2,(512<<10)
	li	a3,32
	li	a0,K0BASE
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_S)

	j	ra

	.end	cacheLsn2eFlushInvalidateAll

/*******************************************************************************
*
* cacheLsn2eDCFlushInvalidateAll - flush entire Loongson2e data cache
*
* RETURNS: N/A
*

* void cacheLsn2eDCFlushInvalidateAll (void)

*/
	.ent	cacheLsn2eDCFlushInvalidateAll
FUNC_LABEL(cacheLsn2eDCFlushInvalidateAll)

	li	a2,(512<<10)
	li	a3,32
	li	a0,K0BASE
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_S)

	j	ra

	.end	cacheLsn2eDCFlushInvalidateAll

/*******************************************************************************
*
* cacheLsn2eDCFlush - flush Loongson2e data cache locations
*
* RETURNS: N/A
*

* void cacheLsn2eDCFlushInvalidate
*     (
*     baseAddr,		/@ virtual address @/
*     byteCount		/@ number of bytes to invalidate @/
*     )

*/	
	.ent	cacheLsn2eDCFlushInvalidate
FUNC_LABEL(cacheLsn2eDCFlushInvalidate)
	
	li	a2, (512<<10)
	li	a3, 32
	vcacheop(a0,a1,a2,a3,Hit_Writeback_Inv_S)	
	j	ra
	.end	cacheLsn2eDCFlushInvalidate


/*******************************************************************************
*
* cacheLsn2eDCInvalidateAll - invalidate entire Loongson2e data cache
*
* RETURNS: N/A
*

* void cacheLsn2eDCInvalidateAll (void)

*/
	.ent	cacheLsn2eDCInvalidateAll
FUNC_LABEL(cacheLsn2eDCInvalidateAll)

    li	a0,K0BASE
	li	a2,(512<<10)
	li	a3,32
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_S)

	j	ra

	.end	cacheLsn2eDCInvalidateAll

/*******************************************************************************
*
* cacheLsn2eDCInvalidate - invalidate Loongson2e data cache locations
*
* RETURNS: N/A
*

* void cacheLsn2eDCInvalidate
*     (
*     baseAddr,		/@ virtual address @/
*     byteCount		/@ number of bytes to invalidate @/
*     )

*/
	.ent	cacheLsn2eDCInvalidate
FUNC_LABEL(cacheLsn2eDCInvalidate)

	li	a2,(512<<10)
	li	a3,32
	vcacheop(a0,a1,a2,a3,Hit_Invalidate_S)

	j	ra
	.end	cacheLsn2eDCInvalidate

/*******************************************************************************
*
* cacheLsn2eICInvalidateAll - invalidate entire Loongson2e instruction cache
*
* RETURNS: N/A
*

* void cacheLsn2eICInvalidateAll (void)

*/
	.ent	cacheLsn2eICInvalidateAll
FUNC_LABEL(cacheLsn2eICInvalidateAll)
	li	a0,K0BASE
	li	a2,(512<<10)
	li	a3,32
	move	a1,a2
	i10cacheop(a0,a1,a2,a3,Index_Writeback_Inv_S)

	j	ra

	.end	cacheLsn2eICInvalidateAll

/*******************************************************************************
*
* cacheLsn2eICInvalidate - invalidate Loongson2e data cache locations
*
* RETURNS: N/A
*

* void cacheLsn2eICInvalidate
*     (
*     baseAddr,		/@ virtual address @/
*     byteCount		/@ number of bytes to invalidate @/
*     )

*/
	.ent	cacheLsn2eICInvalidate
FUNC_LABEL(cacheLsn2eICInvalidate)
	li	a2,(512<<10)
	li	a3,32
	vcacheop(a0,a1,a2,a3,Hit_Invalidate_S)
	
	j	ra
	.end	cacheLsn2eICInvalidate

/******************************************************************************
*
* cacheLsn2eVirtPageFlush - flush one page of virtual addresses from caches
*
* Change ASID, flush the appropriate cache lines from the D- and I-cache,
* and restore the original ASID.
*
* CAVEAT: This routine and the routines it calls MAY be running to clear
* cache for an ASID which is only partially mapped by the MMU. For that
* reason, the caller may want to lock interrupts.
*
* RETURNS: N/A
*
* void cacheLsn2eVirtPageFlush (UINT asid, void *vAddr, UINT pageSize);
*/
	.ent	cacheLsn2eVirtPageFlush
FUNC_LABEL(cacheLsn2eVirtPageFlush)
	/* Save parameters */
	move	t4,a0			/* ASID to flush */
	move	t0,a1			/* beginning VA */
	move	t1,a2			/* length */

	/*
	 * When we change ASIDs, our stack might get unmapped,
	 * so use the stack now to free up some registers for use:
	 *	t0 - virtual base address of page to flush
	 *	t1 - page size
	 *	t2 - original SR
	 *	t3 - original ASID
	 *	t4 - ASID to flush
	 */

	/* lock interrupts */

	mfc0	t2, C0_SR
	HAZARD_CP_READ	
	li	t3, ~SR_INT_ENABLE
	and	t3, t2
	mtc0	t3, C0_SR
	HAZARD_INTERRUPT

	/* change the current ASID to context where page is mapped */

	mfc0	t3, C0_TLBHI		/* read current TLBHI */
	HAZARD_CP_READ
	and	t3, 0xff		/* extract ASID field */
	beq	t3, t4, 0f		/* branch if no need to change */
	mtc0	t4, C0_TLBHI		/* Store new EntryHi  */	
	HAZARD_TLB
0:
	/* clear the virtual addresses from D- and I-caches */
	
	li	a2,(512<<10)

	/* Flush-invalidate primary data cache */
	move	a0, t0
	move	a1, t1
	li	a3,32
	vcacheop(a0,a1,a2,a3,Hit_Writeback_Inv_S)

	
	/* restore the original ASID */
	mtc0	t3, C0_TLBHI		/* Restore old EntryHi  */	
	HAZARD_TLB

	mtc0	t2, C0_SR		/* restore interrupts */
	
	j	ra
	.end	cacheLsn2eVirtPageFlush

/******************************************************************************
*
* cacheLsn2eSync - sync region of memory through all caches
*
* RETURNS: N/A
*
* void cacheLsn2eSync (void *vAddr, UINT pageSize);
*/
	.ent	cacheLsn2eSync
FUNC_LABEL(cacheLsn2eSync)
	/* Save parameters */
	move	t0,a0			/* beginning VA */
	move	t1,a1			/* length */

	/* lock interrupts */

	mfc0	t2, C0_SR
	HAZARD_CP_READ
	li	t3, ~SR_INT_ENABLE
	and	t3, t2
	mtc0	t3, C0_SR
	HAZARD_INTERRUPT

	/*
	 * starting with primary caches, push the memory
	 * block out completely
	 */
	sync

	li	 a2,(512<<10)
	move a0,t0
	move a1,t1
	li	 a3,32
	vcacheop(a0,a1,a2,a3,Hit_Writeback_Inv_S)	
1:
	mtc0	t2, C0_SR		/* restore interrupts */
	
	j	ra
	.end	cacheLsn2eSync
