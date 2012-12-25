/* gt64240.h - Galileo gt64240 bridge device header */

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
01b,20nov02,zmm  Add timer registers. Cleanup and fix comments.
01a,19sep02,zmm  Written.
*/

/*
 * This file contains register offsets and programming values for the
 * GT64240 bridge chip, which appears on the Ocelot-G evaluation board.
 */

#ifndef __INCgt64240h
#define __INCgt64240h

#ifdef __cplusplus
extern "C" {
#endif

/* CPU Address Decode Register Map */

#define SCS0_LOW_DECODE_ADDRESS              0x008
#define SCS0_HIGH_DECODE_ADDRESS             0x010
#define SCS1_LOW_DECODE_ADDRESS              0x208
#define SCS1_HIGH_DECODE_ADDRESS             0x210
#define SCS2_LOW_DECODE_ADDRESS              0x018
#define SCS2_HIGH_DECODE_ADDRESS             0x020
#define SCS3_LOW_DECODE_ADDRESS              0x218
#define SCS3_HIGH_DECODE_ADDRESS             0x220
#define CS0_LOW_DECODE_ADDRESS               0x028
#define CS0_HIGH_DECODE_ADDRESS              0x030
#define CS1_LOW_DECODE_ADDRESS               0x228
#define CS1_HIGH_DECODE_ADDRESS              0x230
#define CS2_LOW_DECODE_ADDRESS               0x248
#define CS2_HIGH_DECODE_ADDRESS              0x250
#define CS3_LOW_DECODE_ADDRESS               0x038
#define CS3_HIGH_DECODE_ADDRESS              0x040
#define BOOT_CS_LOW_DECODE_ADDRESS           0x238
#define BOOT_CS_HIGH_DECODE_ADDRESS          0x240
#define PCI0_IO_LOW_DECODE_ADDRESS           0x048
#define PCI0_IO_HIGH_DECODE_ADDRESS          0x050
#define PCI0_MEMORY0_LOW_DECODE_ADDRESS      0x058
#define PCI0_MEMORY0_HIGH_DECODE_ADDRESS     0x060
#define PCI0_MEMORY1_LOW_DECODE_ADDRESS      0x080
#define PCI0_MEMORY1_HIGH_DECODE_ADDRESS     0x088
#define PCI0_MEMORY2_LOW_DECODE_ADDRESS      0x258
#define PCI0_MEMORY2_HIGH_DECODE_ADDRESS     0x260
#define PCI0_MEMORY3_LOW_DECODE_ADDRESS      0x280
#define PCI0_MEMORY3_HIGH_DECODE_ADDRESS     0x288
#define PCI1_IO_LOW_DECODE_ADDRESS           0x090
#define PCI1_IO_HIGH_DECODE_ADDRESS          0x098
#define PCI1_MEMORY0_LOW_DECODE_ADDRESS      0x0a0
#define PCI1_MEMORY0_HIGH_DECODE_ADDRESS     0x0a8
#define PCI1_MEMORY1_LOW_DECODE_ADDRESS      0x0b0
#define PCI1_MEMORY1_HIGH_DECODE_ADDRESS     0x0b8
#define PCI1_MEMORY2_LOW_DECODE_ADDRESS      0x2a0
#define PCI1_MEMORY2_HIGH_DECODE_ADDRESS     0x2a8
#define PCI1_MEMORY3_LOW_DECODE_ADDRESS      0x2b0
#define PCI1_MEMORY3_HIGH_DECODE_ADDRESS     0x2b8
#define INTERNAL_SPACE_DECODE_ADDRESS        0x068
#define PCI0_IO_ADDRESS_REMAP                0x0f0
#define PCI0_MEMORY0_REMAP_LOW               0x0f8
#define PCI0_MEMORY0_REMAP_HIGH              0x320
#define PCI0_MEMORY1_REMAP_LOW               0x100
#define PCI0_MEMORY1_REMAP_HIGH              0x328
#define PCI0_MEMORY2_REMAP_LOW               0x2f8
#define PCI0_MEMORY2_REMAP_HIGH              0x330
#define PCI0_MEMORY3_REMAP_LOW               0x300
#define PCI0_MEMORY3_REMAP_HIGH              0x338
#define PCI1_IO_REMAP                        0x108
#define PCI1_MEMORY0_REMAP_LOW               0x110
#define PCI1_MEMORY0_REMAP_HIGH              0x340
#define PCI1_MEMORY1_REMAP_LOW               0x118
#define PCI1_MEMORY1_REMAP_HIGH              0x348
#define PCI1_MEMORY2_REMAP_LOW               0x310
#define PCI1_MEMORY2_REMAP_HIGH              0x350
#define PCI1_MEMORY3_REMAP_LOW               0x318
#define PCI1_MEMORY3_REMAP_HIGH              0x358

/* CPU Control Register Map */

#define CPU_CONFIGURATION                    0x000
#define CPU_MODE                             0x120
#define CPU_READ_RESPONSE_CROSS_CTRL_LOW     0x170
#define CPU_READ_RESPONSE_CROSS_CTRL_HIGH    0x178

/* CPU Sync Barrier Register Map */

#define CPU_PCI0_SYNC_BARRIER_VIRTUAL_REG    0x0c0
#define CPU_PCI1_SYNC_BARRIER_VIRTUAL_REG    0x0c8

/* CPU Access Protection Register Map */

#define PROTECT_LOW_ADDRESS_0                0x180
#define PROTECT_HIGH_ADDRESS_0               0x188
#define PROTECT_LOW_ADDRESS_1                0x190
#define PROTECT_HIGH_ADDRESS_1               0x198
#define PROTECT_LOW_ADDRESS_2                0x1a0
#define PROTECT_HIGH_ADDRESS_2               0x1a8
#define PROTECT_LOW_ADDRESS_3                0x1b0
#define PROTECT_HIGH_ADDRESS_3               0x1b8
#define PROTECT_LOW_ADDRESS_4                0x1c0
#define PROTECT_HIGH_ADDRESS_4               0x1c8
#define PROTECT_LOW_ADDRESS_5                0x1d0
#define PROTECT_HIGH_ADDRESS_5               0x1d8
#define PROTECT_LOW_ADDRESS_6                0x1e0
#define PROTECT_HIGH_ADDRESS_6               0x1e8
#define PROTECT_LOW_ADDRESS_7                0x1f0
#define PROTECT_HIGH_ADDRESS_7               0x1f8

/* CPU Error Report Register Map */

#define CPU_ERROR_ADDRESS_LOW                0x070
#define CPU_ERROR_ADDRESS_HIGH               0x078
#define CPU_ERROR_DATA_LOW                   0x128
#define CPU_ERROR_DATA_HIGH                  0x130
#define CPU_ERROR_PARITY                     0x138
#define CPU_ERROR_CAUSE                      0x140
#define CPU_ERROR_MASK                       0x148

/* SDRAM Configuration Register Map */

#define SDRAM_CONFIGURATION                  0x448
#define SDRAM_OPERATION_MODE                 0x474
#define SDRAM_ADDRESS_CONTROL                0x47c
#define SDRAM_TIMING_PARAMETERS              0x4b4
#define SDRAM_UMA_CONTROL                    0x4a4
#define SDRAM_INTERFACE_CROSSBAR_CTRL_LOW    0x4a8
#define SDRAM_INTERFACE_CROSSBAR_CTRL_HIGH   0x4ac
#define SDRAM_INTERFACE_CROSSBAR_TIMEOUT     0x4b0

/* SDRAM Banks Parameters Register Map */

#define SDRAM_BANK0_PARAMETERS               0x44c
#define SDRAM_BANK1_PARAMETERS               0x450
#define SDRAM_BANK2_PARAMETERS               0x454
#define SDRAM_BANK3_PARAMETERS               0x458

/* SDRAM Error Report Register Map */

#define SDRAM_ERROR_DATA_LOW                 0x484
#define SDRAM_ERROR_DATA_HIGH                0x480
#define SDRAM_ERROR_ADDRESS                  0x490
#define SDRAM_RECEIVED_ECC                   0x488
#define SDRAM_CALCULATED_ECC                 0x48c
#define SDRAM_ECC_CONTROL                    0x494
#define SDRAM_ECC_ERROR_COUNTER              0x498

/* Device Control Register Map */

#define DEVICE_BANK0_PARAMETERS              0x45c
#define DEVICE_BANK1_PARAMETERS              0x460
#define DEVICE_BANK2_PARAMETERS              0x464
#define DEVICE_BANK3_PARAMETERS              0x468
#define BOOT_DEVICE_PARAMETERS               0x46c
#define DEVICE_INTERFACE_CONTROL             0x4c0
#define DEVICE_INTERFACE_CROSSBAR_CTRL_LOW   0x4c8
#define DEVICE_INTERFACE_CROSSBAR_CTRL_HIGH  0x4cc
#define DEVICE_INTERFACE_CROSSBAR_TIMEOUT    0x4c4

/* Device Interrupts Register Map */

#define DEVICE_INTERRUPT_CAUSE               0x4d0
#define DEVICE_INTERRUPT_MASK                0x4d4
#define DEVICE_ERROR_ADDRESS                 0x4d8

/* PCI 0 Controller Register Map */

#define PCI0_SCS0_BAR_SIZE                   0xc08
#define PCI0_SCS1_BAR_SIZE                   0xd08
#define PCI0_SCS2_BAR_SIZE                   0xc0c
#define PCI0_SCS3_BAR_SIZE                   0xd0c
#define PCI0_CS0_BAR_SIZE                    0xc10
#define PCI0_CS1_BAR_SIZE                    0xd10
#define PCI0_CS2_BAR_SIZE                    0xd18
#define PCI0_CS3_BAR_SIZE                    0xc14
#define PCI0_BOOT_CS_BAR_SIZE                0xd14
#define PCI0_P2P_MEM0_BAR_SIZE               0xd1c
#define PCI0_P2P_MEM1_BAR_SIZE               0xd20
#define PCI0_P2P_IO_BAR_SIZE                 0xd24
#define PCI0_DAC_SCS0_BAR_SIZE               0xe00
#define PCI0_DAC_SCS1_BAR_SIZE               0xe04
#define PCI0_DAC_SCS2_BAR_SIZE               0xe08
#define PCI0_DAC_SCS3_BAR_SIZE               0xe0c
#define PCI0_DAC_CS0_BAR_SIZE                0xe10
#define PCI0_DAC_CS1_BAR_SIZE                0xe14
#define PCI0_DAC_CS2_BAR_SIZE                0xe18
#define PCI0_DAC_CS3_BAR_SIZE                0xe1c
#define PCI0_DAC_BOOT_CS_BAR_SIZE            0xe20
#define PCI0_DAC_P2P_MEM0_BAR_SIZE           0xe24
#define PCI0_DAC_P2P_MEM1_BAR_SIZE           0xe28
#define PCI0_EXPANSION_ROM_BAR_SIZE          0xd2c
#define PCI0_BASE_ADDR_REGS_ENABLE           0xc3c
#define PCI0_SCS0_BASE_ADDR_REMAP            0xc48
#define PCI0_SCS1_BASE_ADDR_REMAP            0xd48
#define PCI0_SCS2_BASE_ADDR_REMAP            0xc4c
#define PCI0_SCS3_BASE_ADDR_REMAP            0xd4c
#define PCI0_CS0_BASE_ADDR_REMAP             0xc50
#define PCI0_CS1_BASE_ADDR_REMAP             0xd50
#define PCI0_CS2_BASE_ADDR_REMAP             0xd58
#define PCI0_CS3_ADDR_REMAP                  0xc54
#define PCI0_BOOT_CS_ADDR_REMAP              0xd54
#define PCI0_P2P_MEM0_BASE_ADDR_REMAP_L      0xd5c
#define PCI0_P2P_MEM0_BASE_ADDR_REMAP_H      0xd60
#define PCI0_P2P_MEM1_BASE_ADDR_REMAP_L      0xd64
#define PCI0_P2P_MEM1_BASE_ADDR_REMAP_H      0xd68
#define PCI0_P2P_IO_BASE_ADDR_REMAP          0xd6c
#define PCI0_DAC_SCS0_BASE_ADDR_REMAP        0xf00
#define PCI0_DAC_SCS1_BASE_ADDR_REMAP        0xf04
#define PCI0_DAC_SCS2_BASE_ADDR_REMAP        0xf08
#define PCI0_DAC_SCS3_BASE_ADDR_REMAP        0xf0C
#define PCI0_DAC_CS0_BASE_ADDR_REMAP         0xf10
#define PCI0_DAC_CS1_BASE_ADDR_REMAP         0xf14
#define PCI0_DAC_CS2_BASE_ADDR_REMAP         0xf18
#define PCI0_DAC_CS3_BASE_ADDR_REMAP         0xf1c
#define PCI0_DAC_BOOTCS_BASE_ADDR_REMAP      0xf20
#define PCI0_DAC_P2P_MEM0_BASE_ADDR_REMAP_L  0xf24
#define PCI0_DAC_P2P_MEM0_BASE_ADDR_REMAP_H  0xf28
#define PCI0_DAC_P2P_MEM1_BASE_ADDR_REMAP_L  0xf2c
#define PCI0_DAC_P2P_MEM1_BASE_ADDR_REMAP_H  0xf30
#define PCI0_EXPANSION_ROM_BASE_ADDR_REMAP   0xf38
#define PCI0_ADDR_DECODE_CONTROL             0xd3c
#define PCI0_COMMAND                         0xc00
#define PCI0_MODE                            0xd00
#define PCI0_TIMEOUT_RETRY                   0xc04
#define PCI0_READ_BUFFER_DISCARD_TIMER       0xd04
#define MSI0_TRIGGER_TIMER                   0xc38
#define PCI0_ARBITER_CONTROL                 0x1d00
#define PCI0_INTERFACE_CROSSBAR_CONTROL_L    0x1d08
#define PCI0_INTERFACE_CROSSBAR_CONTROL_H    0x1d0c
#define PCI0_INTERFACE_CROSSBAR_TIMEOUT      0x1d04
#define PCI0_READ_RESPONSE_CROSSBAR_CTRL_L   0x1d18
#define PCI0_READ_RESPONSE_CROSSBAR_CTRL_H   0x1d1c
#define PCI0_SYNC_BARRIER_VIRTUAL_REG        0x1d10
#define PCI0_P2P_CONFIG                      0x1d14
#define PCI0_P2P_SWAP_CONTROL                0x1d54
#define PCI0_ACCESS_CONTROL_BASE_0_L         0x1e00
#define PCI0_ACCESS_CONTROL_BASE_0_H         0x1e04
#define PCI0_ACCESS_CONTROL_TOP_0            0x1f08
#define PCI0_ACCESS_CONTROL_BASE_1_L         0x1e10
#define PCI0_ACCESS_CONTROL_BASE_1_H         0x1e14
#define PCI0_ACCESS_CONTROL_TOP_1            0x1e18
#define PCI0_ACCESS_CONTROL_BASE_2_L         0x1e20
#define PCI0_ACCESS_CONTROL_BASE_2_H         0x1e24
#define PCI0_ACCESS_CONTROL_TOP_2            0x1e28
#define PCI0_ACCESS_CONTROL_BASE_3_L         0x1e30
#define PCI0_ACCESS_CONTROL_BASE_3_H         0x1e34
#define PCI0_ACCESS_CONTROL_TOP_3            0x1e38
#define PCI0_ACCESS_CONTROL_BASE_4_L         0x1e40
#define PCI0_ACCESS_CONTROL_BASE_4_H         0x1e44
#define PCI0_ACCESS_CONTROL_TOP_4            0x1e48
#define PCI0_ACCESS_CONTROL_BASE_5_L         0x1e50
#define PCI0_ACCESS_CONTROL_BASE_5_H         0x1e54
#define PCI0_ACCESS_CONTROL_TOP_5            0x1e58
#define PCI0_ACCESS_CONTROL_BASE_6_L         0x1e60
#define PCI0_ACCESS_CONTROL_BASE_6_H         0x1e64
#define PCI0_ACCESS_CONTROL_TOP_6            0x1e68
#define PCI0_ACCESS_CONTROL_BASE_7_L         0x1e70
#define PCI0_ACCESS_CONTROL_BASE_7_H         0x1e74
#define PCI0_ACCESS_CONTROL_TOP_7            0x1e78
#define PCI0_CONFIG_ADDR                     0xcf8
#define PCI0_CONFIG_DATA_VIRTUAL_REG         0xcfc
#define PCI0_INTERRUPT_ACK_VIRTUAL_REG       0xc34
#define PCI0_SERR_MASK                       0xc28
#define PCI0_ERROR_ADDR_L                    0x1d40
#define PCI0_ERROR_ADDR_H                    0x1d44
#define PCI0_ERROR_DATA_L                    0x1d48
#define PCI0_ERROR_DATA_H                    0x1d4C
#define PCI0_ERROR_COMMAND                   0x1d50
#define PCI0_ERROR_CAUSE                     0x1d58
#define PCI0_ERROR_MASK                      0x1d5c

/* PCI 1 Controller Register Map */

#define PCI1_SCS0_BAR_SIZE                   0xc88
#define PCI1_SCS1_BAR_SIZE                   0xd88
#define PCI1_SCS2_BAR_SIZE                   0xc8c
#define PCI1_SCS3_BAR_SIZE                   0xd8c
#define PCI1_CS0_BAR_SIZE                    0xc90
#define PCI1_CS1_BAR_SIZE                    0xd90
#define PCI1_CS2_BAR_SIZE                    0xd98
#define PCI1_CS3_BAR_SIZE                    0xc94
#define PCI1_BOOT_CS_BAR_SIZE                0xd94
#define PCI1_P2P_MEM0_BAR_SIZE               0xd9c
#define PCI1_P2P_MEM1_BAR_SIZE               0xda0
#define PCI1_P2P_IO_BAR_SIZE                 0xda4
#define PCI1_DAC_SCS0_BAR_SIZE               0xe80
#define PCI1_DAC_SCS1_BAR_SIZE               0xe84
#define PCI1_DAC_SCS2_BAR_SIZE               0xe88
#define PCI1_DAC_SCS3_BAR_SIZE               0xe8c
#define PCI1_DAC_CS0_BAR_SIZE                0xe90
#define PCI1_DAC_CS1_BAR_SIZE                0xe94
#define PCI1_DAC_CS2_BAR_SIZE                0xe98
#define PCI1_DAC_CS3_BAR_SIZE                0xe9c
#define PCI1_DAC_BOOT_CS_BAR_SIZE            0xea0
#define PCI1_DAC_P2P_MEM0_BAR_SIZE           0xe94
#define PCI1_DAC_P2P_MEM1_BAR_SIZE           0xe98
#define PCI1_EXPANSION_ROM_BAR_SIZE          0xd9c
#define PCI1_BASE_ADDR_REGS_ENABLE           0xcbc
#define PCI1_SCS0_BASE_ADDR_REMAP            0xcc8
#define PCI1_SCS1_BASE_ADDR_REMAP            0xdc8
#define PCI1_SCS2_BASE_ADDR_REMAP            0xccc
#define PCI1_SCS3_BASE_ADDR_REMAP            0xdcc
#define PCI1_CS0_BASE_ADDR_REMAP             0xcd0
#define PCI1_CS1_BASE_ADDR_REMAP             0xdd0
#define PCI1_CS2_BASE_ADDR_REMAP             0xdd8
#define PCI1_CS3_ADDR_REMAP                  0xcd4
#define PCI1_BOOT_CS_ADDR_REMAP              0xdd4
#define PCI1_P2P_MEM0_BASE_ADDR_REMAP_L      0xddc
#define PCI1_P2P_MEM0_BASE_ADDR_REMAP_H      0xde0
#define PCI1_P2P_MEM1_BASE_ADDR_REMAP_L      0xde4
#define PCI1_P2P_MEM1_BASE_ADDR_REMAP_H      0xde8
#define PCI1_P2P_IO_BASE_ADDR_REMAP          0xdec
#define PCI1_DAC_SCS0_BASE_ADDR_REMAP        0xf80
#define PCI1_DAC_SCS1_BASE_ADDR_REMAP        0xf84
#define PCI1_DAC_SCS2_BASE_ADDR_REMAP        0xf88
#define PCI1_DAC_SCS3_BASE_ADDR_REMAP        0xf8C
#define PCI1_DAC_CS0_BASE_ADDR_REMAP         0xf90
#define PCI1_DAC_CS1_BASE_ADDR_REMAP         0xf94
#define PCI1_DAC_CS2_BASE_ADDR_REMAP         0xf98
#define PCI1_DAC_CS3_BASE_ADDR_REMAP         0xf9c
#define PCI1_DAC_BOOTCS_BASE_ADDR_REMAP      0xfa0
#define PCI1_DAC_P2P_MEM0_BASE_ADDR_REMAP_L  0xfa4
#define PCI1_DAC_P2P_MEM0_BASE_ADDR_REMAP_H  0xfa8
#define PCI1_DAC_P2P_MEM1_BASE_ADDR_REMAP_L  0xfac
#define PCI1_DAC_P2P_MEM1_BASE_ADDR_REMAP_H  0xfb0
#define PCI1_EXPANSION_ROM_BASE_ADDR_REMAP   0xfb8
#define PCI1_ADDR_DECODE_CONTROL             0xdbc
#define PCI1_COMMAND                         0xc80
#define PCI1_MODE                            0xd80
#define PCI1_TIMEOUT_RETRY                   0xc84
#define PCI1_READ_BUFFER_DISCARD_TIMER       0xd84
#define MSI1_TRIGGER_TIMER                   0xcb8
#define PCI1_ARBITER_CONTROL                 0x1d80
#define PCI1_INTERFACE_CROSSBAR_CONTROL_L    0x1d88
#define PCI1_INTERFACE_CROSSBAR_CONTROL_H    0x1d8c
#define PCI1_INTERFACE_CROSSBAR_TIMEOUT      0x1d84
#define PCI1_READ_RESPONSE_CROSSBAR_CTRL_L   0x1d98
#define PCI1_READ_RESPONSE_CROSSBAR_CTRL_H   0x1d9c
#define PCI1_SYNC_BARRIER_VIRTUAL_REG        0x1d90
#define PCI1_P2P_CONFIG                      0x1d94
#define PCI1_P2P_SWAP_CONTROL                0x1dd4
#define PCI1_ACCESS_CONTROL_BASE_0_L         0x1e80
#define PCI1_ACCESS_CONTROL_BASE_0_H         0x1e84
#define PCI1_ACCESS_CONTROL_TOP_0            0x1f88
#define PCI1_ACCESS_CONTROL_BASE_1_L         0x1e80
#define PCI1_ACCESS_CONTROL_BASE_1_H         0x1e94
#define PCI1_ACCESS_CONTROL_TOP_1            0x1e98
#define PCI1_ACCESS_CONTROL_BASE_2_L         0x1ea0
#define PCI1_ACCESS_CONTROL_BASE_2_H         0x1ea4
#define PCI1_ACCESS_CONTROL_TOP_2            0x1ea8
#define PCI1_ACCESS_CONTROL_BASE_3_L         0x1eb0
#define PCI1_ACCESS_CONTROL_BASE_3_H         0x1eb4
#define PCI1_ACCESS_CONTROL_TOP_3            0x1eb8
#define PCI1_ACCESS_CONTROL_BASE_4_L         0x1ec0
#define PCI1_ACCESS_CONTROL_BASE_4_H         0x1ec4
#define PCI1_ACCESS_CONTROL_TOP_4            0x1ec8
#define PCI1_ACCESS_CONTROL_BASE_5_L         0x1ed0
#define PCI1_ACCESS_CONTROL_BASE_5_H         0x1ed4
#define PCI1_ACCESS_CONTROL_TOP_5            0x1ed8
#define PCI1_ACCESS_CONTROL_BASE_6_L         0x1ee0
#define PCI1_ACCESS_CONTROL_BASE_6_H         0x1ee4
#define PCI1_ACCESS_CONTROL_TOP_6            0x1ee8
#define PCI1_ACCESS_CONTROL_BASE_7_L         0x1ef0
#define PCI1_ACCESS_CONTROL_BASE_7_H         0x1ef4
#define PCI1_ACCESS_CONTROL_TOP_7            0x1ef8
#define PCI1_CONFIG_ADDR                     0xc78
#define PCI1_CONFIG_DATA_VIRTUAL_REG         0xc7c
#define PCI1_INTERRUPT_ACK_VIRTUAL_REG       0xcb4
#define PCI1_SERR_MASK                       0xca8
#define PCI1_ERROR_ADDR_L                    0x1dc0
#define PCI1_ERROR_ADDR_H                    0x1dc4
#define PCI1_ERROR_DATA_L                    0x1dc8
#define PCI1_ERROR_DATA_H                    0x1dcc
#define PCI1_ERROR_COMMAND                   0x1dd0
#define PCI1_ERROR_CAUSE                     0x1dd8
#define PCI1_ERROR_MASK                      0x1ddc

/* interrupt read Cpu Select Cause register and bit defines */

#define GT_CPU_SELECT_CAUSE                  0xc70
#define GT_CPU_SELECT_CAUSE_STAT             0x80000000
#define GT_CPU_SELECT_CAUSE_SEL_HIGH         0x40000000

/* interrupt Cause register locations */

#define GT_INT_CAUSE_LOW                     0xc18
#define GT_INT_CAUSE_HIGH                    0xc68

/* interrupt Mask Register locations */

#define GT_CPU_INT_MASK_LOW                  0xc1c
#define GT_CPU_INT_MASK_HIGH                 0xc6c
#define GT_PCI0_INT_MASK_LOW                 0xc24
#define GT_PCI0_INT_MASK_HIGH                0xc64
#define GT_PCI1_INT_MASK_LOW                 0xca4
#define GT_PCI1_INT_MASK_HIGH                0xce4

/* Main Interrupt Cause (Low) */

#define GT_INT_SUM        0x00000001      /* bit  0 */
#define GT_INT_DEV        0x00000002      /* bit  1 */
#define GT_INT_DMA        0x00000004      /* bit  2 */
#define GT_INT_CPU        0x00000008      /* bit  3 */
#define GT_INT_IDMA0_1    0x00000010      /* bit  4 */
#define GT_INT_IDMA2_3    0x00000020      /* bit  5 */
#define GT_INT_IDMA4_5    0x00000040      /* bit  6 */
#define GT_INT_IDMA6_7    0x00000080      /* bit  7 */
#define GT_INT_TIMER0_1   0x00000100      /* bit  8 */
#define GT_INT_TIMER2_3   0x00000200      /* bit  9 */
#define GT_INT_TIMER4_5   0x00000400      /* bit  10 */
#define GT_INT_TIMER6_7   0x00000800      /* bit  11 */
#define GT_INT_PCI0_0     0x00001000      /* bit  12 */
#define GT_INT_PCI0_1     0x00002000      /* bit  13 */
#define GT_INT_PCI0_2     0x00004000      /* bit  14 */
#define GT_INT_PCI0_3     0x00008000      /* bit  15 */
#define GT_INT_PCI1_0     0x00010000      /* bit  16 */
#define GT_INT_ECC        0x00020000      /* bit  17 */
#define GT_INT_PCI1_1     0x00040000      /* bit  18 */
#define GT_INT_PCI1_2     0x00080000      /* bit  19 */
#define GT_INT_PCI1_3     0x00100000      /* bit  20 */
#define GT_INT_PCI0OUTL   0x00200000      /* bit  21 */
#define GT_INT_PCI0OUTH   0x00400000      /* bit  22 */
#define GT_INT_PCI1OUTL   0x00800000      /* bit  23 */
#define GT_INT_PCI1OUTH   0x01000000      /* bit  24 */
#define GT_INT_PCI0INL    0x04000000      /* bit  26 */
#define GT_INT_PCI0INH    0x08000000      /* bit  27 */
#define GT_INT_PCI1INL    0x10000000      /* bit  28 */
#define GT_INT_PCI1INH    0x20000000      /* bit  29 */

/* Main Interrupt Cause (High) */

#define GT_INT_ETH0       0x00000001      /* bit  0 */
#define GT_INT_ETH1       0x00000002      /* bit  1 */
#define GT_INT_ETH2       0x00000004      /* bit  2 */
#define GT_INT_SDMA       0x00000010      /* bit  4 */
#define GT_INT_I2C        0x00000020      /* bit  5 */
#define GT_INT_BRG        0x00000080      /* bit  7 */
#define GT_INT_MPSC0      0x00000100      /* bit  8 */
#define GT_INT_MPSC1      0x00000400      /* bit  10 */
#define GT_INT_COMM       0x00000800      /* bit  11 */
#define GT_INT_GPP7_0     0x01000000      /* bit  24 */
#define GT_INT_GPP15_8    0x02000000      /* bit  25 */
#define GT_INT_GPP23_16   0x04000000      /* bit  26 */
#define GT_INT_GPP31_24   0x08000000      /* bit  27 */

/* Timers registers locations */

#define TIMER_COUNTER_0                      0x850
#define TIMER_COUNTER_1                      0x854
#define TIMER_COUNTER_2                      0x858
#define TIMER_COUNTER_3                      0x85c
#define TIMER_COUNTER_4                      0x950
#define TIMER_COUNTER_5                      0x954
#define TIMER_COUNTER_6                      0x958
#define TIMER_COUNTER_7                      0x95c
#define TIMER_COUNTER0_3_CTRL                0x864
#define TIMER_COUNTER0_3_INT_CAUSE           0x868
#define TIMER_COUNTER0_3_INT_MASK            0x86c
#define TIMER_COUNTER4_7_CTRL                0x964
#define TIMER_COUNTER4_7_INT_CAUSE           0x968
#define TIMER_COUNTER4_7_INT_MASK            0x96c

/* Timer counter control register bit deines */

#define TIMER_COUNTER_CTRL_TC0EN             0x00000001
#define TIMER_COUNTER_CTRL_TC0MODE           0x00000002
#define TIMER_COUNTER_CTRL_TC0TRIG           0x00000004
#define TIMER_COUNTER_CTRL_TC1EN             0x00000100
#define TIMER_COUNTER_CTRL_TC1MODE           0x00000200
#define TIMER_COUNTER_CTRL_TC1TRIG           0x00000400
#define TIMER_COUNTER_CTRL_TC2EN             0x00010000
#define TIMER_COUNTER_CTRL_TC2MODE           0x00020000
#define TIMER_COUNTER_CTRL_TC2TRIG           0x00040000
#define TIMER_COUNTER_CTRL_TC3EN             0x01000000
#define TIMER_COUNTER_CTRL_TC3MODE           0x02000000
#define TIMER_COUNTER_CTRL_TC3TRIG           0x04000000

/* Timer counter interrupt cause register bit deines */

#define  TIMER_COUNTER_INT_CAUSE_TC0         0x00000001
#define  TIMER_COUNTER_INT_CAUSE_TC1         0x00000002
#define  TIMER_COUNTER_INT_CAUSE_TC2         0x00000004
#define  TIMER_COUNTER_INT_CAUSE_TC3         0x00000008
#define  TIMER_COUNTER_INT_CAUSE_SUM         0x80000000

/* Timer counter interrupt mask register bit deines */

#define  TIMER_COUNTER_INT_MASK_TC0          0x00000001
#define  TIMER_COUNTER_INT_MASK_TC1          0x00000002
#define  TIMER_COUNTER_INT_MASK_TC2          0x00000004
#define  TIMER_COUNTER_INT_MASK_TC3          0x00000008

#ifdef __cplusplus
}
#endif

#endif  /* __INCgt64240h */
