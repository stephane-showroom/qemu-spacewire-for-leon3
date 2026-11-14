# GRSPW device model: test plan

This plan defines how each requirement of [SPECIFICATION.md](SPECIFICATION.md) is
verified. It lists the test cases, says which ones already exist in `code/test` and
which ones are to be written, and ends with the traceability matrix from every
`REQ_GRSPW_XXX` to its test cases.

## 1. Conventions

Each test case has a tag `TC_GRSPW_XXX`, where `XXX` is a three-digit number. Like the
requirement tags, a test case tag is never reused.

Verification methods:

| Method | Meaning |
|--------|---------|
| T | Test: an automated test case of `code/test` checks the requirement |
| A | Analysis: the requirement follows from the design and is argued in the test case |
| I | Inspection: the requirement is checked by reading the code |

Test case status:

| Status | Meaning |
|--------|---------|
| Existing | Implemented in `code/test` before this plan, and run by `make -C code/test test` |
| New | Implemented by the requirement files of section 5.1 |
| Blocked | Cannot be implemented until the missing facility of section 8 exists |
| Deleted | Replaced by other test cases; the tag is not reused |

## 2. Test levels

| Level | What runs in the guest | Source | What it shows |
|-------|------------------------|--------|---------------|
| Register | RTEMS application that accesses the registers and descriptors directly | `grspw_raw.c` | Behaviour that the drivers do not reveal, at bit level |
| Legacy driver | RTEMS application using `grspw.c` through `open`/`read`/`write`/`ioctl` | `grspw_legacy.c` | The use case of section 12.1 of the spec |
| Packet driver | RTEMS application using `grspw_pkt.c` with interrupts and its work task | `grspw_pkt_irq.c`, `rtems_spw_echo` | The use case of section 12.2 of the spec |
| Peer | Linux program on the other end of the link | `device_simu`, `peer_*.c`, `rmap_test.c` | What the device sends, and its reaction to what it receives |
| System | The demonstration | `code/demo` | Everything together, checked by hand |

A register-level or driver-level test is built once per core (`-DCORE=<n>`) and run on
each core it applies to. RMAP tests run on cores 2 and 3 only, and their negative
counterparts (RMAP absent) on cores 0 and 1.

## 3. Environment

- The `localhost/qemu-dev-plugin` container, with QEMU rebuilt by
  `code/integrate_qemu.sh`.
- `qemu-system-sparc -M leon3_generic` from `/workspace/qemu/build`, started by
  `code/test/run_tests.sh` with a 10-second timeout per test.
- The message queues `/spwq` and `/devq`, deleted before and after each test. No other
  QEMU may use them while the suite runs.

## 4. Pass criteria

- A guest test passes when its console shows `TEST OK` (or the expected pattern of the
  application). It prints `TEST FAILED: <reason>` and exits otherwise.
- A peer that checks the device (e.g. `rmap_test`) reports its failures in its log and
  makes the guest fail by not sending the expected final packet.
- The suite passes when there is no `FAIL` and no `XPASS`. A test that fails because of
  a known deviation from the spec is marked `known` and reported `XFAIL`, with the
  reason: in `run_tests.sh` for the existing tests, with an `@known` line for the
  requirement files.
- Each check of a requirement file prints `CHECK FAILED: <check>` when it fails, so
  that the log says which part of the requirement is not met.

## 5. Test infrastructure

| Item | Purpose | Source |
|------|---------|--------|
| `peer_script` | A scriptable peer: acknowledge at once, after a delay or never; send any packet, a numbered series of packets or a time code; send an RMAP write; report what the device sent back to the guest as a data packet, so that every check stays in the guest | `code/test/peer_script.c` |
| Per-core interrupt counters | `count_irq` installed on the four IRQ lines (10 to 13), each with its own counter | `req/req_guest.h`, REQ_GRSPW_152 |
| AMBA plug and play reader | Walk the APB plug and play records from the guest with `ambapp_for_each` | REQ_GRSPW_001, 002 |
| Unmapped address | 0x10000000, outside RAM and devices | REQ_GRSPW_114, 137, 146 |

### 5.1 Requirement files

The test cases marked New are implemented in `code/test/req`, with **one file per
requirement**: `REQ_GRSPW_XXX.c` holds only the checks of `REQ_GRSPW_XXX`. A test case
of section 6 is therefore implemented by the files of the requirements it covers, and
each file names its test case in its `@tc` line.

The header comment of each file says how it runs. The Makefile and `run_tests.sh` read
these lines, so a new file needs no other change:

| Line | Meaning |
|------|---------|
| `@tc TC_GRSPW_XXX` | Test case(s) of this plan |
| `@side guest` | RTEMS program built from the file with `req/req_guest.h`, once per core |
| `@side peer` | Linux program built with `req/req_peer.h`, run against the guest `req/peer_guest.c`; the checks are in the peer, which tells the guest the result |
| `@cores 0 1 2 3` | Cores the test runs on |
| `@peer <command>` | `none`, `device_simu`, or `peer_script <steps>` (guest side only) |
| `@known <cores>: <reason>` | Known deviation of the model on these cores (`all` for every core): reported `XFAIL` |

A test result is named after the file and the core, e.g. `REQ_GRSPW_088.2`. To run a
subset: `ONLY='REQ_GRSPW_03*' ./run_tests.sh <qemu> <ram_mb>`.

## 6. Test cases

### 6.1 Smoke

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_001 | `hello` | System | - | - | Existing |

**TC_GRSPW_001: boot.** Boots the minimal RTEMS application. Proves the environment
before any GRSPW test. No requirement.

### 6.2 Configuration and registers

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_002 | `pnp_scan` | Register | 0 | - | New |
| TC_GRSPW_003 | `boot_values` | Register | 0 to 3 | - | New |
| TC_GRSPW_004 | `reset_values` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_005 | `register_map` | Register | 0 to 3 | - | New |
| TC_GRSPW_006 | - | - | - | - | Deleted |
| TC_GRSPW_007 | - | - | - | - | Deleted |
| TC_GRSPW_008 | `status_clear` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_009 | `dmactrl_bits` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_010 | `descriptor_wrap` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_058 | `ctrl_fields` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_059 | `sts_fields` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_060 | `config_fields` | Register | 0 to 3 | - | New |
| TC_GRSPW_061 | `dmactrl_fields` | Register | 0 to 3 | `device_simu`, `peer_script` | New |
| TC_GRSPW_062 | `table_fields` | Register | 0 to 3 | `device_simu` | New |

**TC_GRSPW_002: plug and play.** Walk the APB plug and play table.
Expected: exactly four records with vendor 0x01 and device 0x1F, at 0x80000A00,
0x80000B00, 0x80000C00, 0x80000D00, with IRQ 10 to 13 and a 256-byte bar. Read `CTRL`
of each core: PO = 0 and bits 28:27 = 0 (one port, one DMA channel).

