# GRSPW device model: specification

This document specifies the behaviour of the GRSPW SpaceWire codec simulated by
`code/leon3_grspw_v1` on QEMU's `leon3_generic` machine. It is derived from two
sources:

- the GRSPW chapter of the GRLIB IP core user's manual
  ([grlib.pdf](https://download.gaisler.com/products/GRLIB/doc/grlib.pdf)), which
  defines the hardware;
- the RTEMS 6.2 drivers `bsps/shared/grlib/spw/grspw.c` (legacy driver) and
  `bsps/shared/grlib/spw/grspw_pkt.c` (packet driver), which define what software
  actually relies on.

The manual is the reference. The drivers are used to settle what the manual leaves
open, and to know what software relies on, since the goal of the model is that
unmodified RTEMS applications run on it. A requirement that departs from the manual
says so and gives its reason with the `[S]` tag.

## 1. Conventions

Each requirement has a tag `REQ_GRSPW_XXX`, where `XXX` is a three-digit number, unique
in this document. The tests and the code use it to refer to the requirement. A tag
is never reused: a requirement that is removed keeps its number, marked as deleted,
and a new requirement takes the next free number.

A requirement uses *shall*. Its source is given in brackets:

| Source tag | Source |
|-----|--------|
| `[M x.y]` | Manual, GRSPW chapter, section x.y (numbering of the chapter, e.g. 77.4.7) |
| `[L fn]` | Legacy driver `grspw.c`, function `fn` |
| `[P fn]` | Packet driver `grspw_pkt.c`, function `fn` |
| `[S]` | Simulation decision: no hardware equivalent, or a deliberate simplification |

Bit fields are written `REG.FIELD`, e.g. `CTRL.LS`. Access types:

| Access | Meaning |
|--------|---------|
| `r` | Read only. Writes are ignored. |
| `rw` | Read and write. Reads return the last value written. |
| `wc` | Status bit set by the device, cleared by writing 1. Writing 0 has no effect. |
| `sc` | Self clearing: writing 1 triggers an action, the bit always reads 0 afterwards. |
| `NR` | Not reset: keeps its value through a reset. |

## 2. Configuration

### 2.1 Cores

REQ_GRSPW_001. The device shall provide four independent GRSPW cores on the APB bus of
`leon3_generic`, with these parameters `[S]`:

| Core | APB base   | IRQ | RMAP target | Unaligned RX | RMAP CRC |
|------|------------|-----|-------------|--------------|----------|
| 0    | 0x80000A00 | 10  | No          | No           | No       |
| 1    | 0x80000B00 | 11  | No          | No           | No       |
| 2    | 0x80000C00 | 12  | Yes         | Yes          | Yes      |
| 3    | 0x80000D00 | 13  | Yes         | Yes          | Yes      |

REQ_GRSPW_002. Each core shall be declared in the APB plug and play table with vendor
0x01 (Gaisler) and device 0x1F (`GAISLER_SPW`), its IRQ number and an APB bar
covering 256 bytes `[M 77.11]`. Both drivers recognise a GRSPW1 by this device
identifier `[L grspw_device_init] [P grspw2_init3]`.

REQ_GRSPW_003. An RMAP core shall behave as the hardware configured with the `rmap`
generic set to 1, which also enables the RMAP CRC logic and unaligned receive
accesses `[M 77.13, 77.18]`. A non-RMAP core shall behave as `rmap`, `rmapcrc` and
`rxunaligned` set to 0.

REQ_GRSPW_004. Every core shall have one SpaceWire port (`ports` = 1) and one DMA channel
`[M 77.1]`.

REQ_GRSPW_005. The input signals of the hardware shall have these fixed values `[S]`:

| Signal | Value | Effect |
|--------|-------|--------|
| `rmapen` | 1 on RMAP cores | Reset value of `CTRL.RE` and `CTRL.AS` |
| `clkdiv10` | 4 | Reset value of both `CLKDIV` fields |
| `tickin` | 0 | Tick-in only through `CTRL.TI` |
| `nodeaddr` generic | 254 | Reset value of `NODEADDR` |
| `destkey` generic | 0 | Reset value of `DKEY` |
| `sysfreq` | `frequency` property, default 40 MHz | Reset value of `TDR` |

### 2.2 Data format

REQ_GRSPW_006. The core shall only support 32-bit big-endian hosts: the first byte of a
packet in memory is the most significant byte of the word `[M 77.1, 77.12.1]`.

## 3. Registers

### 3.1 Register map

REQ_GRSPW_007. The registers shall be 32-bit, at these offsets from the APB base `[M 77.10]`:

| Offset | Name | Register |
|--------|------|----------|
| 0x00 | `CTRL` | Control |
| 0x04 | `STS` | Status / interrupt source |
| 0x08 | `NODEADDR` | Node address |
| 0x0C | `CLKDIV` | Clock divisor |
| 0x10 | `DKEY` | Destination key |
| 0x14 | `TIME` | Time |
| 0x18 | `TDR` | Timer and disconnect |
| 0x20 | `DMACTRL` | DMA channel control and status |
| 0x24 | `DMAMAXLEN` | DMA RX maximum length |
| 0x28 | `DMATXDESC` | DMA transmit descriptor table address |
| 0x2C | `DMARXDESC` | DMA receive descriptor table address |

REQ_GRSPW_008. Any other offset of the 256-byte window (0x1C, 0x30 to 0xFC) shall read 0
and ignore writes. The packet driver relies on this: it reads the GRSPW2
interrupt-code registers (`ICCTRL` at 0xA0) and writes then reads back
`ICRLPRESC` (0xB8) to detect GRSPW2 features, and expects 0 for both
`[P grspw2_init3]`. It also writes the GRSPW2 per-channel address register at 0x30
`[P grspw_addr_ctrl]`.

REQ_GRSPW_009. Only aligned 32-bit accesses are defined: the result of any other access
is undefined `[M 77.7.1]`. Such an access shall not crash the emulator, and shall not
prevent later 32-bit accesses from working `[S]`.

Each register below has a table of its fields, then one requirement per field and one
per group of reserved bits, then the requirements that concern the whole register. A
field requirement gives the access type and the reset value of the table; the reset
value applies after a system reset and after `CTRL.RS` (REQ_GRSPW_156, REQ_GRSPW_157),
and an `NR` field is 0 after power-on and keeps its value through `CTRL.RS`. Bits that
the manual makes available only with a given generic (`rmap`, `ports` = 2) behave as
reserved on a core without that generic.

### 3.2 CTRL (0x00)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31 | RA | r | 1 on RMAP cores, else 0 | RMAP target available |
| 30 | RX | r | 1 on RMAP cores, else 0 | Unaligned receive accesses available |
| 29 | RC | r | 1 on RMAP cores, else 0 | RMAP CRC available |
| 28:27 | - | r | 0 | Reserved. GRSPW2 uses it for the number of DMA channels: must read 0 |
| 26 | PO | r | 0 | Number of ports minus one |
| 25:22 | - | r | 0 | Reserved. GRSPW2 uses bits 25 (CCSDS CRC) and 24 (interrupt distribution): must read 0 |
| 21 | PS | r | 0 | Port select. Only with 2 ports |
| 20 | NP | r | 0 | No port force. Only with 2 ports |
| 19:18 | - | r | 0 | Reserved |
| 17 | RD | rw (RMAP cores) | 0 | RMAP buffer disable |
| 16 | RE | rw (RMAP cores) | 1 on RMAP cores, else 0 | RMAP enable |
| 15:12 | - | r | 0 | Reserved |
| 11 | TR | rw | 0 | Time-code reception enable |
| 10 | TT | rw | 0 | Time-code transmission enable |
| 9 | LI | rw | NR | Interrupt on link error |
| 8 | TQ | rw | NR | Interrupt on tick-out |
| 7 | - | r | 0 | Reserved |
| 6 | RS | sc | 0 | Reset of the core |
| 5 | PM | rw | 0 | Promiscuous mode |
| 4 | TI | sc | 0 | Tick-in |
| 3 | IE | rw | 0 | Interrupt enable for the link events (LI, TQ) |
| 2 | AS | rw | `rmapen` on RMAP cores, else 0 | Autostart |
| 1 | LS | rw | 0 | Link start |
| 0 | LD | rw | 0 | Link disable |

REQ_GRSPW_010. `CTRL.RA` (bit 31) shall be read only. It shall read 1 on the RMAP cores
and 0 on the others (REQ_GRSPW_003), whatever is written `[M 77.10.1]`. The legacy
driver enables its RMAP controls only when RA is 1 `[L grspw_control SET_DESTKEY]`,
and the packet driver reads it as its RMAP support `[P grspw2_init3]`.

REQ_GRSPW_011. `CTRL.RX` (bit 30) shall be read only. It shall read 1 on the RMAP cores
and 0 on the others, whatever is written `[M 77.10.1]`. The packet driver reads it as
its unaligned-reception support `[P grspw2_init3]`.

REQ_GRSPW_012. `CTRL.RC` (bit 29) shall be read only. It shall read 1 on the RMAP cores
and 0 on the others, whatever is written `[M 77.10.1]`. The packet driver reads it as
its RMAP CRC support `[P grspw2_init3]`.

REQ_GRSPW_013. `CTRL.PO` (bit 26) shall be read only and read 0, the number of ports
minus one (REQ_GRSPW_004), whatever is written `[M 77.10.1]`. The packet driver
computes its number of ports from it `[P grspw2_init3]`.

REQ_GRSPW_014. `CTRL.PS` (bit 21) exists only with two ports. It shall read 0 and ignore
writes (REQ_GRSPW_004) `[M 77.10.1]`.

REQ_GRSPW_015. `CTRL.NP` (bit 20) exists only with two ports. It shall read 0 and ignore
writes (REQ_GRSPW_004) `[M 77.10.1]`.

REQ_GRSPW_016. `CTRL.RD` (bit 17) shall be read-write on the RMAP cores, with reset value
0 `[M 77.10.1]`. It has no effect in the model (REQ_GRSPW_145). On the other cores it
shall read 0 and ignore writes: the legacy driver reads RD back after writing it and
reports an I/O error when it differs, which is the expected result on a core without
RMAP `[L grspw_control SET_RMAPBUFDIS]`.

REQ_GRSPW_017. `CTRL.RE` (bit 16) shall be read-write on the RMAP cores, with reset value
1 (`rmapen`, REQ_GRSPW_005); it enables the RMAP target (REQ_GRSPW_105). On the other
cores it shall read 0 and ignore writes `[M 77.10.1]`. The legacy driver reads RE back
after writing it and reports an I/O error when it differs
`[L grspw_control SET_RMAPEN]`.

REQ_GRSPW_018. `CTRL.TR` (bit 11) shall be read-write, with reset value 0. It enables the
reception of time codes (REQ_GRSPW_090) `[M 77.10.1]`.

REQ_GRSPW_019. `CTRL.TT` (bit 10) shall be read-write, with reset value 0. It enables the
transmission of time codes on a tick-in (REQ_GRSPW_088, REQ_GRSPW_089) `[M 77.10.1]`.

REQ_GRSPW_020. `CTRL.LI` (bit 9) shall be read-write and `NR`. It enables the interrupt
on a link error (REQ_GRSPW_153) `[M 77.10.1]`.

REQ_GRSPW_021. `CTRL.TQ` (bit 8) shall be read-write and `NR`. It enables the interrupt
on a received valid time code (REQ_GRSPW_091, REQ_GRSPW_153) `[M 77.10.1]`.

REQ_GRSPW_022. `CTRL.RS` (bit 6) shall be self clearing: writing 1 shall reset the core
(REQ_GRSPW_157), and RS shall read 0 when the write returns `[M 77.10.1] [S]`.

REQ_GRSPW_023. `CTRL.PM` (bit 5) shall be read-write, with reset value 0. It makes every
packet go to the DMA channel whatever its address (REQ_GRSPW_104) `[M 77.10.1]`. The
legacy driver reads PM back after writing it `[L grspw_control SET_PROMISCUOUS]`.

REQ_GRSPW_024. `CTRL.TI` (bit 4) shall be self clearing. Writing 1 shall request a
time-code transmission (REQ_GRSPW_088). TI shall read 1 until the time code has been
sent, then 0 `[M 77.3.5]`. In the simulation the time code is sent during the write,
so TI reads 0 `[S]`.

REQ_GRSPW_025. `CTRL.IE` (bit 3) shall be read-write, with reset value 0. It enables the
interrupts selected by LI and TQ (REQ_GRSPW_153) `[M 77.10.1]`.

REQ_GRSPW_026. `CTRL.AS` (bit 2) shall be read-write, with reset value `rmapen` on the
RMAP cores (1, REQ_GRSPW_005) and 0 on the others. It starts the link when a NULL is
received (REQ_GRSPW_083, REQ_GRSPW_084) `[M 77.10.1]`.

REQ_GRSPW_027. `CTRL.LS` (bit 1) shall be read-write, with reset value 0. It starts the
link (REQ_GRSPW_083, REQ_GRSPW_084) `[M 77.10.1]`. The legacy driver checks
`CTRL[1:0]` = 10b after a link start `[L grspw_control LINKSTART]`.

REQ_GRSPW_028. `CTRL.LD` (bit 0) shall be read-write, with reset value 0. It disables the
link (REQ_GRSPW_083, REQ_GRSPW_084) `[M 77.10.1]`. The legacy driver checks
`CTRL[1:0]` = 01b after a link disable `[L grspw_control LINKDISABLE]`.

REQ_GRSPW_029. The reserved bits of `CTRL` (28:27, 25:22, 19:18, 15:12 and 7) shall read 0
and ignore writes. The packet driver derives its hardware support from bits 28:24,
so the number of DMA channels it computes must be 1 and the CCSDS CRC and interrupt
distribution support 0 `[P grspw2_init3]`.

### 3.3 STS (0x04)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:24 | - | r | 0 | Reserved |
| 23:21 | LS | r | 0 | Link state: 0 error-reset, 1 error-wait, 2 ready, 3 started, 4 connecting, 5 run |
| 20:10 | - | r | 0 | Reserved |
| 9 | AP | r | 0 | Active port. Only with 2 ports |
| 8 | EE | wc | 0 | Early EOP/EEP |
| 7 | IA | wc | 0 | Invalid address |
| 6 | WE | wc | 0 | Write synchronisation error |
| 5 | - | r | 0 | Reserved |
| 4 | PE | wc | 0 | Parity error |
| 3 | DE | wc | 0 | Disconnect error |
| 2 | ER | wc | 0 | Escape error |
| 1 | CE | wc | 0 | Credit error |
| 0 | TO | wc | 0 | Tick-out: a time code was received |

REQ_GRSPW_030. `STS.LS` (bits 23:21) shall be read only and give the state of the link
state machine (REQ_GRSPW_083, REQ_GRSPW_084); writes shall be ignored `[M 77.10.2]`.
The legacy driver waits for the value 5 (run) `[L grspw_hw_waitlink]`.

REQ_GRSPW_031. `STS.AP` (bit 9) exists only with two ports. It shall read 0 and ignore
writes (REQ_GRSPW_004) `[M 77.10.2]`.

REQ_GRSPW_032. `STS.EE` (bit 8) shall be `wc`, with reset value 0. It shall be set when a
packet too short is received (REQ_GRSPW_106) `[M 77.10.2]`.

REQ_GRSPW_033. `STS.IA` (bit 7) shall be `wc`, with reset value 0. It shall be set when a
packet with a wrong address is received (REQ_GRSPW_104) `[M 77.10.2]`.

REQ_GRSPW_034. `STS.WE` (bit 6) shall be `wc`, with reset value 0. It is set by the
physical layer only (REQ_GRSPW_086), so a write of 1 shall not set it `[M 77.10.2]`.

REQ_GRSPW_035. `STS.PE` (bit 4) shall be `wc`, with reset value 0. It is set by the
physical layer only (REQ_GRSPW_086), so a write of 1 shall not set it `[M 77.10.2]`.

REQ_GRSPW_036. `STS.DE` (bit 3) shall be `wc`, with reset value 0. It is set by the
physical layer only (REQ_GRSPW_086), so a write of 1 shall not set it `[M 77.10.2]`.

REQ_GRSPW_037. `STS.ER` (bit 2) shall be `wc`, with reset value 0. It is set by the
physical layer only (REQ_GRSPW_086), so a write of 1 shall not set it `[M 77.10.2]`.

REQ_GRSPW_038. `STS.CE` (bit 1) shall be `wc`, with reset value 0. It is set by the
physical layer only (REQ_GRSPW_086), so a write of 1 shall not set it `[M 77.10.2]`.

REQ_GRSPW_039. `STS.TO` (bit 0) shall be `wc`, with reset value 0. It shall be set when a
valid time code is received (REQ_GRSPW_091) `[M 77.3.5, 77.10.2]`. Both drivers
clear it, then read `TIME` `[L grspw_interrupt] [P grspw_isr]`.

REQ_GRSPW_040. The reserved bits of `STS` (31:24, 20:10 and 5) shall read 0 and ignore
writes.

REQ_GRSPW_041. Writing `0xFFFFFFFF` shall clear all `wc` bits and change nothing else. The
packet driver does this at initialisation `[P grspw_hw_softreset]`, the legacy
driver writes the mask of the `wc` bits `[L grspw_hw_reset]`.

REQ_GRSPW_042. The interrupt handlers read STS and write back the bits they have seen.
A bit set by the device between the read and the write shall stay set
`[L grspw_interrupt] [P grspw_isr]`.

### 3.4 NODEADDR (0x08)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:8 | - | r | 0 | Reserved |
| 7:0 | NODEADDR | rw | 254 | Node address |

REQ_GRSPW_043. `NODEADDR.NODEADDR` (bits 7:0) shall be read-write, with reset value 254
(the `nodeaddr` generic, REQ_GRSPW_005). Received packets are compared with it
(REQ_GRSPW_104) `[M 77.10.3]`.

REQ_GRSPW_044. Bits 31:8 of `NODEADDR` shall read 0 and ignore writes. The packet driver
writes the GRSPW2 node mask in bits 15:8 and reads it back as the current mask; a
GRSPW1 has no mask, so it reads 0 `[P grspw_addr_ctrl]`. The legacy driver only
writes the mask on a GRSPW2 `[L grspw_control SET_NODEMASK]`.

### 3.5 CLKDIV (0x0C)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:16 | - | r | 0 | Reserved |
| 15:8 | CLKDIVSTART | rw | `clkdiv10` | Divisor in the states other than run |
| 7:0 | CLKDIVRUN | rw | `clkdiv10` | Divisor in the run state |

REQ_GRSPW_045. `CLKDIV.CLKDIVSTART` (bits 15:8) shall be read-write, with reset value
`clkdiv10` (4, REQ_GRSPW_005) `[M 77.10.4]`. The value has no effect on the simulated
link `[S]`. The legacy driver compares the whole register after each write
`[L grspw_control SET_CLKDIVSTART]`.

REQ_GRSPW_046. `CLKDIV.CLKDIVRUN` (bits 7:0) shall be read-write, with reset value
`clkdiv10` (4, REQ_GRSPW_005) `[M 77.10.4]`. The value has no effect on the simulated
link `[S]`. The legacy driver compares the whole register after each write
`[L grspw_control SET_CLKDIV]`.

REQ_GRSPW_047. Bits 31:16 of `CLKDIV` shall read 0 and ignore writes.

### 3.6 DKEY (0x10)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:8 | - | r | 0 | Reserved |
| 7:0 | DESTKEY | rw (RMAP cores) | 0 | RMAP destination key |

REQ_GRSPW_048. `DKEY.DESTKEY` (bits 7:0) shall be read-write on the RMAP cores, with
reset value 0 (the `destkey` generic, REQ_GRSPW_005); RMAP commands are checked
against it (REQ_GRSPW_141) `[M 77.10.5]`. On the other cores it shall read 0 and
ignore writes. The legacy driver does not write it there: it refuses the key with
`RTEMS_NOT_IMPLEMENTED` when `CTRL.RA` is 0 `[L grspw_control SET_DESTKEY]`.

REQ_GRSPW_049. Bits 31:8 of `DKEY` shall read 0 and ignore writes.

### 3.7 TIME (0x14)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:8 | - | r | 0 | Reserved |
| 7:6 | TCTRL | rw | 0 | Time control flags |
| 5:0 | TIMECNT | rw | 0 | Time counter |

REQ_GRSPW_050. `TIME.TCTRL` (bits 7:6) shall be read-write, with reset value 0. It is
sent with the counter on a tick-in (REQ_GRSPW_088) and updated by a received valid
time code (REQ_GRSPW_091) `[M 77.3.5, 77.10.6]`.

REQ_GRSPW_051. `TIME.TIMECNT` (bits 5:0) shall be read-write, with reset value 0. It is
incremented by a tick-in (REQ_GRSPW_088) and updated by a received time code
(REQ_GRSPW_090) `[M 77.10.6]`. The packet driver writes `TIME` = 0 at initialisation
`[P grspw_hw_softreset]`.

REQ_GRSPW_052. Bits 31:8 of `TIME` shall read 0 and ignore writes.

REQ_GRSPW_053. A write of `TIME` shall update both fields without transmitting a time
code `[M 77.10.6]`.

### 3.8 TDR (0x18)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:22 | - | r | 0 | Reserved |
| 21:12 | DISCONNECT | rw | from `sysfreq` | Disconnect time, in clock cycles minus 3 |
| 11:0 | TIMER64 | rw | from `sysfreq` | Cycles for 6.4 µs |

REQ_GRSPW_054. `TDR.DISCONNECT` (bits 21:12) shall be read-write. Its reset value shall be
computed from the system frequency as the drivers do,
`DISCONNECT = ceil(f_kHz × 85 / 100000) − 3`, which gives 31 at 40 MHz
`[M 77.8.3, 77.10.7] [P grspw1_calc_discon]`. Both drivers write it and the legacy
driver checks it `[L grspw_control SET_COREFREQ, SET_DISCONNECT] [P grspw2_init3]`. The
value has no effect on the simulated link `[S]`.

REQ_GRSPW_055. `TDR.TIMER64` (bits 11:0) shall be read-write. Its reset value shall be
computed from the system frequency as the drivers do,
`TIMER64 = ceil(f_kHz × 64 / 10000)`, which gives 256 at 40 MHz
`[M 77.8.3, 77.10.7] [P grspw1_calc_timer64]`. Both drivers write it and the legacy
driver checks it `[L grspw_control SET_COREFREQ, SET_TIMER] [P grspw2_init3]`. The
value has no effect on the simulated link `[S]`.

REQ_GRSPW_056. Bits 31:22 of `TDR` shall read 0 and ignore writes.

### 3.9 DMACTRL (0x20)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:17 | - | r | 0 | Reserved |
| 16 | LE | rw | 0 | Disable the transmitter on a link error |
| 15:13 | - | r | 0 | Reserved. GRSPW2 uses them (SP, SA, EN): must read 0 |
| 12 | NS | rw | 0 | No spill |
| 11 | RD | rw | 0 | RX descriptors available. Cleared by the core |
| 10 | RX | r | 0 | Reception active |
| 9 | AT | sc | 0 | Abort transmission |
| 8 | RA | wc | 0 | RX AHB error |
| 7 | TA | wc | 0 | TX AHB error |
| 6 | PR | wc | 0 | Packet received |
| 5 | PS | wc | 0 | Packet sent |
| 4 | AI | rw | NR | Interrupt on AHB error |
| 3 | RI | rw | NR | Interrupt on reception |
| 2 | TI | rw | NR | Interrupt on transmission |
| 1 | RE | rw | 0 | Receiver enable |
| 0 | TE | rw | 0 | Transmitter enable. Cleared by the core |

REQ_GRSPW_057. `DMACTRL.LE` (bit 16) shall be read-write, with reset value 0. It selects
whether a link error stops the transmitter (REQ_GRSPW_135) `[M 77.10.8]`.

REQ_GRSPW_058. `DMACTRL.NS` (bit 12) shall be read-write, with reset value 0. It selects
whether packets without a descriptor are kept or discarded (REQ_GRSPW_107)
`[M 77.10.8]`.

REQ_GRSPW_059. `DMACTRL.RD` (bit 11) shall be read-write, with reset value 0. Writing 1
makes the core read the current receive descriptor again (REQ_GRSPW_110); the core
shall clear it when a packet finds the descriptor disabled (REQ_GRSPW_107)
`[M 77.10.8]`.

REQ_GRSPW_060. `DMACTRL.RX` (bit 10) shall be read only, with reset value 0. It shall
read 1 only while a packet is being written to memory. Since a packet is written in
one step `[S]`, RX reads 0 between guest accesses. The legacy driver polls it before
changing its buffers `[L grspw_hw_wait_rx_inactive]`.

REQ_GRSPW_061. `DMACTRL.AT` (bit 9) shall be self clearing: writing 1 aborts the current
transmission (REQ_GRSPW_136), and AT shall read 0 afterwards `[M 77.10.8]`.

REQ_GRSPW_062. `DMACTRL.RA` (bit 8) shall be `wc`, with reset value 0. It shall be set by
a memory access error on reception (REQ_GRSPW_114) `[M 77.10.8]`.

REQ_GRSPW_063. `DMACTRL.TA` (bit 7) shall be `wc`, with reset value 0. It shall be set by
a memory access error on transmission (REQ_GRSPW_137) `[M 77.10.8]`.

REQ_GRSPW_064. `DMACTRL.PR` (bit 6) shall be `wc`, with reset value 0. It shall be set
when a packet has been stored (REQ_GRSPW_108) `[M 77.10.8]`.

REQ_GRSPW_065. `DMACTRL.PS` (bit 5) shall be `wc`, with reset value 0. It shall be set
when a packet has been sent (REQ_GRSPW_133) `[M 77.10.8]`.

REQ_GRSPW_066. `DMACTRL.AI` (bit 4) shall be read-write and `NR`. It enables the
interrupt on an AHB error (REQ_GRSPW_154) `[M 77.10.8]`.

REQ_GRSPW_067. `DMACTRL.RI` (bit 3) shall be read-write and `NR`. It enables the
interrupt on reception (REQ_GRSPW_154) `[M 77.10.8]`.

REQ_GRSPW_068. `DMACTRL.TI` (bit 2) shall be read-write and `NR`. It enables the
interrupt on transmission (REQ_GRSPW_154) `[M 77.10.8]`.

REQ_GRSPW_069. `DMACTRL.RE` (bit 1) shall be read-write, with reset value 0. It enables
the receiver (REQ_GRSPW_107, REQ_GRSPW_111) `[M 77.10.8]`.

REQ_GRSPW_070. `DMACTRL.TE` (bit 0) shall be read-write, with reset value 0. Writing 1
starts the transmitter (REQ_GRSPW_129); the core shall clear it when it finds a
disabled descriptor (REQ_GRSPW_132), on an abort (REQ_GRSPW_136) and on a memory
error (REQ_GRSPW_137) `[M 77.10.8]`.

REQ_GRSPW_071. The reserved bits of `DMACTRL` (31:17 and 15:13) shall read 0 and ignore
writes.

REQ_GRSPW_072. After a write of `PS | PR | TA | RA` with no reception in progress, the
register shall read 0. The legacy driver fails its start-up otherwise
`[L grspw_hw_startup]`.

REQ_GRSPW_073. The drivers update the register by read-modify-write, and write back the
`wc` bits they read in order to clear them, together with the control bits
`[L grspw_interrupt, grspw_hw_send, grspw_rxnext, grspw_hw_stop]
[P grspw_isr, grspw_dma_enable_int]`. Therefore:

- writing 1 to RE, TE or RD shall be accepted at any time, including when the bit
  is already 1;
- writing 1 to TE when the next descriptor is disabled shall only clear TE again
  (REQ_GRSPW_132);
- a `wc` bit set by the device between the read and the write shall stay set.

### 3.10 DMAMAXLEN (0x24)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:25 | - | r | 0 | Reserved |
| 24:2 | RXMAXLEN | rw | NR | Maximum length of a received packet, in bytes |
| 1:0 | - | r | 0 | Always 0 |

REQ_GRSPW_074. `DMAMAXLEN.RXMAXLEN` (bits 24:2) shall be read-write and `NR`; a received
packet is cut at this length (REQ_GRSPW_108) `[M 77.10.9]`. A value that is a multiple
of 4 shall read back unchanged. The legacy driver compares the value read with the
value written (it rounds its buffer size up to a multiple of 8)
`[L grspw_set_rxmaxlen]`. The packet driver writes its configured length, 1024 by
default, without checking `[P grspw_dma_start]`.

REQ_GRSPW_075. Bits 31:25 and 1:0 of `DMAMAXLEN` shall read 0 and ignore writes.

### 3.11 DMATXDESC (0x28)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:10 | DESCBASEADDR | rw | NR | Table base, 1 KiB aligned |
| 9:4 | DESCSEL | rw | 0 | Offset of the current descriptor (16-byte steps) |
| 3:0 | - | r | 0 | Reserved |

REQ_GRSPW_076. `DMATXDESC.DESCBASEADDR` (bits 31:10) shall be read-write and `NR`. It is
the address of the 1 KiB transmit descriptor table `[M 77.10.10]`.

REQ_GRSPW_077. `DMATXDESC.DESCSEL` (bits 9:4) shall be read-write, with reset value 0.
The core shall start from the descriptor it selects, advance it by one descriptor
each time a descriptor is used, and return it to 0 after the last of the 64
descriptors or after a descriptor with its WR bit set `[M 77.5.6]`. The packet driver
sets WR only on the last of 64 descriptors, the legacy driver on the last of its
configured count `[P grspw_tx_schedule_send] [L grspw_hw_send]`.

REQ_GRSPW_078. Bits 3:0 of `DMATXDESC` shall read 0 and ignore writes.

### 3.12 DMARXDESC (0x2C)

| Bits | Field | Access | Reset | Description |
|------|-------|--------|-------|-------------|
| 31:10 | DESCBASEADDR | rw | NR | Table base, 1 KiB aligned |
| 9:3 | DESCSEL | rw | 0 | Offset of the current descriptor (8-byte steps) |
| 2:0 | - | r | 0 | Reserved |

REQ_GRSPW_079. `DMARXDESC.DESCBASEADDR` (bits 31:10) shall be read-write and `NR`. It is
the address of the 1 KiB receive descriptor table `[M 77.10.11]`.

REQ_GRSPW_080. `DMARXDESC.DESCSEL` (bits 9:3) shall be read-write, with reset value 0.
The core shall start from the descriptor it selects, advance it by one descriptor
each time a descriptor is used, and return it to 0 after the last of the 128
descriptors or after a descriptor with its WR bit set `[M 77.4.3]`. The packet driver
sets WR only on the last of 128 descriptors, the legacy driver on the last of its
configured count `[P grspw_rx_schedule_ready] [L grspw_hw_startup]`.

REQ_GRSPW_081. Bits 2:0 of `DMARXDESC` shall read 0 and ignore writes.

REQ_GRSPW_082. Both drivers write the table bases with DESCSEL = 0 before enabling the
channel, and the packet driver writes 0 at initialisation
`[L grspw_hw_startup] [P grspw_dma_start, grspw_hw_dma_softreset]`. `DMATXDESC` and
`DMARXDESC` can be written while the channel is idle; writing them while the channel
is active is undefined in the hardware and shall not crash the emulator
`[M 77.4.3, 77.5.6]`.

## 4. Link interface

REQ_GRSPW_083. The link state machine shall follow the SpaceWire standard as controlled by
`CTRL` `[M 77.3.1]`:

- with LD set, the link shall go to, or stay in, the error-reset state (0);
- with LD clear, the link may start when LS is set, or when AS is set and a NULL is
  received from the peer.

REQ_GRSPW_084. The physical layer is not simulated `[S]`. The peer is considered always
present and the link start immediate:

- writing LS = 1 or AS = 1 with LD = 0 shall put `STS.LS` to run (5) before the
  write returns;
- writing LD = 1 shall put `STS.LS` to error-reset (0);
- when LD, LS and AS are all 0, the state shall be left as it is.

The legacy driver writes LS at open time and then waits for the run state, checking
every 10 clock ticks with a default timeout of 10 ticks
`[L grspw_hw_reset, grspw_hw_waitlink]`. The packet driver never waits for the link.

REQ_GRSPW_085. Leaving the run state while a packet is being transmitted is a link error
(REQ_GRSPW_135). With REQ_GRSPW_084, this only happens through a guest write of LD.

REQ_GRSPW_086. The error bits WE, PE, DE, ER and CE of `STS` come from the physical layer.
They shall not be set in nominal operation `[S]`. The interrupt and link disable
logic attached to them (REQ_GRSPW_153) shall still be implemented, so that fault injection
can be added.

REQ_GRSPW_087. When the packet driver sees one of the error bits selected with its
`LINKOPTS_DIS_ON_*` options while `CTRL.IE` is set, it writes
`LD | (CTRL & ~(IE | LS))` from its interrupt handler `[P grspw_isr]`. This write
shall bring the link down like any write of LD.

## 5. Time codes

REQ_GRSPW_088. A tick-in (write of `CTRL.TI` = 1) with `CTRL.TT` = 1 and the link in the run
state shall increment `TIME.TIMECNT` modulo 64 and send a time code made of
`TIME.TCTRL` and the new counter value `[M 77.3.5]`.

REQ_GRSPW_089. A tick-in with TT = 0 shall have no effect: no increment and nothing sent
`[M 77.3.5]`.

REQ_GRSPW_090. A time code received with `CTRL.TR` = 1 shall store its counter in
`TIME.TIMECNT`, whatever its value. With TR = 0, a received time code shall be ignored
`[M 77.3.5, 77.10.6]`.

REQ_GRSPW_091. A received time code is valid when its counter is the value of
`TIME.TIMECNT` before the reception plus one, modulo 64. A valid time code received
with `CTRL.TR` = 1 shall also `[M 77.3.5, 77.10.1]`:

- store its control flags in `TIME.TCTRL`;
- set `STS.TO` (tick-out);
- raise the interrupt if `CTRL.TQ` and `CTRL.IE` are set (REQ_GRSPW_153).

A time code that is not valid shall only update `TIME.TIMECNT` (REQ_GRSPW_090). The
manual does not define a valid time code; this is the definition of the SpaceWire
standard, and the condition the manual gives for storing the control flags. Both
drivers read the time register on TO and do not check continuity
`[L grspw_interrupt] [P grspw_isr]`.

REQ_GRSPW_092. TT and TR may be set at the same time `[M 77.3.5]`.

## 6. Receive DMA

### 6.1 Descriptors

REQ_GRSPW_093. A receive descriptor shall have 8 bytes `[M 77.4.4]`:

| Word | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 31 | TR | Packet truncated (longer than `DMAMAXLEN`). Written by the core |
| 0 | 30 | DC | Data CRC error. Written by the core |
| 0 | 29 | HC | Header CRC error. Written by the core |
| 0 | 28 | EP | Packet ended by an EEP. Written by the core |
| 0 | 27 | IE | Interrupt on reception of this packet |
| 0 | 26 | WR | Next descriptor is the first of the table |
| 0 | 25 | EN | Descriptor enabled. Cleared by the core |
| 0 | 24:0 | PACKETLENGTH | Number of bytes stored. Written by the core |
| 1 | 31:0 | PACKETADDRESS | Buffer address |

The guest writes word 1, then word 0 with EN = 1, IE and WR; the core writes the other
fields of word 0 when the packet ends (REQ_GRSPW_103). The requirements below give each
field on its own.

REQ_GRSPW_094. `TR` (word 0, bit 31) shall be written by the core when the packet ends:
1 when the packet was longer than `DMAMAXLEN` and was cut (REQ_GRSPW_108), 0 otherwise,
whatever the guest wrote in it `[M 77.4.4]`. The legacy driver reports a truncated
packet as an error `[L grspw_hw_receive]`.

REQ_GRSPW_095. `DC` (word 0, bit 30) shall be written by the core when the packet ends:
on an RMAP core, 1 when the CRC of the packet is not zero at its end (REQ_GRSPW_112);
on the other cores always 0, whatever the guest wrote in it `[M 77.4.4, 77.4.8]`.

REQ_GRSPW_096. `HC` (word 0, bit 29) shall be written by the core when the packet ends:
on an RMAP core, 1 when the CRC is not zero at the end of the header (REQ_GRSPW_112);
on the other cores always 0, whatever the guest wrote in it `[M 77.4.4, 77.4.8]`.

REQ_GRSPW_097. `EP` (word 0, bit 28) shall be written by the core when the packet ends:
1 for a packet ended by an EEP (REQ_GRSPW_113), 0 for a packet ended by an EOP,
whatever the guest wrote in it `[M 77.4.4]`. The legacy driver reports a packet with
EP as an error `[L grspw_hw_receive]`.

REQ_GRSPW_098. `IE` (word 0, bit 27) shall be read by the core: when it is set and
`DMACTRL.RI` is set, storing the packet shall raise the interrupt (REQ_GRSPW_154); when
it is clear, no reception interrupt shall be raised for this packet. The core shall
keep its value when it writes word 0 `[M 77.4.4]`.

REQ_GRSPW_099. `WR` (word 0, bit 26) shall be read by the core: when it is set, the next
descriptor shall be the first of the table (REQ_GRSPW_080). The core shall keep its
value when it writes word 0 `[M 77.4.4]`.

REQ_GRSPW_100. `EN` (word 0, bit 25) shall be read by the core: a packet shall be stored
only in an enabled descriptor, and a disabled descriptor shall be handled as in
REQ_GRSPW_107. The core shall write it to 0 when the packet ends, which both drivers
take as the sign that the packet is complete `[M 77.4.4]
[L grspw_hw_receive] [P grspw_rx_process_scheduled]`.

REQ_GRSPW_101. `PACKETLENGTH` (word 0, bits 24:0) shall be written by the core when the
packet ends, with the number of bytes stored, address and protocol identifier
included (REQ_GRSPW_108), whatever the guest wrote in it `[M 77.4.4]`. Both drivers
read the length from these bits `[L grspw_hw_receive] [P grspw_rx_process_scheduled]`.

REQ_GRSPW_102. `PACKETADDRESS` (word 1) shall be the address where the first byte of
the packet is stored, and shall not be changed by the core `[M 77.4.4]`. On a non-RMAP
core the address shall be used word-aligned (bits 1:0 ignored), and the last word of
the packet may be written in full. On an RMAP core any address shall be accepted and
only the bytes of the packet written `[M 77.4.4, 77.7]`. The legacy driver aligns its
buffers on 8 bytes; the packet driver uses the buffers of the application.

REQ_GRSPW_103. When a packet ends, the core shall write word 0 in one access with EN = 0,
the status bits and the length, and keep IE and WR as they were. Both drivers read
the length from bits 24:0 and the status from bits 31:28 once EN is 0
`[L grspw_hw_receive] [P grspw_rx_process_scheduled]`.

### 6.2 Reception process

REQ_GRSPW_104. The core shall take the first byte of a packet as its logical address and
compare it with `NODEADDR` `[M 77.4.7]`:

- on a mismatch, the packet shall be discarded and `STS.IA` set, unless `CTRL.PM`
  is set;
- with PM set, every packet shall go to the DMA channel, whatever its address
  `[M 77.4.10]`.

REQ_GRSPW_105. On an RMAP core with `CTRL.RE` = 1, a packet whose second byte is 0x01 and
whose third byte has bits 7:6 = 01b (RMAP command) shall go to the RMAP target
(section 8). Every other packet, including RMAP replies, packets with the extended
protocol identifier and all packets when RE = 0, shall go to the DMA channel
`[M 77.2.2, 77.4.7, 77.6.6]`. This also applies in promiscuous mode
`[M 77.4.10]`.

REQ_GRSPW_106. A packet shorter than 2 bytes, or shorter than 3 bytes when it has protocol
identifier 0x01 and RMAP is enabled, shall be discarded and set `STS.EE`
`[M 77.4.7, 77.10.2]`.

REQ_GRSPW_107. A packet that goes to the DMA channel shall be handled as follows
`[M 77.4.6]`:

| `DMACTRL.RE` | `DMACTRL.RD` | Next descriptor | `DMACTRL.NS` | Action |
|--------------|--------------|-----------------|--------------|--------|
| 0 | - | - | - | Discard the packet |
| 1 | 0 | - | 0 | Discard the packet |
| 1 | 0 | - | 1 | Keep the packet until RD is set |
| 1 | 1 | Enabled | - | Store the packet (REQ_GRSPW_108) |
| 1 | 1 | Disabled | 0 | Clear RD, discard the packet |
| 1 | 1 | Disabled | 1 | Clear RD, keep the packet until RD is set again |

REQ_GRSPW_108. Storing a packet shall:

1. write the whole packet, address and protocol identifier included, EOP/EEP
   excluded, from PACKETADDRESS; both drivers expect the address at offset 0 and
   skip it themselves `[L grspw_hw_receive]`;
2. stop at `DMAMAXLEN` bytes, discard the rest and set TR when the packet is longer
   `[M 77.4.2]`;
3. write word 0 of the descriptor (REQ_GRSPW_103);
4. advance `DMARXDESC.DESCSEL` (REQ_GRSPW_080);
5. set `DMACTRL.PR`;
6. raise the interrupt if the descriptor IE and `DMACTRL.RI` are both set (REQ_GRSPW_154).

REQ_GRSPW_109. Kept packets (NS = 1) shall be stored in arrival order once descriptors are
available. The model shall keep at least 64 packets per core; later packets shall
be discarded `[S]`.

REQ_GRSPW_110. Writing RD = 1 shall make the core read the current descriptor again, and
store a kept packet at once if the descriptor is enabled `[M 77.4.5]`. Both drivers
write RD after enabling descriptors `[L grspw_rxnext] [P grspw_rx_schedule_ready]`.

REQ_GRSPW_111. Clearing RE shall make later packets be discarded; a packet already being
stored shall be completed `[M 77.4.6]`.

### 6.3 CRC check

REQ_GRSPW_112. On an RMAP core, the RMAP CRC (8-bit CRC of ECSS-E-ST-50-52C) shall be
computed over every received packet, EOP/EEP excluded `[M 77.4.8]`:

- the header length is deduced from the command byte (third byte), as if the packet
  were an RMAP packet; if the CRC is not zero at the end of the header, HC shall be
  set;
- the computation shall continue without restarting up to the end of the packet; if
  it is not zero at the end, DC shall be set.

On a non-RMAP core, HC and DC shall always be 0. The legacy driver can treat HC and
DC as errors (`check_rmap_err`) `[L grspw_hw_receive]`.

### 6.4 Errors

REQ_GRSPW_113. A packet ended by an EEP shall be stored with EP set `[M 77.4.4]`.

REQ_GRSPW_114. A memory access error while storing a packet shall discard the rest of the
packet, clear `DMACTRL.RE`, set `DMACTRL.RA` and raise the interrupt if `DMACTRL.AI`
is set `[M 77.4.9]`. In the simulation, an access outside guest memory is such an
error `[S]`.

## 7. Transmit DMA

### 7.1 Descriptors

REQ_GRSPW_115. A transmit descriptor shall have 16 bytes `[M 77.5.3]`:

| Word | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 31:18 | - | Reserved |
| 0 | 17 | DC | Append the RMAP CRC after the data |
| 0 | 16 | HC | Append the RMAP CRC after the header |
| 0 | 15 | LE | Link error during the transmission. Written by the core |
| 0 | 14 | IE | Interrupt when the packet has been sent |
| 0 | 13 | WR | Next descriptor is the first of the table |
| 0 | 12 | EN | Descriptor enabled. Cleared by the core |
| 0 | 11:8 | NONCRCLEN | Bytes at the start of the header excluded from the CRC |
| 0 | 7:0 | HEADERLEN | Header length, 0 to 255 |
| 1 | 31:0 | HEADERADDRESS | Header address, any alignment |
| 2 | 31:24 | - | Reserved |
| 2 | 23:0 | DATALEN | Data length, 0 to 16 MiB − 1 |
| 3 | 31:0 | DATAADDRESS | Data address, any alignment |

The guest writes words 1 to 3, then word 0 with EN = 1; the core writes EN and LE when
the packet has been sent (REQ_GRSPW_133). The requirements below give each field on
its own.

REQ_GRSPW_116. `DC` (word 0, bit 17) shall be read by the core: on an RMAP core, when it
is set, the RMAP CRC of the data shall be sent after the data, and when it is clear,
no CRC; on the other cores it shall be ignored and no CRC sent (REQ_GRSPW_130)
`[M 77.5.3]`.

REQ_GRSPW_117. `HC` (word 0, bit 16) shall be read by the core: on an RMAP core, when it
is set, the RMAP CRC of the header, without its first NONCRCLEN bytes, shall be sent
after the header, and when it is clear, no CRC; on the other cores it shall be
ignored and no CRC sent (REQ_GRSPW_130) `[M 77.5.3]`.

REQ_GRSPW_118. `LE` (word 0, bit 15) shall be written by the core when the descriptor
is completed: 1 when the packet was ended by a link error or an abort
(REQ_GRSPW_135, REQ_GRSPW_136), 0 otherwise, whatever the guest wrote in it
`[M 77.5.3]`. The legacy driver scans the completed descriptors for LE
`[L grspw_interrupt]`.

REQ_GRSPW_119. `IE` (word 0, bit 14) shall be read by the core: when it is set and
`DMACTRL.TI` is set, the completion of the descriptor shall raise the interrupt
(REQ_GRSPW_154); when it is clear, no transmission interrupt shall be raised for this
packet. The core shall keep its value when it writes word 0 `[M 77.5.3]`.

REQ_GRSPW_120. `WR` (word 0, bit 13) shall be read by the core: when it is set, the next
descriptor shall be the first of the table (REQ_GRSPW_077). The core shall keep its
value when it writes word 0 `[M 77.5.3]`.

REQ_GRSPW_121. `EN` (word 0, bit 12) shall be read by the core: an enabled descriptor
shall be sent (REQ_GRSPW_130), a disabled one shall stop the transmitter
(REQ_GRSPW_132). The core shall write it to 0 when the descriptor is completed
(REQ_GRSPW_133) `[M 77.5.3]`. The legacy driver in blocking mode and the packet driver
take EN = 0 as the sign that the packet has been sent
`[L grspw_hw_send] [P grspw_tx_process_scheduled]`.

REQ_GRSPW_122. `NONCRCLEN` (word 0, bits 11:8) shall give the number of bytes at the
start of the header, 0 to 15, that are sent but not included in the header CRC
(REQ_GRSPW_117) `[M 77.5.3]`. The core shall keep its value when it writes word 0.

REQ_GRSPW_123. `HEADERLEN` (word 0, bits 7:0) shall give the number of header bytes sent
from HEADERADDRESS, 0 to 255 (REQ_GRSPW_130, REQ_GRSPW_131) `[M 77.5.3]`. The core
shall keep its value when it writes word 0.

REQ_GRSPW_124. `HEADERADDRESS` (word 1) shall be the address of the header, with any
alignment, and shall not be changed by the core `[M 77.5.3]`.

REQ_GRSPW_125. `DATALEN` (word 2, bits 23:0) shall give the number of data bytes sent
from DATAADDRESS (REQ_GRSPW_130, REQ_GRSPW_131) `[M 77.5.3]`, within the transport limit
(REQ_GRSPW_138). It shall not be changed by the core.

REQ_GRSPW_126. `DATAADDRESS` (word 3) shall be the address of the data, with any
alignment, and shall not be changed by the core `[M 77.5.3]`.

REQ_GRSPW_127. The reserved bits 31:18 of word 0 shall be ignored by the core, and kept
when it writes word 0. The packet driver may set bits 19:18 (GRSPW2 CRC type): the
CRC shall always be the RMAP CRC `[P grspw_tx_schedule_send]`.

REQ_GRSPW_128. The reserved bits 31:24 of word 2 shall be ignored by the core. The
packet driver copies the control bits into bits 29:24 of word 2, for GRSPW2
compatibility: DATALEN shall be bits 23:0 only `[P grspw_tx_schedule_send]`.

### 7.2 Transmission process

REQ_GRSPW_129. Writing `DMACTRL.TE` = 1 shall make the core read the current descriptor
`[M 77.5.4, 77.5.5]`.

REQ_GRSPW_130. For an enabled descriptor, the core shall send one packet made of `[M 77.5.3]`:

1. HEADERLEN bytes from HEADERADDRESS;
2. if HC is set (RMAP cores only), the CRC of the header bytes after the first
   NONCRCLEN; it is not sent when HEADERLEN is 0;
3. DATALEN bytes from DATAADDRESS;
4. if DC is set (RMAP cores only), the CRC of the data bytes; a CRC of zero length
   (0x00) is sent when DATALEN is 0;
5. an EOP.

The core shall not add the address or protocol identifier: they must be in the
buffers `[M 77.2.2]`.

REQ_GRSPW_131. A descriptor with HEADERLEN = 0 and DATALEN = 0 shall send nothing, not even
an EOP, and shall still be completed (REQ_GRSPW_133) `[M 77.5.3]`.

REQ_GRSPW_132. A disabled descriptor shall clear TE and stop the transmitter until TE is
written again `[M 77.5.4]`. The legacy driver in blocking mode waits for TE = 0
`[L grspw_hw_send]`.

REQ_GRSPW_133. When a packet has been sent, the core shall `[M 77.5.5]`:

1. write word 0 with EN = 0 and the status (LE), keeping the other fields;
2. advance `DMATXDESC.DESCSEL` (REQ_GRSPW_077);
3. set `DMACTRL.PS`;
4. raise the interrupt if the descriptor IE and `DMACTRL.TI` are both set (REQ_GRSPW_154);
5. read the next descriptor.

REQ_GRSPW_134. A packet shall be considered sent when the peer has acknowledged it
(section 9) `[S]`. Until then TE stays 1, EN stays 1, and the guest keeps running.
Without a peer, the transmission never ends. An acknowledge received while no packet
is pending shall be ignored. The acknowledge does not identify its packet: when the
peer acknowledges a packet ended by an abort, a link error or a reset
(REQ_GRSPW_135, REQ_GRSPW_136, REQ_GRSPW_157) after a later packet has been sent, that
later packet is completed by it `[S]`.

### 7.3 Errors

REQ_GRSPW_135. A link error during a transmission (REQ_GRSPW_085) shall discard the rest of the
packet, complete the descriptor at once with LE set (REQ_GRSPW_133), then
`[M 77.5.7]`:

- if `DMACTRL.LE` = 0, continue with the next descriptors when the link is in the
  run state again;
- if `DMACTRL.LE` = 1, clear TE and stop.

REQ_GRSPW_136. Writing `DMACTRL.AT` = 1 shall truncate the current packet with an EEP, set
LE in its descriptor, and clear TE. With no transmission in progress, it shall only
clear TE `[M 77.5.7, 77.10.8]`. The packet driver stops a channel this way
`[P grspw_hw_dma_stop]`.

REQ_GRSPW_137. A memory access error while reading a descriptor or data shall stop the
transmitter: TE cleared, `DMACTRL.TA` set, interrupt if `DMACTRL.AI` is set. A packet
already started shall be ended with an EEP. The descriptor shall stay enabled, with
no error bit `[M 77.5.7]`.

REQ_GRSPW_138. A packet longer than the transport limit (section 9) shall be completed with
LE set and not sent `[S]`.

## 8. RMAP target

This section applies to RMAP cores with `CTRL.RE` = 1.

REQ_GRSPW_139. The target shall execute the RMAP commands of ECSS-E-ST-50-52C (write, read,
read-modify-write) on guest memory, and send the reply if the acknowledge bit is set
`[M 77.6.2]`.

REQ_GRSPW_140. The handling of each instruction shall follow this table `[M 77.6.6]`:

| Write | Verify | Ack | Incr | Command | Action |
|-------|--------|-----|------|---------|--------|
| 0 | 0 | 0 | - | Invalid | Nothing. No reply |
| 0 | 0 | 1 | 0 | Read, single address | Address word aligned, length multiple of 4, else error 10. Reply |
| 0 | 0 | 1 | 1 | Read, incrementing | No restriction. Reply |
| 0 | 1 | 0 | - | Invalid | Nothing. No reply |
| 0 | 1 | 1 | 0 | Invalid | Nothing. Reply with error 2 |
| 0 | 1 | 1 | 1 | Read-modify-write | See REQ_GRSPW_143. Reply |
| 1 | 0 | - | 0 | Write, single address | Address word aligned, length multiple of 4, else nothing done (error 10 if ack) |
| 1 | 0 | - | 1 | Write, incrementing | No restriction |
| 1 | 1 | - | - | Verified write | Length 0 to 4, else nothing done (error 9 if ack). Alignment as REQ_GRSPW_143, else error 10 |

REQ_GRSPW_141. The destination key shall be compared with `DKEY`. On a mismatch the
command shall not be executed, and the reply, if requested, shall carry error 3
`[M 77.6.2]`.

REQ_GRSPW_142. When several errors apply, the reported code shall be the first one in this
order `[M 77.6.2]`:

| Order | Code | Error |
|-------|------|-------|
| 1 | 2 | Unused packet type or command code |
| 2 | 3 | Invalid destination key |
| 3 | 9 | Verify buffer overrun |
| 4 | 11 | RMW data length error |
| 5 | 10 | Authorisation failure (unsupported option, alignment) |
| 6 | 1 | General error: memory error during a non-verified write |
| 7 | 5 / 7 | Early EOP / early EEP |
| 8 | 4 | Invalid data CRC |
| 9 | 1 | General error: memory error during a verified write or RMW |
| 10 | 7 | EEP |
| 11 | 6 | Cargo too large |

Errors 1 to 8 shall be checked before a verified write or a RMW is executed; the
later ones do not prevent its execution. Error code 12 is not implemented.

REQ_GRSPW_143. A verified write or a RMW shall make a single memory access of 1, 2 or 4
bytes: 1 byte at any address, 2 bytes halfword aligned, 4 bytes word aligned, 3 bytes
refused (error 10); the incrementing bit has no effect. A RMW accepts the data
lengths 0, 2, 4 and 8 (data plus mask), else error 11 `[M 77.6.3, 77.6.5]`.

REQ_GRSPW_144. A read reply shall carry no data when its status is not zero. An
authorisation failure shall be reported even for a zero length `[M 77.6.4]`.

REQ_GRSPW_145. Replies shall be sent before the packets of the DMA channel
`[M 77.6.2]`. `CTRL.RD` has no effect, since commands are executed one at a time
in arrival order `[S]`.

REQ_GRSPW_146. A memory error during a read shall end the reply early. The transport has no
EEP marker (section 9), so the reply is delivered truncated `[M 77.6.4] [S]`.

REQ_GRSPW_147. A read whose reply would exceed the transport limit shall be refused with
error 10 `[S]`.

## 9. External link

REQ_GRSPW_148. The SpaceWire link of all cores shall be carried by two POSIX message queues
of 10 messages of 8192 bytes `[S]`:

| Queue | Direction |
|-------|-----------|
| `/spwq` | Device to peer |
| `/devq` | Peer to device |

REQ_GRSPW_149. Each message shall be
`[core: u8][type: u8][header length: u8][header][data length: u32][data]`, with
these types `[S]`:

| Type | Meaning |
|------|---------|
| 0x00 ACK | Sent by the peer for each DATA packet it receives from the device |
| 0x01 DATA | A SpaceWire packet |
| 0x02 TIME | A time code |
| 0x03 RMAP | An RMAP reply from the device. Not acknowledged |

REQ_GRSPW_150. A packet shall be limited to 8185 bytes (message size minus the fixed
fields) `[S]`.

REQ_GRSPW_151. The device shall not block the guest while waiting for the queues: messages
from the peer are handled asynchronously, with the emulator lock held while guest
memory, registers and interrupts are accessed `[S]`.

## 10. Interrupts

REQ_GRSPW_152. Each core shall drive its own interrupt line (REQ_GRSPW_001). Each interrupt event
shall generate one pulse, as GRLIB cores do on the LEON3 interrupt controller `[S]`.

REQ_GRSPW_153. Link events shall interrupt when `CTRL.IE` is set and `[M 77.10.1]`:

- `CTRL.TQ` is set and a valid time code is received (REQ_GRSPW_091);
- `CTRL.LI` is set and a link error occurs (REQ_GRSPW_086).

Both drivers set IE whenever LI or TQ is set, and clear it otherwise
`[L grspw_control SET_LINK_ERR_IRQ, SET_TCODE_CTRL] [P grspw_link_ctrl, grspw_tc_ctrl]`.

REQ_GRSPW_154. DMA events shall interrupt independently of `CTRL.IE` `[M 77.10.8]`:

| Event | Condition |
|-------|-----------|
| Packet received | Descriptor IE and `DMACTRL.RI` |
| Packet sent (successful or not) | Descriptor IE and `DMACTRL.TI` |
| AHB error | `DMACTRL.AI` |

REQ_GRSPW_155. An interrupt shall be generated by the event, not by the state of PR or PS.
Setting RI or TI while PR or PS is already 1 shall not generate an interrupt. The
packet driver clears RI and TI in its handler and sets them again from its work task
before processing the descriptors, so it does not depend on this
`[P grspw_isr, grspw_dma_enable_int]`.

## 11. Reset

REQ_GRSPW_156. A system reset shall put every register to its reset value (section 3).
`NR` fields shall be 0 after power-on `[S]`.

REQ_GRSPW_157. Writing `CTRL.RS` = 1 shall reset the core like a system reset, except the
`NR` fields, and stop any transmission or reception in progress `[M 77.10.1]`.
The legacy driver writes `CTRL` = `RS` alone, then writes `STS` five times to let the
reset complete, then writes `CTRL` = `LS` `[L grspw_hw_reset]`: the reset shall be
complete when the write of RS returns.

REQ_GRSPW_158. After a reset, the link state shall be error-reset (0), then follow REQ_GRSPW_084.

REQ_GRSPW_159. The packet driver does not use `CTRL.RS`. It resets the core by register
writes, keeping the link, RMAP and port settings in case the RMAP target is in use
`[P grspw_hw_stop, grspw_hw_softreset]`:

- `DMACTRL` = `(DMACTRL & (LE | NS)) | AT`, then `DMACTRL & LE`;
- `CTRL` = `CTRL & (LD | LS | AS | RE | RD | NP | PS)`;
- `DMAMAXLEN` = 1024, `DMATXDESC` = `DMARXDESC` = 0;
- `STS` = 0xFFFFFFFF, `TIME` = 0.

These writes shall leave the link in its current state when LS or AS stays set.

## 12. Driver use cases

These sequences summarise how the drivers use the core. They shall work end to end,
and the test suite covers them.

### 12.1 Legacy driver (`grspw.c`)

| Step | Driver action | Device behaviour required |
|------|---------------|---------------------------|
| Open | `CTRL` = RS; `STS` cleared ×5; `CTRL` = LS; reads `NODEADDR`, `DKEY`, `CLKDIV`, `CTRL`, `TDR` | REQ_GRSPW_157, REQ_GRSPW_084, reset values |
| Start | Clears `STS`; waits `STS.LS` = 5 (10 ticks); `DMACTRL` = PS\|PR\|TA\|RA and expects 0; 128 RX descriptors IE\|EN, WR on the last; writes `DMAMAXLEN`, `DMATXDESC`, `DMARXDESC`; `DMACTRL` = RD\|RE\|NS\|TI\|RI | REQ_GRSPW_084, REQ_GRSPW_072, REQ_GRSPW_074, REQ_GRSPW_082 |
| Send | Fills word 1 to 3, then word 0 = IE\|EN\|hlen [\|HC\|DC\|NONCRCLEN] [\|WR]; `DMACTRL` \|= TE\|TI | REQ_GRSPW_116 to REQ_GRSPW_123, REQ_GRSPW_129 to REQ_GRSPW_134 |
| Interrupt | Reads `STS`, writes back the error bits; on TO, clears it and reads `TIME`; writes back `DMACTRL` as read; scans the TX descriptors for EN = 0 and LE; on PR, wakes the reader | REQ_GRSPW_042, REQ_GRSPW_073, REQ_GRSPW_133, REQ_GRSPW_118, REQ_GRSPW_121, REQ_GRSPW_108 |
| Receive | Waits for EN = 0; error if EP or TR (or HC/DC); copies from offset 1 (or 0 / 2); re-enables the descriptor and writes RD\|RE\|RI\|NS | REQ_GRSPW_094 to REQ_GRSPW_101, REQ_GRSPW_103, REQ_GRSPW_108, REQ_GRSPW_110 |
| Stop | Clears RE, RI, RD, TE, TI; waits for `DMACTRL.RX` = 0 | REQ_GRSPW_073, REQ_GRSPW_060 |

### 12.2 Packet driver (`grspw_pkt.c`)

| Step | Driver action | Device behaviour required |
|------|---------------|---------------------------|
| Probe | Reads the capabilities in `CTRL`; reads `ICCTRL`; writes `TDR`; probes `ICRLPRESC`; stops and soft-resets the core | REQ_GRSPW_010 to REQ_GRSPW_015, REQ_GRSPW_029, REQ_GRSPW_008, REQ_GRSPW_054, REQ_GRSPW_055, REQ_GRSPW_159 |
| Open | Allocates both tables in a 1 KiB aligned area (RX, then TX); attaches the interrupt | REQ_GRSPW_076, REQ_GRSPW_079 |
| Link | `CTRL` = `(CTRL & ~(LI\|LD\|LS\|AS))` \| options, IE if LI or TQ; `CLKDIV` | REQ_GRSPW_084, REQ_GRSPW_153, REQ_GRSPW_045, REQ_GRSPW_046 |
| DMA start | `DMATXDESC`, `DMARXDESC`, `DMAMAXLEN`; `DMACTRL` = AI\|PS\|PR\|TA\|RA\|RE \| NS [\|LE] [\|RI] [\|TI] | REQ_GRSPW_073, REQ_GRSPW_107 |
| RX prepare | Up to 128 descriptors EN [\|IE], WR on entry 127; `DMACTRL` \|= RE\|RD | REQ_GRSPW_107, REQ_GRSPW_110, REQ_GRSPW_080 |
| TX send | Up to 64 descriptors, control bits copied in word 2 bits 29:24, WR on entry 63; `DMACTRL` \|= TE | REQ_GRSPW_116 to REQ_GRSPW_134 |
| Interrupt | Reads `STS`, clears the selected bits; reads `ICCTRL`; for the channel, events = (PR if RI) \| (PS if TI) \| RA \| TA; writes `DMACTRL` with RI/TI cleared; work task re-enables them and clears PR/PS | REQ_GRSPW_042, REQ_GRSPW_154, REQ_GRSPW_155, REQ_GRSPW_008 |
| DMA stop | `DMACTRL` = kept bits \| AT | REQ_GRSPW_136 |

## 13. Out of scope

- GRSPW2 features: several DMA channels, per-channel node address and mask, address
  and protocol identifier stripping, distributed interrupts, CCSDS and ISO CRC.
- Two ports (`CTRL.PS`, `CTRL.NP`, `STS.AP`).
- Link timing, clock divisors, timers, character-level errors (parity, escape,
  credit, disconnect) other than by fault injection.
- `CTRL.RD` (one RMAP buffer instead of several).
- Packets longer than 8185 bytes.

## 14. Open points

None.
