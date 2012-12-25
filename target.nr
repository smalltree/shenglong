'\" t
.\" cp7000g/target.nr - CP7000G target specific documentation
.\"
.\" Copyright 1984-2002 Wind River Systems, Inc. */
.\"
.\" This file has been developed or significantly modified by the
.\" MIPS Center of Excellence Dedicated Engineering Staff.
.\" This notice is as per the MIPS Center of Excellence Master Partner
.\" Agreement, do not remove this notice without checking first with
.\" WR/Platforms MIPS Center of Excellence engineering management.
.\"
.\" modification history
.\" --------------------
.\" 01a,19nov02,zmm  created 
.\"
.TH "cp7000g" T "Momentum Computer CP7000G Ocelot" "Rev: 25 Mar 2002" "TORNADO REFERENCE: VXWORKS"
.so wrs.an

.SH "NAME"
.aX "Momentum CP7000G Ocelot"

.SH "INTRODUCTION"
This reference entry provides board-specific information necessary to run
VxWorks for the cp7000g BSP. Before using a board with VxWorks,
verify that the board runs in the factory configuration by using
vendor-supplied ROMs and jumper settings and checking the RS-232
connection.

.SS "Boot ROMs"
The CP7000G contains a single 32 pin PLCC socket that is used with
the AMD AM29F040B flash ROM.  The size of the flash device is 512KB.
.PP
NVRAM onboard the CP7000G is implemented using an SGS-Thomson M48T37Y
Timekeeper.  This device contains up to 32KB of non-volatile memory,
16 bytes (top of memory) of which are registers for time-of-day.
The NVRAM is used for storage of boot parameters and (optionally)
the Ethernet MAC address of the wancom Ethernet controller. Offsets
0 through 0x7cf2 are available for application use.  The rest are
reserved.
.PP

.SS "Jumpers"
Below are the jumpers to enable various hardware features of the CP7000G.
This section summarizes all the jumpers on the board.
.TS E
expand;
cf3 s s
lf3 lf3 lf3
l l lw(3i) .
.ne 5
Momentum CP7000G Ocelot
.sp .25
Jumper	Function	Description
_
JP1	User Jumper	T{
This jumper, when installed, sets the corresponding status bit in
the Board Status Register of the CP7000G PLD.
T}
JP2	Test Mode 	T{
It enables Special Test Mode. It is not installed by default.
T}
JP3	Little Endian Select T{
This jumper has to be installed when Little Endian VxWorks images are
use. It has to be removed when Big Endian images are in use.
T}
JP4	Boot Flash Write Enable	T{
This jumper must be installed for the AM29F040B boot flash ROM to 
be programmable.
T}
JP5	JTAG Enable T{
Enables JTAG Scan Chain. It is not installed by facory setup.
T}
JP6	Test Mode 	T{
It enables Special Test Mode. It is not installed by default.
T}

JP8	ATX PWR On 	T{
When it is installed, it turns ATX power on (Valid only for ATX power
option).
T}
.TE
.PP
For details of jumper configuration, see the the board's hardware
manual for other jumpers not directly related to VxWorks operation.

.SH "FEATURES"
This section describes all features of the Momentum CP7000G Ocelot.
It documents all configurations of the board and the interaction between
features and configuration items.

.SS "Supported Features"

The following are supported features of the CP7000G in VxWorks:

    128MB, 256MB, 512MB, or 1024 MB memory options.
    L3 cache size of 2 MB, 4MB, 8MB, or none. 
    One 10/100 MBit Ethernet on front panel
    Two 10/100 MBit Ethernet ports (ENET) on rear panel.
    Two 10/100/1000 MBit Ethernets (G-ENET ports) on rear panel.
    Front panel serial port.
    Onboard NVRAM.

.SS "Untested Features"

The following features of the CP7000G are supported, however as yet not
completely tested:

    Local to Compact PCI interface.

.SS "Unsupported Features"

The following features of the CP7000G are not supported under the current 
release of the VxWorks Board Support Package:

    8MB M-Systems Flash Disk.
    Flash programming algorithms for AM29F040B.
    Time-of-Day clock.

.SH "HARDWARE DETAILS"
This section documents the details of the device drivers and board
hardware elements.

.SS "Devices"
The chip drivers included are:

    sysLib.c - Contains NVRAM access support for boot parameters
    i21555.c - INTEL 21554/21555 PCI-to-PCI bridge driver
    m48t37WdTimer.c - SGS-Thomson M48T37 watchdog timer
    gt64240.c - Galileo system controller driver

.SS "Devices"
The CP7000G board has one serial ports, the port is available to the
front panel. It is implemented using the National NS16552 Duart.
.PP
Ethernet support is provided by front panel RJ45 connectivity to
one wancom 10/100 Ethernet controller, and by two 10/100 Ethernet ports
on rear panel.
.PP
The INTEL 21554/21555 driver supports local to Compact PCI bus access and
mailbox interrupt handling.  It is included by the file i21555.c.
Note that this feature is not completely tested.
.PP
NVRAM access to the SGS-Thomson Timekeeper is implemented using routines
found in the sysLib.c file.
.PP
The front panel of the CP7000G board contains the RJ45 Ethernet port, RJ-11
serial consolse port, a BIT LED ("B"), a USER LED ("U"), and a Hot-Swap
Compact PCI ejector/injector release LED.  The Hot-Swap LED turns on
and remains on until either software resets the LED or the ejector is closed.

.SS "Memory Maps"
 
The CP7000G SBC operates in 32-bit address mode and thus supports a 4 GB
(gigabyte) physical address space.  This provides access to all onboard
devices as well as memory resource for mapping access to the Compact PCI
bus.  The memory regions of interest in this address
space are listed in the table below.