**TC_GRSPW_003: power-on values.** First action after boot, before any write to the
core: read every register.
Expected: the reset values of section 3 of the spec, with the `NR` fields at 0,
`TDR` = 0x0001F100, `CLKDIV` = 0x0404, `NODEADDR` = 254, and on cores 2 and 3
RA = RX = RC = RE = 1 and AS = 1. The AS check fails on the current model (open point,
section 9): mark it `known` until the point is decided.

**TC_GRSPW_004: reset by CTRL.RS (existing `reset_values`).** Sets non-reset fields,
writes RS with LI and TQ.
Expected: LI, TQ, AI, RI, TI, `DMAMAXLEN` and the table bases keep their values; the
other fields are back to their reset values; `STS.LS` = 0; the selectors are 0.

**TC_GRSPW_005: register map.** Write a pattern to 0x1C and to every word from 0x30 to
0xFC, then read them back; then make byte and halfword reads and writes on `CTRL`,
`NODEADDR` and `TIME`.
Expected: the unused offsets read 0 and the defined registers are unchanged by the
pattern writes. The result of the partial accesses is undefined (manual 77.7.1):
they do not stop QEMU, later 32-bit accesses work, and the test reaches
`TEST OK`.

**TC_GRSPW_006 and TC_GRSPW_007: deleted.** They checked the reserved bits and the read
back of all registers at once. Each field now has its own requirement, checked by
TC_GRSPW_058 to TC_GRSPW_062.

**TC_GRSPW_008: clearing STS.** Produce IA (packet to a wrong address), EE (1-byte
packet from the peer) and TO (time code from the peer) with the link in run, then write
0xFFFFFFFF to `STS`.
Expected: IA, EE and TO are 0; `STS.LS` is still 5.

**TC_GRSPW_009: DMACTRL access rules.** With no reception in progress write
`PS | PR | TA | RA` and read; write `TE` with the current TX descriptor disabled; write
`RE | RD` twice in a row; read `RX` between packets.
Expected: the first read gives 0; TE reads 0 again and no packet reaches the peer; the
second write of RE | RD is accepted; RX reads 0; bits 15:13 read 0 after a write of
ones.

**TC_GRSPW_010: descriptor tables.** Enable all 128 RX and 64 TX descriptors without WR,
send 130 packets through the loop of `device_simu`; then repeat with WR on RX
descriptor 3 and TX descriptor 1; then start with `DMARXDESC` written with DESCSEL = 5.
Expected: `DESCSEL` advances by 8 (RX) and 16 (TX) per packet, returns to 0 after
entry 127 / 63 and after a descriptor with WR, and reception starts at entry 5 in the
last run.

TC_GRSPW_058 to TC_GRSPW_062 check each field of section 3 of the spec on its own, one
requirement file per field and one per group of reserved bits. The checks depend on the
access type of the field:

| Access | Checks |
|--------|--------|
| `r` | Same value at power-on, after `CTRL.RS`, after a write of ones and after a write of zeros |
| `rw` | Power-on value; reset value after `CTRL.RS`; several values written and read back; after a write of another value then `CTRL.RS`, the reset value again |
| `rw`, `NR` | 0 at power-on; values written and read back; the value kept through `CTRL.RS` |
| `wc` | 0 at power-on; a write of 1 does not set it; set by its event; unchanged by a write of 0 and by a write of 1 to the other status bits; cleared by a write of 1; 0 again after `CTRL.RS` |
| `sc` | Reads 0 after a write of 1, and the action took place |
| reserved | 0 at power-on, after a reset and after a write of ones |

The events of the `wc` bits are: a 1-byte packet (EE), a packet for another node (IA), a
time code (TO), a packet stored (PR) or sent (PS), a buffer or descriptor at an unmapped
address (RA, TA). WE, PE, DE, ER and CE cannot be set without fault injection: their
files check the other rules, and setting them is left to TC_GRSPW_013 (blocked).

**TC_GRSPW_058: CTRL fields.** REQ_GRSPW_010 to 029. RA, RX, RC: 1 on cores 2 and 3, 0 on
cores 0 and 1. RE and RD read 0 on cores 0 and 1. LD and LS: `CTRL[1:0]` reads 01b after
a link disable and 10b after a link start, as the legacy driver checks. TI: a tick-in
with TT sends a time code (`peer_script wait report`). RS: the write resets `NODEADDR`.

**TC_GRSPW_059: STS fields.** REQ_GRSPW_030 to 040. `STS.LS` reads 0 after LD and 5 after
LS, whatever is written. For EE, IA and TO the peer sends the event twice, each time
after a packet of the guest, followed by a marker packet that the guest waits for.

**TC_GRSPW_060: NODEADDR, CLKDIV, DKEY, TIME and TDR fields.** REQ_GRSPW_043 to 052 and
054 to 056. Reset values 254, 0x04 for both divisors, 0, 0, and 31 and 256 for `TDR` at
40 MHz. `DESTKEY` reads 0 on cores 0 and 1.

**TC_GRSPW_061: DMACTRL fields.** REQ_GRSPW_057 to 071. RD is cleared by a packet that
finds its descriptor disabled. TE reads 1 while a packet waits for its acknowledge
(the peer waits 1 s), and is cleared on the next, disabled, descriptor. AT reads 0.

**TC_GRSPW_062: DMAMAXLEN and descriptor table fields.** REQ_GRSPW_074 to 081. The
selectors advance by one descriptor per packet, wrap after 64 (TX) and 128 (RX) and
after a descriptor with WR, and the core starts from the selector written (TX
descriptor 3, RX descriptor 5).

### 6.3 Link interface

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_011 | `link_fsm` | Register | 0 to 3 | - | New |
| TC_GRSPW_012 | `link_error_tx` | Register | 0 to 3 | `peer_script` (no ack) | New |
| TC_GRSPW_013 | `link_error_bits` | Register | 0 to 3 | - | Blocked |

**TC_GRSPW_011: link state machine.** From a reset, write in turn: LS; LD; LD | LS;
0; AS; 0; then reset with RS.
Expected `STS.LS` after each write: 5, 0, 0, 0 (unchanged), 5, 5 (unchanged), 0.

**TC_GRSPW_012: link error during a transmission.** The peer never acknowledges. Start
a packet, write LD while it is pending, then LS.
Expected with `DMACTRL.LE` = 0: the descriptor completes with LE = 1, PS is set, and the
next enabled descriptor is transmitted once the link is back in run. Repeat with
`DMACTRL.LE` = 1: TE is cleared and the next descriptor stays enabled.

**TC_GRSPW_013: character-level errors (blocked).** Needs the fault injection of
section 8. Inject each of WE, PE, DE, ER, CE.
Expected: the bit is set in `STS`; with `CTRL.LI` and `CTRL.IE` an interrupt is
raised; with the packet driver and `LINKOPTS_DIS_ON_*`, the link goes to error-reset.

