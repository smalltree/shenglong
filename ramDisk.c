#include "vxWorks.h"
#include "stdlib.h"
#include "semLib.h"
#include "ioLib.h"
#include "string.h"
#include "errno.h"
#include "assert.h"

#include "private/dosFsVerP.h"
/*#include "private/cbioLibP.h" /* only CBIO modules may include this file */

#include "ramDrv.h"
#include "dosFsLib.h"
#include "stdio.h"

STATUS usrRamDiskInit ()
{
	BLK_DEV *pBlkDev;
	DOS_VOL_DESC *pVolDesc;
	/* create a ram disk */
	pBlkDev=ramDevCreate(0, 512, 400, 200000, 0);
	if(pBlkDev==NULL){
		printf("ran disk create error!!\n");
		printErrno();
		return ERROR;
	}
	/* Create Ram Disk Device */
	/*if(dosFsDevCreate("/ram0", pBlkDev, 4, NULL)==NULL){
		printf("create ramdisk error!!!\n");
		printErrno();
		return ERROR;
	}*/
	/* Format dosFs */
	/*if(dosFsVolFormat(pBlkDev, DOS_OPT_BLANK | DOS_OPT_QUIET | DOS_OPT_FAT32, NULL)==NULL){
		printf("Format dosFs error!!!\n");
		printErrno();
		return ERROR;
	}*/
	pVolDesc=dosFsMkfs("/ram0",pBlkDev);
	
	return OK;
}