# Makefile - make rules for bsp/config/lsn2ecpci
#
# Copyright 2001-2007 Loongson.Ict.Cas
#
# This file has been developed by Wangfuquan,Loongson.Ict.Cas
#
# modification history
# --------------------
# 2007-06-06 create
#
# DESCRIPTION
# This file contains rules for building VxWorks for the
# Loongson2e Single Board Computer.
#

CPU              = MIPS64
TOOL             = gnule

TGT_DIR=$(WIND_BASE)/target
include $(TGT_DIR)/h/make/defs.bsp
#include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
#include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.


TARGET_DIR       = lsn2ecpci_mips64le
VENDOR           = Loongson.Ict.Cas
BOARD            = Loongson2E-CPCI-00 #Loongson2E-CPCI Single Board Computer

#
# The constants ROM_TEXT_ADRS, ROM_SIZE, RAM_LOW_ADRS and
# RAM_HIGH_ADRS are defined in config.h and MakeSkel.
# All definitions for these constants must be identical.
#

ROM_TEXT_ADRS    	= bfc00000	# ROM entry address
ROM_SIZE         	= 00080000	# number of bytes of ROM space

RAM_LOW_ADRS     	= 80200000	# RAM text/data address
RAM_HIGH_ADRS    	= 80400000	# RAM text/data address

MACH_EXTRA          = gei82543End.o i8237Dma.o ns16550Sio.o  simpleprintf.o cs5536_vsm.o mod_ops-lm2f.o \
					miiLib.o ataDrv.o ataShow.o gpio.o watchDog.o uart.o flash_sst.o updateBIOS.o

##MACH_EXTRA          += x86emu\x86emu.a
##EXTRA_DEFINE		+= -D${BOARD}  

USRCONFIG			 = usrConfig.c
#BOOTCONFIG			 = usrConfig.c

CPU_VARIANT	= _lsn2e

## Only redefine make definitions above this point, or the expansion of
## makefile target dependencies may be incorrect.

x86emu\x86emu.a: x86emu.a
include x86emu\Makefile
include $(TGT_DIR)/h/make/rules.bsp
#include $(TGT_DIR)/h/make/rules.$(WIND_HOST_TYPE)