### 6.4 Time codes

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_014 | `time_codes` | Register | 0 to 3 | `peer_time_codes` | Existing |
| TC_GRSPW_015 | `tick_in` | Register | 0 to 3 | `peer_script` (record) | New |
| TC_GRSPW_016 | `time_code_rx_rules` | Register | 0 to 3 | `peer_script` | New |

**TC_GRSPW_014: reception (existing `time_codes`).** The peer sends 62, 63, 0 with
`TIME` = 61, TR, TQ and IE set.
Expected: three interrupts, `TIME.TIMECNT` = 0, TO set.

**TC_GRSPW_015: transmission.** Set `TIME` = 0x7E (TCTRL = 01b, counter 62), then tick
in with TT = 1 three times; then tick in with TT = 0; then write `TIME` directly; then
set TT and TR together and tick in.
Expected: the peer records three TIME messages 0x7F, 0x40, 0x41 (counter wraps to 0,
TCTRL kept), nothing for the tick-in with TT = 0 and nothing for the direct write
(counter unchanged by the TT = 0 tick-in), one message for the last tick-in. `CTRL.TI`
reads 0 after each write.

**TC_GRSPW_016: reception rules.** With TR = 0 the peer sends a time code; then, with
TR, TQ and IE set and `TIME` = 0x10, the time codes 0xD1 (valid: counter 0x11), 0x45
(not valid: counter 5) and 0x86 (valid: counter 6 after 5).
Expected: the first one leaves `TIME` and TO unchanged. A valid time code stores the
counter and the control flags, sets TO and interrupts; a time code that is not valid
stores the counter only: TCTRL unchanged, TO not set, no interrupt (manual 77.3.5).

### 6.5 Receive DMA

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_017 | `header` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_018 | `invalid_address` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_019 | `promiscuous` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_020 | `rx_routing` | Register | 2, 3 | `peer_script` | New |
| TC_GRSPW_021 | `no_rmap`, `rmap_disabled` | Register | 0, 1 / 2, 3 | `device_simu` | Existing |
| TC_GRSPW_022 | `short_packets` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_023 | `rx_spill` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_024 | `rx_no_spill` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_025 | `rx_kept_queue` | Register | 0 to 3 | `peer_script` | New |
| TC_GRSPW_026 | `rx_truncate` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_027 | `rx_descriptor_status` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_028 | `rx_crc_flags` | Register | 2, 3 | `peer_script` | New |
| TC_GRSPW_029 | `rx_eep` | Register | 0 to 3 | - | Blocked |
| TC_GRSPW_030 | `rx_ahb_error` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_063 | `rx_descriptor_fields` | Register | 0 to 3 | `device_simu`, `peer_script` | New |

**TC_GRSPW_017: packet layout (existing `header`).** Header and data in two buffers.
Expected: the received packet starts with the address and protocol identifier, then
the data, with the length of the whole packet.

**TC_GRSPW_018: address mismatch (existing `invalid_address`).**
Expected: IA set, the descriptor stays enabled.

**TC_GRSPW_019: promiscuous mode.** With PM = 1 the peer sends a packet to another
address; on cores 2 and 3 it also sends an RMAP write command with RE = 1.
Expected: the packet is stored, IA is not set; the RMAP command is executed by the
target, not stored.

**TC_GRSPW_020: what goes to the RMAP target.** With RE = 1 the peer sends: an RMAP
reply (type 00b), a packet with the extended protocol identifier (0x00 0x00 0x01), a
packet with protocol 0x01 and type 10b, then an RMAP command.
Expected: the first three are stored in the DMA channel; only the command is executed.

**TC_GRSPW_021: RMAP-like packet to the DMA channel (existing `no_rmap`,
`rmap_disabled`).** An RMAP read command with a wrong header CRC.
Expected: stored unchanged; HC = DC = 0 on cores 0 and 1; HC = DC = 1 on cores 2 and 3
with RE = 0.

**TC_GRSPW_022: short packets.** The peer sends a 1-byte packet; on cores 2 and 3 with
RE = 1 it sends a 2-byte packet with protocol 0x01; on all cores a 2-byte packet with
another protocol.
Expected: the first two are discarded and set EE; the last one is stored.

**TC_GRSPW_023: spill rules.** One packet per row of the table of REQ_GRSPW_107: RE = 0;
RE = 1, RD = 0, NS = 0; RE = 1, RD = 1, next descriptor disabled, NS = 0. Then clear RE
and send again.
Expected: all discarded, no descriptor written; RD cleared in the third case.

**TC_GRSPW_024: no spill (existing `rx_no_spill`).** NS = 1 and no descriptor when the
packet arrives, then a descriptor and RD.
Expected: the packet is stored once the descriptor is enabled.

**TC_GRSPW_025: queue of kept packets.** NS = 1, no descriptor. The peer sends 66
numbered packets. Then enable 66 descriptors and write RD.
Expected: packets 1 to 64 are stored in order; 65 and 66 are discarded.

**TC_GRSPW_026: truncation.** `DMAMAXLEN` = 16, packet of 40 bytes.
Expected: length 16, TR = 1, the 16 first bytes stored and nothing written after them;
a packet of exactly 16 bytes has TR = 0.

**TC_GRSPW_027: descriptor completion and buffer alignment.** Descriptors with IE and
WR set; then a buffer at an address ending in 01b, with guard bytes around it.
Expected: after reception EN = 0, IE and WR kept, length and status in word 0. On cores
2 and 3 the packet starts at the given byte and the guard bytes are untouched; on
cores 0 and 1 the packet starts at the word boundary below.

**TC_GRSPW_028: receive CRC flags.** The peer sends: a valid RMAP reply (correct header
and data CRC); one with a bad header CRC; one with a bad data CRC; a non-RMAP packet
ending with its RMAP CRC.
Expected: HC/DC = 0/0, 1/1, 0/1, and DC = 0 for the last one.

**TC_GRSPW_029: EEP (blocked).** Needs an EEP marker in the queue format (section 8).
Expected: a packet ended by an EEP is stored with EP = 1.

**TC_GRSPW_030: memory error on reception.** RX descriptor pointing at an unmapped
address, AI = 1.
Expected: RA set, RE cleared, one interrupt, the next packet is discarded until RE is
set again.

**TC_GRSPW_063: receive descriptor fields.** REQ_GRSPW_094 to 102, one file per field of
the descriptor. The status fields written by the core (TR, DC, HC, EP, PACKETLENGTH) are
preset by the guest to a wrong value, to check that the core writes them.
Expected: TR set only for a packet longer than `DMAMAXLEN` (8 bytes); DC and HC from the
CRC check on cores 2 and 3, 0 on cores 0 and 1 (same four packets as TC_GRSPW_028); EP
0 for a packet ended by an EOP; an interrupt with IE and RI, none without IE; after a
descriptor with WR, the next packet in descriptor 0; a disabled descriptor neither
written nor used, RD cleared; lengths 2, 5, 100 and 1024 written; the packet at the
address of word 1, which the core does not change. IE and WR are kept in word 0.