.TS E
expand;
cf3 s s
lf3 lf3 lf3
lw(1i) lw(1i) lw(1i) .
.ne 5
Physical Memory Map, CPU Bus
.sp .25
Start	End	Use
_
0x00000000	0x1fffffff	System RAM
0x20000000	0xbfffffff	Unused
0xc0000000	0xcfffffff	PCI Memory Space Allocation
0xd0000000	0xdfffffff	Compact PCI Memory/IO Space Allocation
0xe0000000	0xefffffff	PCI I/O Space Allocation
0xf0000000	0xf3ffffff	Unused
0xf4000000	0xf4000fff	Galileo GT64240 Internal Registers
0xfc000000	0xfc00000f	CP7000G PLD Registers
0xfc000010	0xfc7fffff	Unused
0xfc800000	0xfc807fef	Onboard NVRAM
0xfc808000	0xfcffffff	Unused
0xfd000000	0xfd00003f	Serial Ports
0xfd000040	0xfeffffff	Unused
0xff000000	0xff7fffff	Flash Disk
0xff800000	0xffbfffff	Unused
0xffc00000	0xffc7ffff	Boot Flash ROM
0xffc80000	0xffffffff	Unused
_
.TE

.TS E
expand;
cf3 s s
lf3 lf3 lf3
lw(1i) lw(1i) lw(1i).
.ne 5
Physical Memory Map, PCI Bus
.sp .25
Start	End	Use
_
0x00000000	0x1fffffff	System RAM (Memory Space)
0x20000000	0xbfffffff	Unused
0xc0000000	0xcfffffff	PCI Memory Space Allocation
0xd0000000	0xdfffffff	Compact PCI Memory/IO Space Allocation
0xe0000000	0xefffffff	PCI I/O Space Allocation
0xf0000000	0xffffffff	Unused
_
.TE

.SS "Shared Memory"
Test-And-Set support is currently not available.  Mailboxes are implemented
using the 21554/21555 PCI-to-PCI non-transparent bridge.  VxWorks
shared memory is currently not supported.

.SS "Interrupts"

The rm7000 extended and vectored interrupt handlers are supported.

.SS "Serial Configuration"

The serial port only supports RTS/CTS flow control.  No hardware flow
contol is supported otherwise.  Serial console is brought to the front
panel using an RJ-11 interface.

.SS "Network Configuration"
Ethernet is available through five different ports: one on the front
panel using the wancom Ethernet contoller, two 10/100 ports on the rear
panel also using wancom, and two 10/100/1000 ports on rear panel using
Intel82543 Ethernet contoller. The MAC addresses are obtained by the BSP
via an attached serial EEPROM, which is programmed at the factory.

.SS "Compact PCI Access"
Compact PCI Access is supported, but it is not fully tested.

.SS "PCI Access"
PCI Access is supported though GT64240 system contoller.
Two PCI buses are available, PCI bus 0 and PCI bus 1.

.SS "Boot Devices"

VxWorks booting can be accomplished through either ROM resident VxWorks
image or by download through the Ethernet.

.SS "Boot Methods"

All network boot configurations described in the VxWorks Programmer's
Guide and Network Programmer's guide is supported by the BSP.
.PP
Verification of successful code execution from the boot is indicated by
the BIT LED (indicated by a "B" on the front panel) turning off.  The
USER LED (indicated by a "U" on the front panel) is off and stays
off.

.SS "Timestamp"

The generic MIPS timestamp driver is supported. It is based on the
internal MIPS count/compare registers.

.SH "SPECIAL CONSIDERATIONS"
This section describes miscellaneous information that the user needs
to know about the BSP.

.SS "Deliverables"

Wind River BSPs may be supplied with one or more of the following
ROM-based pre-built projects: vxWorks_rom, vxWorks_romCompress and
vxWorks_romResident. Due to the current memory configuration of this
target, insufficient ROM/flash memory is available to contain the
following projects:
.PP

.bS
vxWorks_rom
vxWorks_romResident
.bE

As a result, these projects have not been included in the delivery
of this BSP. In some cases, it may be possible to add memory to a 
target sufficient for the undelivered images.  In this case, please 
reconfigure the ROM_SIZE variable to reflect the amount of memory on 
the target before building these projects.

.SH "BOARD LAYOUT"
The diagram below shows jumpers relevant to VxWorks configuration.

.bS
 ________________                  _______________________________________
| CPCI CONNECTOR |                | CPCI CONNECTORS                       |
|                 ----------------                                        |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|      +---------+                                                        |
|      |         |                                   VxWorks Socketed ROM |
|      | RM7000  |                                                        |
|      |         |                                    +-----+             |
|      |         |                                    |     |             |
|      +---------.  <- pin 1                          |     |             |
|                                                     +-----/             |
|                                                              JP6 JP3 JP1|
|                                                              :   :   :  |
|                                                reset         JP5 JP4 JP2|
|                                           COM1  o   ENET     :   :   :  |
|___________________________________________-----____------_______________|
.bE

.SH "REAR PANEL LAYOUT"

.bS
                                   ___________            ________________
                                  |           |          |                |
|---------------------------------             ----------                 |
|                                             CPCI CONNECTORS             |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|                                                                         |
|    ENET0     ENET1                        COM1     G-ENET1   G-ENET2    |
|____------____------_______________________-----____------____------_____|
.bE


    Key: 
    X  vertical jumper installed
    :  vertical jumper absent

.SH "BIBLIOGRAPHY"
.iB "VxWorks 5.5/Tornado 2.2 Board Support Package Guide"

.iB "Momentum Computer CP7000G Single Board Computer Specification Rev 1.0"