### 6.6 Transmit DMA

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_031 | `tx_burst` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_032 | `tx_crc` | Register | 2, 3 | `device_simu` | Existing |
| TC_GRSPW_033 | `tx_reserved_bits` | Register | 0 to 3 | `peer_script` (record) | New |
| TC_GRSPW_034 | `tx_empty_parts` | Register | 0 to 3 | `peer_script` (record) | New |
| TC_GRSPW_035 | `tx_pending` | Register | 0 to 3 | `peer_script` (delayed ack) | New |
| TC_GRSPW_036 | `tx_abort` | Register | 0 to 3 | `peer_script` (no ack) | New |
| TC_GRSPW_037 | `tx_ahb_error` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_038 | `tx_too_long` | Register | 0 to 3 | `peer_script` (record) | New |
| TC_GRSPW_064 | `tx_descriptor_fields` | Register | 0 to 3 | `device_simu`, `peer_script` (record) | New |

**TC_GRSPW_031: descriptor chain (existing `tx_burst`).** Several descriptors enabled,
one write of TE.
Expected: all packets sent and received, TE cleared on the first disabled descriptor.

**TC_GRSPW_032: CRC generation (existing `tx_crc`).** HC, DC and NONCRCLEN = 1.
Expected: header CRC after the header, not covering the first byte; data CRC after the
data.

**TC_GRSPW_033: reserved descriptor bits.** Word 2 with bits 29:24 set (as the packet
driver writes it) and word 0 with bits 19:18 set, HC and DC on cores 2 and 3.
Expected: the recorded packet has DATALEN bytes of data and RMAP CRCs.

**TC_GRSPW_034: empty header or data.** Descriptors with: HEADERLEN = 0 and DC; DATALEN =
0 and HC and DC; both lengths 0.
Expected on cores 2 and 3: no header CRC when HEADERLEN = 0; a 0x00 data CRC when
DATALEN = 0. On all cores: nothing is sent for both lengths 0, but the descriptor is
completed and PS is set.

**TC_GRSPW_035: pending transmission.** The peer acknowledges after 1 s.
Expected: during that second EN and TE stay 1 and the guest keeps running (a counter
task advances); after the acknowledge the descriptor completes.

**TC_GRSPW_036: abort.** The peer never acknowledges. Start a packet, write AT; then
write AT with nothing in progress.
Expected: LE set in the descriptor, TE cleared; the second write only clears TE.

**TC_GRSPW_037: memory error on transmission.** A TX descriptor whose data address is
unmapped, AI = 1; then a TX descriptor table at an unmapped base.
Expected: TA set, TE cleared, one interrupt each time, the descriptor stays enabled
with no error bit.

**TC_GRSPW_038: transport limit.** Packets of 8185 and 8186 bytes.
Expected: the first one is recorded by the peer; the second one completes with LE = 1
and nothing reaches the peer.

**TC_GRSPW_064: transmit descriptor fields.** REQ_GRSPW_116 to 128, one file per field of
the descriptor. The peer reports each packet (`peer_script wait report ack`).
Expected: with DC or HC, the data or header CRC is sent on cores 2 and 3 only, and
without them no CRC; the header CRC skips the first NONCRCLEN bytes (0, 1, 3); LE preset
by the guest is 0 after a packet sent without error; an interrupt with IE and TI, none
otherwise; after a descriptor with WR the next descriptor is descriptor 0; a disabled
descriptor stops the transmitter and sends nothing; header lengths 2, 3, 255 and 0 and
data lengths 1, 7 and 1000 are sent; header and data are read from unaligned
addresses; the reserved bits of words 0 and 2 are ignored. The core keeps every field
of word 0 but EN and LE, and does not change words 1 to 3.

### 6.7 RMAP target

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_039 | `rmap` | Register + peer | 2, 3 | `rmap_test` | Existing |
| TC_GRSPW_040 | `rmap_matrix` | Register + peer | 2, 3 | `rmap_test` (extended) | New |
| TC_GRSPW_041 | `rmap_error_order` | Register + peer | 2, 3 | `rmap_test` (extended) | New |
| TC_GRSPW_042 | `rmap_large_read` | Register + peer | 2, 3 | `rmap_test` (extended) | New |
| TC_GRSPW_043 | - | Analysis | 2, 3 | - | New |

**TC_GRSPW_039: RMAP target (existing `rmap`).** `rmap_test` checks its CRC against
the patterns of the standard, then: incrementing, verified, unaligned and
single-address writes and reads; write without reply; RMW and RMW of length 6; invalid
key; bad data CRC; bad header CRC and incomplete header (no reply); unaligned single
read; verified write of 3 and 5 bytes; unused command; early EOP; too much data; read
with data; write and read of unmapped memory; extended address; target disabled on the
other core; reply address.
Expected: the reply status and data given in `rmap_test.c` for each case, and the
final write visible in guest memory.

**TC_GRSPW_040: remaining command codes.** The rows of REQ_GRSPW_140 not covered by
TC_GRSPW_039: codes 0/0/0/x and 0/1/0/x (no reply, nothing done); verified write
without ack of 5 bytes (nothing done); single-address write without ack, unaligned
(nothing done); RMW of 1 and 2 bytes, aligned and unaligned; read of length 0 with an
authorisation failure.
Expected: the action and reply of the table; read-back of memory after each command.

**TC_GRSPW_041: error priority.** Commands with two errors each: unused code and wrong
key; wrong key and verify overrun; verify overrun and RMW length error; authorisation
failure and bad data CRC; early EOP and bad data CRC.
Expected: the code of the first error in the order of REQ_GRSPW_142.

**TC_GRSPW_042: large read.** A read whose reply needs more than 8185 bytes, and one
that just fits.
Expected: error 10 and no data for the first; the data for the second.

**TC_GRSPW_043: reply priority (analysis).** The model executes a command and sends its
reply before handling the next message from the queue, while DMA transmissions wait for
an acknowledge. Argue from the code that a reply cannot be queued behind a DMA packet,
and that `CTRL.RD` has no effect.

### 6.8 External link

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_044 | `device_simu_queues` | Peer | - | `queue_test` | Existing |
| TC_GRSPW_045 | `data_before_ack` | Legacy driver | 0 to 3 | `peer_data_before_ack` | Existing |

**TC_GRSPW_044: queue format (existing).** `queue_test` exchanges messages with
`device_simu` alone.
Expected: queue attributes and message format of REQ_GRSPW_148 and REQ_GRSPW_149.

**TC_GRSPW_045: packet before the acknowledge (existing).** The peer sends its answer
before acknowledging the guest's packet.
Expected: both the reception and the end of transmission are handled; the guest is not
blocked.

### 6.9 Interrupts

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_046 | `rx_ri_only` | Register | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_047 | `tx_interrupt` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_048 | `irq_edge` | Register | 0 to 3 | `device_simu` | New |
| TC_GRSPW_049 | `irq_per_core` | Register | all at once | `device_simu` | New |

**TC_GRSPW_046: receive interrupt (existing `rx_ri_only`).** Descriptor IE and RI, with
`CTRL.IE` = 0.
Expected: one interrupt.

**TC_GRSPW_047: transmit interrupt.** Four packets: descriptor IE with TI, IE without
TI, TI without IE, neither; then one completed with LE (abort, as TC_GRSPW_036) with IE
and TI.
Expected: exactly two interrupts (first and last), PS set in all cases.

**TC_GRSPW_048: event, not level.** Receive a packet with IE and RI = 0, so PR = 1 and
no interrupt; then set RI.
Expected: no interrupt when RI is set; one interrupt at the next packet.

**TC_GRSPW_049: one line per core.** One packet with interrupt on each core in turn.
Expected: only the counter of the IRQ line of that core increments.

### 6.10 Reset

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_050 | `reset_in_flight` | Register | 0 to 3 | `peer_script` (no ack) | New |
| TC_GRSPW_051 | `pkt_softreset` | Register | 0 to 3 | `device_simu` | New |

**TC_GRSPW_050: reset during a transfer.** A transmission waiting for its acknowledge,
and NS = 1 with a kept packet; write RS; then the peer acknowledges.
Expected: TE = 0, the late acknowledge does not complete any descriptor, the kept
packet is not stored after the reset.

**TC_GRSPW_051: packet driver soft reset.** With the link in run and RE set on cores 2
and 3, apply the register sequence of REQ_GRSPW_159.
Expected: `STS.LS` stays 5; RE, LS, AS kept; `DMACTRL` = LE only; `DMAMAXLEN` = 1024;
the selectors are 0; `STS` error bits and `TIME` cleared. On cores 2 and 3, an RMAP
command is still executed afterwards.

### 6.11 Driver use cases

| Tag | Runner name | Level | Cores | Peer | Status |
|-----|-------------|-------|-------|------|--------|
| TC_GRSPW_052 | `irq_rx_blocking`, `many_packets`, `stop_start`, `clear_ctrl_bit`, `max_packet` | Legacy driver | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_053 | `echo_pkt` | Packet driver | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_054 | `pkt_irq` | Packet driver | 0 to 3 | `device_simu` | Existing |
| TC_GRSPW_055 | `all_cores` | Register | all at once | `device_simu` | Existing |
| TC_GRSPW_056 | `demo` | System | 0, 2 | `code/demo/simu` | Existing, manual |
| TC_GRSPW_057 | `legacy_config` | Legacy driver | 0 to 3 | - | New |

**TC_GRSPW_052: legacy driver.** Open, start, blocking exchanges with interrupts (10,
then 300 to wrap the 64 TX descriptors), stop and start again, link error interrupt on
and off, a packet of 1024 bytes.
Expected: every exchange returns the packet sent; `packets_sent` counts every transmit
interrupt.

**TC_GRSPW_053: packet driver without interrupts (existing `echo_pkt`).** Probe, open,
one exchange by polling.
Expected: `SPW ECHO OK`.

**TC_GRSPW_054: packet driver with interrupts (existing `pkt_irq`).** Eight packets
sent as one list, reception waited on with `grspw_dma_rx_wait`.
Expected: the eight packets received in order, through the work task only.

**TC_GRSPW_055: four cores at once (existing `all_cores`).** A different packet on each
core at the same time.
Expected: each packet comes back on its own core.

**TC_GRSPW_056: demonstration.** `code/demo/demo.sh`, by hand.
Expected: `INIT` → `WARMUP` → `OPERATIONAL` driven by RMAP, IMU data printed; with
`--imu-fail 20`, `FAIL` after 3 s without data.

**TC_GRSPW_057: legacy configuration ioctls.** The ioctls that read back registers:
`SET_NODEADDR`, `SET_CLKDIV`, `SET_CLKDIVSTART`, `SET_DESTKEY`, `SET_PROMISCUOUS`,
`SET_RMAPEN`, `SET_RMAPBUFDIS`, `SET_COREFREQ`, `LINKDISABLE`, `LINKSTART`,
`SET_TCODE_CTRL`, `GET_TCODE`.
Expected: success on all cores, except `SET_RMAPEN` = 1, `SET_RMAPBUFDIS` = 1 and a
non-zero `SET_DESTKEY` on cores 0 and 1, which return `RTEMS_IO_ERROR`.

## 7. Traceability matrix

| Requirement | Subject | Method | Test cases |
|-------------|---------|--------|------------|
| REQ_GRSPW_001 | Four cores, addresses, IRQs | T | TC_GRSPW_002, 049, 055 |
| REQ_GRSPW_002 | Plug and play record | T | TC_GRSPW_002 |
| REQ_GRSPW_003 | RMAP cores configuration | T | TC_GRSPW_003, 021, 032 |
| REQ_GRSPW_004 | One port, one DMA channel | T | TC_GRSPW_002, 006 |
| REQ_GRSPW_005 | Input signal values | T | TC_GRSPW_003, 004 |
| REQ_GRSPW_006 | Big-endian byte order | T, A | TC_GRSPW_017, 039 |
| REQ_GRSPW_007 | Register offsets | T | TC_GRSPW_005 |
| REQ_GRSPW_008 | Unused offsets read 0 | T | TC_GRSPW_005, 053 |
| REQ_GRSPW_009 | Partial accesses: undefined, no crash | T | TC_GRSPW_005 |
| REQ_GRSPW_010 | CTRL.RA | T | TC_GRSPW_058, 003, 053 |
| REQ_GRSPW_011 | CTRL.RX | T | TC_GRSPW_058, 003, 053 |
| REQ_GRSPW_012 | CTRL.RC | T | TC_GRSPW_058, 003, 053 |
| REQ_GRSPW_013 | CTRL.PO | T | TC_GRSPW_058, 002, 053 |
| REQ_GRSPW_014 | CTRL.PS | T | TC_GRSPW_058, 002 |
| REQ_GRSPW_015 | CTRL.NP | T | TC_GRSPW_058, 002 |
| REQ_GRSPW_016 | CTRL.RD | T | TC_GRSPW_058, 057 |
| REQ_GRSPW_017 | CTRL.RE | T | TC_GRSPW_058, 003, 057 |
| REQ_GRSPW_018 | CTRL.TR | T | TC_GRSPW_058 |
| REQ_GRSPW_019 | CTRL.TT | T | TC_GRSPW_058 |
| REQ_GRSPW_020 | CTRL.LI | T | TC_GRSPW_058, 004 |
| REQ_GRSPW_021 | CTRL.TQ | T | TC_GRSPW_058, 004 |
| REQ_GRSPW_022 | CTRL.RS | T | TC_GRSPW_058, 004 |
| REQ_GRSPW_023 | CTRL.PM | T | TC_GRSPW_058, 057 |
| REQ_GRSPW_024 | CTRL.TI | T | TC_GRSPW_058, 015 |
| REQ_GRSPW_025 | CTRL.IE | T | TC_GRSPW_058 |
| REQ_GRSPW_026 | CTRL.AS | T | TC_GRSPW_058, 003 |
| REQ_GRSPW_027 | CTRL.LS | T | TC_GRSPW_058, 052, 057 |
| REQ_GRSPW_028 | CTRL.LD | T | TC_GRSPW_058, 052, 057 |
| REQ_GRSPW_029 | CTRL reserved bits | T | TC_GRSPW_058, 053 |
| REQ_GRSPW_030 | STS.LS | T | TC_GRSPW_059, 011 |
| REQ_GRSPW_031 | STS.AP | T | TC_GRSPW_059, 002 |
| REQ_GRSPW_032 | STS.EE | T | TC_GRSPW_059, 008 |
| REQ_GRSPW_033 | STS.IA | T | TC_GRSPW_059, 008 |
| REQ_GRSPW_034 | STS.WE | T | TC_GRSPW_059; set by TC_GRSPW_013 (blocked) |
| REQ_GRSPW_035 | STS.PE | T | TC_GRSPW_059; set by TC_GRSPW_013 (blocked) |
| REQ_GRSPW_036 | STS.DE | T | TC_GRSPW_059; set by TC_GRSPW_013 (blocked) |
| REQ_GRSPW_037 | STS.ER | T | TC_GRSPW_059; set by TC_GRSPW_013 (blocked) |
| REQ_GRSPW_038 | STS.CE | T | TC_GRSPW_059; set by TC_GRSPW_013 (blocked) |
| REQ_GRSPW_039 | STS.TO | T | TC_GRSPW_059, 008 |
| REQ_GRSPW_040 | STS reserved bits | T | TC_GRSPW_059 |
| REQ_GRSPW_041 | STS cleared by all ones | T | TC_GRSPW_008 |
| REQ_GRSPW_042 | STS race on write-back | T | TC_GRSPW_008, 052 |
| REQ_GRSPW_043 | NODEADDR.NODEADDR | T | TC_GRSPW_060, 003 |
| REQ_GRSPW_044 | NODEADDR reserved bits (no mask) | T | TC_GRSPW_060 |
| REQ_GRSPW_045 | CLKDIV.CLKDIVSTART | T | TC_GRSPW_060, 003, 057 |
| REQ_GRSPW_046 | CLKDIV.CLKDIVRUN | T | TC_GRSPW_060, 003, 057 |
| REQ_GRSPW_047 | CLKDIV reserved bits | T | TC_GRSPW_060 |
| REQ_GRSPW_048 | DKEY.DESTKEY | T | TC_GRSPW_060, 057 |
| REQ_GRSPW_049 | DKEY reserved bits | T | TC_GRSPW_060 |
| REQ_GRSPW_050 | TIME.TCTRL | T | TC_GRSPW_060 |
| REQ_GRSPW_051 | TIME.TIMECNT | T | TC_GRSPW_060 |
| REQ_GRSPW_052 | TIME reserved bits | T | TC_GRSPW_060 |
| REQ_GRSPW_053 | TIME write sends nothing | T | TC_GRSPW_015 |
| REQ_GRSPW_054 | TDR.DISCONNECT | T | TC_GRSPW_060, 003, 057 |
| REQ_GRSPW_055 | TDR.TIMER64 | T | TC_GRSPW_060, 003, 057 |
| REQ_GRSPW_056 | TDR reserved bits | T | TC_GRSPW_060 |
| REQ_GRSPW_057 | DMACTRL.LE | T | TC_GRSPW_061 |
| REQ_GRSPW_058 | DMACTRL.NS | T | TC_GRSPW_061 |
| REQ_GRSPW_059 | DMACTRL.RD | T | TC_GRSPW_061 |
| REQ_GRSPW_060 | DMACTRL.RX | T | TC_GRSPW_061, 009, 052 |
| REQ_GRSPW_061 | DMACTRL.AT | T | TC_GRSPW_061 |
| REQ_GRSPW_062 | DMACTRL.RA | T | TC_GRSPW_061 |
| REQ_GRSPW_063 | DMACTRL.TA | T | TC_GRSPW_061 |
| REQ_GRSPW_064 | DMACTRL.PR | T | TC_GRSPW_061 |
| REQ_GRSPW_065 | DMACTRL.PS | T | TC_GRSPW_061 |
| REQ_GRSPW_066 | DMACTRL.AI | T | TC_GRSPW_061, 004 |
| REQ_GRSPW_067 | DMACTRL.RI | T | TC_GRSPW_061, 004 |
| REQ_GRSPW_068 | DMACTRL.TI | T | TC_GRSPW_061, 004 |
| REQ_GRSPW_069 | DMACTRL.RE | T | TC_GRSPW_061 |
| REQ_GRSPW_070 | DMACTRL.TE | T | TC_GRSPW_061 |
| REQ_GRSPW_071 | DMACTRL reserved bits | T | TC_GRSPW_061 |
| REQ_GRSPW_072 | DMACTRL reads 0 after clear | T | TC_GRSPW_009, 052 |
| REQ_GRSPW_073 | DMACTRL read-modify-write | T | TC_GRSPW_009, 052, 054 |
| REQ_GRSPW_074 | DMAMAXLEN.RXMAXLEN | T | TC_GRSPW_062, 004, 026 |
| REQ_GRSPW_075 | DMAMAXLEN reserved bits | T | TC_GRSPW_062 |
| REQ_GRSPW_076 | DMATXDESC.DESCBASEADDR | T | TC_GRSPW_062, 004 |
| REQ_GRSPW_077 | DMATXDESC.DESCSEL | T | TC_GRSPW_062, 010, 052 |
| REQ_GRSPW_078 | DMATXDESC reserved bits | T | TC_GRSPW_062 |
| REQ_GRSPW_079 | DMARXDESC.DESCBASEADDR | T | TC_GRSPW_062, 004 |
| REQ_GRSPW_080 | DMARXDESC.DESCSEL | T | TC_GRSPW_062, 010, 052 |
| REQ_GRSPW_081 | DMARXDESC reserved bits | T | TC_GRSPW_062 |
| REQ_GRSPW_082 | Table base writes | T | TC_GRSPW_010, 004 |
| REQ_GRSPW_083 | Link state machine | T | TC_GRSPW_011 |
| REQ_GRSPW_084 | Immediate link start | T | TC_GRSPW_011, 052 |
| REQ_GRSPW_085 | Link error by LD | T | TC_GRSPW_012 |
| REQ_GRSPW_086 | Character-level error bits | T | TC_GRSPW_013 (blocked) |
| REQ_GRSPW_087 | Link disable from the packet driver ISR | T | TC_GRSPW_013 (blocked) |
| REQ_GRSPW_088 | Tick-in with TT | T | TC_GRSPW_015 |
| REQ_GRSPW_089 | Tick-in without TT | T | TC_GRSPW_015 |
| REQ_GRSPW_090 | Time-code reception | T | TC_GRSPW_014, 016 |
| REQ_GRSPW_091 | Valid time codes: TCTRL, TO, interrupt | T | TC_GRSPW_014, 016 |
| REQ_GRSPW_092 | TT and TR together | T | TC_GRSPW_015 |
| REQ_GRSPW_093 | RX descriptor layout | T, I | TC_GRSPW_027 |
| REQ_GRSPW_094 | RX descriptor TR | T | TC_GRSPW_063, 026 |
| REQ_GRSPW_095 | RX descriptor DC | T | TC_GRSPW_063, 028 |
| REQ_GRSPW_096 | RX descriptor HC | T | TC_GRSPW_063, 028 |
| REQ_GRSPW_097 | RX descriptor EP | T | TC_GRSPW_063; set by TC_GRSPW_029 (blocked) |
| REQ_GRSPW_098 | RX descriptor IE | T | TC_GRSPW_063, 046 |
| REQ_GRSPW_099 | RX descriptor WR | T | TC_GRSPW_063, 010 |
| REQ_GRSPW_100 | RX descriptor EN | T | TC_GRSPW_063, 024 |
| REQ_GRSPW_101 | RX descriptor PACKETLENGTH | T | TC_GRSPW_063, 027 |
| REQ_GRSPW_102 | RX descriptor PACKETADDRESS, alignment | T | TC_GRSPW_063, 027 |
| REQ_GRSPW_103 | RX descriptor completion | T | TC_GRSPW_017, 027 |
| REQ_GRSPW_104 | Address check, promiscuous | T | TC_GRSPW_018, 019 |
| REQ_GRSPW_105 | Packets for the RMAP target | T | TC_GRSPW_019, 020, 021 |
| REQ_GRSPW_106 | Short packets | T | TC_GRSPW_008, 022 |
| REQ_GRSPW_107 | Spill rules | T | TC_GRSPW_023, 024 |
| REQ_GRSPW_108 | Storing a packet | T | TC_GRSPW_017, 026, 031 |
| REQ_GRSPW_109 | Kept packets | T | TC_GRSPW_024, 025 |
| REQ_GRSPW_110 | RD rereads the descriptor | T | TC_GRSPW_024, 025 |
| REQ_GRSPW_111 | Clearing RE | T | TC_GRSPW_023 |
| REQ_GRSPW_112 | Receive CRC check | T | TC_GRSPW_021, 028, 039 |
| REQ_GRSPW_113 | EEP flag | T | TC_GRSPW_029 (blocked) |
| REQ_GRSPW_114 | RX memory error | T | TC_GRSPW_030 |
| REQ_GRSPW_115 | TX descriptor layout | T, I | TC_GRSPW_033 |
| REQ_GRSPW_116 | TX descriptor DC | T | TC_GRSPW_064, 032 |
| REQ_GRSPW_117 | TX descriptor HC | T | TC_GRSPW_064, 032 |
| REQ_GRSPW_118 | TX descriptor LE | T | TC_GRSPW_064, 036 |
| REQ_GRSPW_119 | TX descriptor IE | T | TC_GRSPW_064, 047 |
| REQ_GRSPW_120 | TX descriptor WR | T | TC_GRSPW_064, 010 |
| REQ_GRSPW_121 | TX descriptor EN | T | TC_GRSPW_064, 031 |
| REQ_GRSPW_122 | TX descriptor NONCRCLEN | T | TC_GRSPW_064, 032 |
| REQ_GRSPW_123 | TX descriptor HEADERLEN | T | TC_GRSPW_064, 034 |
| REQ_GRSPW_124 | TX descriptor HEADERADDRESS | T | TC_GRSPW_064 |
| REQ_GRSPW_125 | TX descriptor DATALEN | T | TC_GRSPW_064, 034 |
| REQ_GRSPW_126 | TX descriptor DATAADDRESS | T | TC_GRSPW_064 |
| REQ_GRSPW_127 | TX descriptor word 0 reserved bits | T | TC_GRSPW_064, 033 |
| REQ_GRSPW_128 | TX descriptor word 2 reserved bits | T | TC_GRSPW_064, 033 |
| REQ_GRSPW_129 | TE starts the transmitter | T | TC_GRSPW_031 |
| REQ_GRSPW_130 | Packet composition | T | TC_GRSPW_031, 032, 034 |
| REQ_GRSPW_131 | Empty descriptor | T | TC_GRSPW_034 |
| REQ_GRSPW_132 | Disabled descriptor clears TE | T | TC_GRSPW_031 |
| REQ_GRSPW_133 | TX completion | T | TC_GRSPW_031, 047 |
| REQ_GRSPW_134 | Sent on acknowledge | T | TC_GRSPW_035, 045 |
| REQ_GRSPW_135 | TX link error, DMACTRL.LE | T | TC_GRSPW_012 |
| REQ_GRSPW_136 | Abort | T | TC_GRSPW_036 |
| REQ_GRSPW_137 | TX memory error | T | TC_GRSPW_037 |
| REQ_GRSPW_138 | Packet over the transport limit | T | TC_GRSPW_038 |
| REQ_GRSPW_139 | RMAP commands | T | TC_GRSPW_039 |
| REQ_GRSPW_140 | RMAP command table | T | TC_GRSPW_039, 040 |
| REQ_GRSPW_141 | Destination key | T | TC_GRSPW_039 |
| REQ_GRSPW_142 | Error priority | T | TC_GRSPW_039, 041 |
| REQ_GRSPW_143 | Verified write and RMW accesses | T | TC_GRSPW_039, 040 |
| REQ_GRSPW_144 | Read reply on error | T | TC_GRSPW_039, 040 |
| REQ_GRSPW_145 | Reply priority, CTRL.RD | A | TC_GRSPW_043 |
| REQ_GRSPW_146 | Read of invalid memory | T | TC_GRSPW_039 |
| REQ_GRSPW_147 | Read over the transport limit | T | TC_GRSPW_042 |
| REQ_GRSPW_148 | Message queues | T | TC_GRSPW_044 |
| REQ_GRSPW_149 | Message format | T | TC_GRSPW_044 |
| REQ_GRSPW_150 | 8185-byte limit | T | TC_GRSPW_038 |
| REQ_GRSPW_151 | Guest not blocked | T, A | TC_GRSPW_035, 045 |
| REQ_GRSPW_152 | One interrupt line per core | T | TC_GRSPW_049 |
| REQ_GRSPW_153 | Link event interrupts | T | TC_GRSPW_014, 016, 052; TC_GRSPW_013 for LI (blocked) |
| REQ_GRSPW_154 | DMA event interrupts | T | TC_GRSPW_030, 046, 047, 054 |
| REQ_GRSPW_155 | Interrupt on event | T | TC_GRSPW_048, 054 |
| REQ_GRSPW_156 | Power-on values | T | TC_GRSPW_003 |
| REQ_GRSPW_157 | Reset by CTRL.RS | T | TC_GRSPW_004, 050 |
| REQ_GRSPW_158 | Link state after reset | T | TC_GRSPW_004, 011 |
| REQ_GRSPW_159 | Packet driver soft reset | T | TC_GRSPW_051 |

### 7.1 Coverage summary

Every requirement has its file `code/test/req/REQ_GRSPW_XXX.c` (section 5.1). Results
of `make -C code/test test` on 2026-11-14, after the corrections of the model listed in
7.3:

| Result | Requirements |
|--------|--------------|
| Pass on all their cores | 156 |
| Known deviation of the model (`XFAIL`) | 0 |
| Blocked (`XFAIL`), see section 8 | 3 (REQ_GRSPW_086, 087, 113), plus the setting of `STS` WE, PE, DE, ER, CE (REQ_GRSPW_034 to 038), of the RX descriptor EP (REQ_GRSPW_097) and the LI part of REQ_GRSPW_153 |

REQ_GRSPW_145 was planned as an analysis (TC_GRSPW_043). Its file tests what can be
observed: RMAP replies are sent while a DMA transmission waits for its acknowledge.

TC_GRSPW_057 (legacy configuration ioctls) is not implemented at driver level: the
register behaviour it relies on is checked by REQ_GRSPW_016, 017, 023, 027, 028, 045,
046, 048, 054 and 055.

### 7.2 Requirements aligned with the manual

Two requirements departed from the manual, while the model followed it. They were
rewritten after the manual on 2026-11-14, and their tests with them:

| Requirement | Before | After |
|-------------|--------|-------|
| REQ_GRSPW_009 | Byte and halfword accesses handled as 32-bit accesses | Undefined, as in the manual (77.7.1); they shall not crash QEMU nor disturb later 32-bit accesses |
| REQ_GRSPW_091 | TO, the interrupt and the counter for every received time code | TCTRL, TO and the interrupt only for a valid time code (counter + 1); the counter for every time code (77.3.5) |

### 7.3 Deviations corrected in the model

Found by the tests, corrected in `code/leon3_grspw_v1` according to the manual, on
2026-11-14:

| Requirements | Deviation | Correction |
|--------------|-----------|------------|
| REQ_GRSPW_005, 026, 156 | `CTRL.AS` reset to 0 on cores 2 and 3 | Reset value `rmapen` (1) on the RMAP cores (77.10.1) |
| REQ_GRSPW_048 | `DKEY` stored on cores 0 and 1 | Reads 0 without RMAP (77.10.5) |
| REQ_GRSPW_102 | Unaligned buffer address used on cores 0 and 1 | Bits 1:0 ignored without `rxunaligned` (77.4.4) |
| REQ_GRSPW_094 to 097, 118 | Status bits of descriptor word 0 only set, never cleared | Written by the core: cleared, then set when they apply |
| REQ_GRSPW_085, 135 | A link error (LD) did not end the pending transmission | Descriptor completed with LE; TE cleared with `DMACTRL.LE` (77.5.7) |
| REQ_GRSPW_136 | AT did not end the pending transmission | Descriptor completed with LE, TE cleared (77.5.7) |
| REQ_GRSPW_062, 063, 114, 137 | Memory errors not detected | A bus error of `address_space_read/write` sets RA or TA, clears RE or TE, interrupts with AI; a TX descriptor stays enabled (77.4.9, 77.5.7) |
| REQ_GRSPW_147 | RMAP reads refused above 8167 bytes | Reply size computed with the reply address actually sent and the 8185-byte packet limit |

The old test `reset_values` (`grspw_raw.c`) expected `CTRL.AS` = 0 after a reset on
cores 2 and 3, the previous behaviour of the model; its expected value was corrected
with the model.

## 8. Facilities missing for blocked tests

| Facility | Unblocks | Proposal |
|----------|----------|----------|
| Fault injection of character-level errors | TC_GRSPW_013 (REQ_GRSPW_086, 087, LI of 153) | A new message type in the queue format, e.g. `0x04 ERROR` with the `STS` bit to set, sent by the peer. Requires a change of REQ_GRSPW_149 |
| EEP marker in the queue format | TC_GRSPW_029 (REQ_GRSPW_113), and the EEP parts of TC_GRSPW_036, 037 and of RMAP read errors | A flag in the DATA message telling that the packet ends with an EEP. Requires a change of REQ_GRSPW_149 and REQ_GRSPW_146 |

Until then, the EEP sent on an abort or a memory error is not observable: TC_GRSPW_036
and 037 only check the descriptor and register effects.

## 9. Open points and risks

- **Timing-dependent tests.** The tests synchronise on messages or register states: a
  time code from the peer sets TO once the packets sent before it are handled. Three
  files rely on a delay of the peer before its acknowledge, because the guest clock may
  run slower than the host clock: REQ_GRSPW_134 (3 s, the guest checks for 200 ms),
  REQ_GRSPW_070 (1 s, the guest checks at once) and REQ_GRSPW_082 (0.5 s). REQ_GRSPW_134
  failed once on core 3 in a full run (the transmission ended before the 200 ms) and
  passed on three reruns: on a loaded host the delay may be too short.
- **Hanging runs.** A transmission that is never acknowledged blocks QEMU until the
  10-second timeout (as `echo_pkt_no_peer`). Independently, a run sometimes times out
  without any output from the guest, then passes when rerun: REQ_GRSPW_137.0, 077.2,
  080.1, 157.2 and `pkt_irq.1` did so once each, about one run in 600. Run in a loop, `pkt_irq.1` hung once in 610
  runs; the queues were empty and the peer waited for a message, so the transport was
  not blocked. The cause is not known yet.
- **Late acknowledges.** The acknowledge message does not identify its packet
  (REQ_GRSPW_134). A peer that acknowledges a packet ended by an abort, a link error or
  a reset after a later packet has been sent completes that later packet. The tests
  never do this.
- **Shared queues.** All tests use the same two queues: the suite cannot run in
  parallel, and a stale queue from an interrupted run makes tests fail. `run_tests.sh`
  already deletes them around each test.
