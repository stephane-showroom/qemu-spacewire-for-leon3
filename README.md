# qemu-spacewire-for-leon3

A SpaceWire simulation environment for LEON3 flight software. It adds a GRSPW
SpaceWire codec to QEMU's `leon3_generic` SPARC machine, so that unmodified RTEMS
applications can use the RTEMS GRSPW drivers without any hardware.

- Four GRSPW cores with registers, interrupts and DMA, modelled on the GRIP
  documentation, GRSPW chapter ([grip.pdf](https://download.gaisler.com/products/GRLIB/doc/grip.pdf)).
- An RMAP target on cores 2 and 3, following ECSS-E-ST-50-52C.
- Works with the RTEMS 6 `grspw` and `grspw_pkt` drivers.
- The other end of the link is an ordinary Linux program, connected through POSIX
  message queues.
- Specified in [SPECIFICATION.md](SPECIFICATION.md), one requirement per register
  field and descriptor field, and verified by [TEST_PLAN.md](TEST_PLAN.md), with one
  test file per requirement.

```
 ┌────────────────── QEMU leon3_generic ──────────────────┐
 │  RTEMS application                                     │
 │        │  grspw / grspw_pkt driver                     │
 │        ▼                                               │        /spwq
 │  GRSPW device (4 cores: registers, IRQ, DMA, RMAP)  ───┼───────────────►  host program
 │                                                     ◄──┼───────────────   (device_simu)
 └────────────────────────────────────────────────────────┘        /devq
```

## Modules

| Path | Runs on | Role |
|------|---------|------|
| `code/leon3_grspw_v1` | QEMU | The GRSPW device: registers, DMA, interrupts, time codes, RMAP target. |
| `code/qemu-patches` | QEMU | Patches of the QEMU tree. `0001` adds the device to the build and to `leon3_generic`. `0002` adds the LEON forced cache miss ASI, which the RTEMS drivers use for their reads. |
| `code/integrate_qemu.sh` | Build | Links the device into QEMU, applies the patches and rebuilds QEMU. |
| `code/device_simu` | Linux | Runtime peer of the link: acknowledges each packet and sends it back in upper case. |
| `code/rtems_hello` | RTEMS | Minimal application, and `qemu_boot.S`, the entry point needed to boot RTEMS with `-kernel`. |
| `code/rtems_spw_echo` | RTEMS | SpaceWire echo using the `grspw_pkt` driver, with `device_simu` at the other end. |
| `code/demo` | Both | Demonstration: an onboard computer (RTEMS) with an inertial measurement unit and a ground station (Linux). |
| `code/test` | Both | Replayable test suite, on the four cores: the driver and register tests, and in `code/test/req` one test file per requirement of the specification. |
| `SPECIFICATION.md` | - | The requirements of the device (`REQ_GRSPW_XXX`), derived from the GRSPW manual, with the RTEMS drivers for what the manual leaves open. |
| `TEST_PLAN.md` | - | The test cases (`TC_GRSPW_XXX`), the traceability matrix from each requirement to its tests, the results and the deviations found. |

The device is built inside the QEMU tree: it needs to create MMIO regions and
interrupts, which the TCG plugin API (`-plugin`) cannot do.

## Building the containers

The development environment is a chain of three images, built with
[Podman](https://podman.io/). Each one starts from the previous one, so build them
in this order. The first two compile a cross toolchain and RTEMS, which takes a
while.

| Order | Containerfile | Image | Content |
|-------|---------------|-------|---------|
| 1 | `Containerfile.leon3-toolchain` | `rtems6-sparc-toolchain` | GCC, binutils, newlib and GDB for `sparc-rtems6` |
| 2 | `Containerfile.rtems-build` | `rtems6-sparc-build` | RTEMS 6.2 with the `sparc/leon3` BSP |
| 3 | `Containerfile.qemu-dev` | `localhost/qemu-dev-plugin` | QEMU 11.0 sources and build tools, tmux for the demonstration |

```sh
podman build -f Containerfile.leon3-toolchain -t rtems6-sparc-toolchain .
podman build -f Containerfile.rtems-build     -t rtems6-sparc-build .
podman build -f Containerfile.qemu-dev        -t localhost/qemu-dev-plugin .
```

## Working in VS Code

1. Install the **Dev Containers** extension.
2. To use Podman, let VS Code find it as `docker`:

   ```sh
   sudo ln -s /usr/bin/podman /usr/local/bin/docker
   ```

3. Edit `.devcontainer/devcontainer.json` for your machine: `dockerSocket` must match
   your user ID (`/run/user/<uid>/podman/podman.sock`). The clone is mounted from
   wherever you opened it, so its path needs no change.
4. Open the repository, then run **Dev Containers: Reopen in Container** from the
   command palette. The project is in `/workspace/spacewire`.
5. Open a terminal in VS Code (**Terminal > New Terminal**). It runs inside the
   container, and every command below is run from there.

### Setting up QEMU

The image contains a stock QEMU in `/workspace/qemu`, without the GRSPW device. VS Code
adds it when it creates the container, with the `postCreateCommand` of
`.devcontainer/devcontainer.json`, so wait for this command to end in the terminal it
opens. Outside VS Code, or to run it again:

```sh
code/integrate_qemu.sh
```

The script is safe to run again. It:

1. links `code/leon3_grspw_v1` into the QEMU tree as `hw/misc/grspw`, so the sources stay
   in this repository;
2. applies `code/qemu-patches/*.patch`, skipping the ones already applied;
3. rebuilds QEMU in `/workspace/qemu/build`. The first run takes several minutes.

To use another QEMU tree, give its path: `code/integrate_qemu.sh <qemu-dir>`. It must
be a git tree already configured in `<qemu-dir>/build`, and the patches are made for
QEMU 11.0 (`stable-11.0`).

Check that the device is there:

```sh
/workspace/qemu/build/qemu-system-sparc -device grlib-grspw,help
```

This lists the options of the device. An error means that this QEMU has no GRSPW
device, which is the case of the `qemu-system-sparc` in your `PATH`: it comes from the
image and is not updated. `code/rtems_spw_echo` and `code/test` use the one in
`/workspace/qemu/build`.

| When | What to do |
|------|------------|
| New or rebuilt container | Nothing in VS Code, otherwise run `code/integrate_qemu.sh`: `/workspace/qemu` is part of the container, unlike the repository, so its changes are lost. |
| Change in `code/leon3_grspw_v1` | Run `code/integrate_qemu.sh` to rebuild QEMU. |
| New patch in `code/qemu-patches` | Run `code/integrate_qemu.sh`. |
| Change in a patch already applied, or `patch does not apply` | Restore the tree, then run the script: `git -C /workspace/qemu checkout .` |

### Checking the environment

Once QEMU is set up, run the test suite:

```sh
make -C code/test test
```

The environment is functional when this command ends without error, with only `PASS`
and `XFAIL` lines. The `XFAIL` lines are `echo_pkt_no_peer`, which fails by design,
and the requirements that cannot be tested yet (`REQ_GRSPW_086`, `087` and `113`, see
section 8 of [TEST_PLAN.md](TEST_PLAN.md)).

### Running the tests

Run the suite, which takes about 3 minutes:

```sh
make -C code/test test
```

To run only some tests, give a shell pattern on their name:

```sh
ONLY='REQ_GRSPW_03*' make -C code/test test
```

Each test prints one line, and its name ends with the core it ran on:

```
PASS   hello
XFAIL  echo_pkt_no_peer             no peer: the packet is never acknowledged, by design
PASS   all_cores
PASS   echo_pkt.0
PASS   irq_rx_blocking.0
...
PASS   REQ_GRSPW_001.0
...
```

| Result | Meaning |
|--------|---------|
| `PASS` | The test succeeded. |
| `FAIL` | The test failed. See its log in `code/test/build/logs/<test>.log`. |
| `XFAIL` | Expected failure: marked `known` in `code/test/run_tests.sh`, or with an `@known` line in a requirement file. The reason is printed on the line. |
| `XPASS` | A test marked as an expected failure succeeded: remove the mark. |

The command fails if there is any `FAIL` or `XPASS`.

Do not run the suite while another QEMU is using the link: it deletes the message
queues before each test.

### What the tests cover

| Tests | Purpose |
|-------|---------|
| `hello`, `echo_pkt` | The applications of this repository. |
| `pkt_irq` | `grspw_pkt` with interrupts, its work task and a list of packets. |
| `irq_rx_blocking`, `many_packets`, `stop_start`, `clear_ctrl_bit`, `max_packet`, `data_before_ack` | The legacy `grspw` driver, with interrupts. |
| `reset_values`, `rx_ri_only`, `tx_burst`, `rx_no_spill`, `invalid_address`, `header`, `time_codes` | Behaviour of the documentation, at register level. |
| `rmap`, `rmap_disabled`, `tx_crc`, `no_rmap` | RMAP and its CRC, on the cores concerned. |
| `all_cores` | The four cores active at the same time. |
| `device_simu_queues` | `device_simu` alone, at message queue level. |
| `REQ_GRSPW_XXX` | One test per requirement of [SPECIFICATION.md](SPECIFICATION.md), on each core it applies to. |

`rmap_test` is the RMAP initiator of the `rmap` test. It first checks its CRC
against the test patterns of the standard.

Each file `code/test/req/REQ_GRSPW_XXX.c` holds only the checks of its requirement.
Its header comment says how it runs, and the Makefile and `run_tests.sh` read it, so a
new file needs no other change:

| Line | Meaning |
|------|---------|
| `@tc` | Test case(s) of the test plan |
| `@side guest` or `@side peer` | Checks in the RTEMS program (`req/req_guest.h`), or in a Linux peer (`req/req_peer.h`) run against `req/peer_guest.c` |
| `@cores` | Cores the test runs on |
| `@peer` | Program at the other end: `none`, `device_simu`, or `peer_script` with its steps |
| `@known <cores>: <reason>` | Expected failure on these cores |

`peer_script` (`code/test/peer_script.c`) is a peer driven by its arguments: it
acknowledges at once, later or never, sends packets, time codes or RMAP writes, and
sends back to the guest what it received, so that the checks stay in the guest.

### Running the applications

```sh
make -C code/rtems_hello run
make -C code/rtems_spw_echo run
```

`rtems_spw_echo` starts `device_simu` by itself. To run your own application, start
`device_simu` first:

```sh
make -C code/device_simu run
```

QEMU returns 0 even when RTEMS stops on a fatal error, so check the console output
rather than the exit code. After an interrupted run, remove the stale queues, whose
old messages would be taken for acknowledges:

```sh
rm -f /dev/mqueue/spwq /dev/mqueue/devq
```

## Demonstration

`code/demo` puts the pieces together: an onboard computer on the LEON3 receives the
data of an inertial measurement unit (IMU), and a ground station commands its state
with RMAP.

```
┌─── QEMU: onboard computer (RTEMS) ───┐          ┌─── simu (Linux) ──────────────┐
│ core 0 ◄──── IMU packets, DMA ───────┼──────────┼──── imu.c      1 packet/s     │
│ core 2 ───── address of the state ───┼──────────┼───► ground.c                  │
│        ◄──── RMAP read/write ────────┼──────────┼──── INIT→WARMUP→OPERATIONAL   │
└──────────────────────────────────────┘  /spwq   └───────────────────────────────┘
                                          /devq
```

| Path | Runs on | Role |
|------|---------|------|
| `code/demo/demo_protocol.h` | Both | Packet formats, node addresses, RMAP key and states. |
| `code/demo/onboard` | RTEMS | The onboard computer. |
| `code/demo/simu` | Linux | The environment simulator, only reader of `/spwq` and writer of `/devq`. It routes the messages by core number to `imu.c` (core 0) and `ground.c` (core 2), and acknowledges every packet of the onboard computer. |
| `code/demo/demo.sh` | Linux | Runs both side by side in tmux. |

The onboard computer:

- receives the IMU packets on core 0, in DMA: roll, pitch, yaw, angular rates and
  accelerations, with a sequence number;
- keeps its state in a variable, whose address it sends to the ground on core 2 at
  startup. The ground then reads and writes this variable with RMAP commands, which
  the RMAP target of core 2 executes without the processor;
- has the states `INIT` → `WARMUP` → `OPERATIONAL`, and `FAIL` from any state. An
  invalid transition or value written by the ground is reported and undone;
- ignores the IMU in `INIT`, receives its data without using it in `WARMUP`, and
  prints it in `OPERATIONAL`;
- goes to `FAIL` by itself when no IMU data came for 3 s in `OPERATIONAL`;
- prints a status line every second, and highlights each state change.

The ground station waits for the address, reads the initial state, then commands
`WARMUP` and `OPERATIONAL` five seconds apart. Each write is a verified write with
reply, followed by a read of the state. It then reads the state every five seconds.
Every packet is printed in hexadecimal.

```sh
code/demo/demo.sh                   # automatic scenario
code/demo/demo.sh -i                # the ground waits for Enter before each step
code/demo/demo.sh --imu-fail 20     # the IMU stops after 20 s: the onboard computer goes to FAIL
code/demo/demo.sh shutdown          # from another terminal: FAIL, then stop after 5 s
code/demo/demo.sh stop              # from another terminal: stop at once
```

The script builds both programs. In the tmux session:

| Key | Effect |
|-----|--------|
| Escape | The ground station commands `FAIL` with RMAP, and the demonstration stops 5 s later. |
| Ctrl-C | The demonstration stops at once. |
| Enter | In `-i` mode, in the right pane: next step of the ground scenario. |

Stopping kills QEMU and the simulator and removes the message queues. The session
has its own tmux server (`tmux -L spw-demo`), so it also works from inside tmux and
these keys leave your other sessions alone.

tmux comes with `Containerfile.qemu-dev` or use two terminals and start the simulator first:

```sh
make -C code/demo/simu run ARGS="-i"
make -C code/demo/onboard run
```

## The GRSPW device

On `-M leon3_generic`:

| Core | Registers  | IRQ | RMAP |
|------|------------|-----|------|
| 0    | 0x80000A00 | 10  | No   |
| 1    | 0x80000B00 | 11  | No   |
| 2    | 0x80000C00 | 12  | Yes  |
| 3    | 0x80000D00 | 13  | Yes  |

The cores are listed in the APB plug&play table (Gaisler, device 0x1F).

Set `GRSPW_LOG_DIR=<dir>` to log register accesses to `<dir>/log_grspw<N>.txt`.

### Message queues

The link uses two POSIX message queues of 10 messages of 8192 bytes:

| Queue | Direction |
|-------|-----------|
| `/spwq` | QEMU to the peer |
| `/devq` | The peer to QEMU |

Each message is `[core][protocol word][header length][header][data length, u32][data]`.

| Protocol word | Meaning |
|---------------|---------|
| `0x00` ACK | Sent by the peer for each data packet. |
| `0x01` DATA | A SpaceWire packet. |
| `0x02` TIME | A time code. |
| `0x03` RMAP | An RMAP reply from QEMU. The peer does not acknowledge it. |

A transmitted packet stays in progress, with `TE` set, until the peer acknowledges
it. The guest keeps running meanwhile. Without a peer, its transmissions never end.
An abort (`DMACTRL.AT`) or a link disable (`CTRL.LD`) ends it at once, with `LE` set
in its descriptor.

### Memory errors

An access of the DMA channel outside the guest's memory is an AHB error, as in the
manual: `DMACTRL.RA` or `TA` is set, the receiver or transmitter is disabled, and an
interrupt is raised if `DMACTRL.AI` is set. An RMAP write or read-modify-write reports
it with error code 1, and an RMAP read gives a truncated reply (see the limitations).

### RMAP

Cores 2 and 3 execute RMAP commands when `CTRL.RE` is set, which is the reset value.
Their memory accesses go directly to the guest's memory. With `RE` cleared, RMAP
packets go to the DMA channel like any other packet. The RMAP CRC is the 8-bit CRC of
ECSS-E-ST-50-52C, which the DMA channel can also append and check.

### Limitations

Most limitations come from the same choice: the link is not simulated character by
character. The device exchanges whole packets with a Linux program through two
message queues, so what the hardware does at the level of characters, time or
electrical signals has to be simplified.

- **Packets of at most 8185 bytes.** A larger transmission ends with a link error, and
  an RMAP read whose reply would not fit is refused with error code 10.
  *Why:* each packet travels in one queue message, and Linux limits a message to
  8192 bytes by default (`/proc/sys/fs/mqueue/msgsize_max`), minus the 7 bytes of the
  message header. SpaceWire itself has no packet size limit.
- **No EEP marker.** An RMAP read of invalid memory gives a truncated reply, without
  the EEP that ends it on the hardware, and the peer cannot send a packet ended by an
  EEP.
  *Why:* the message format has no field telling how a packet ends, so every DATA
  message is taken as ended by an EOP.
- **No link timing, and `CTRL.RD` has no effect.** `CLKDIV` and `TDR` read back what
  is written but change nothing.
  *Why:* the device is event driven: a packet is written to memory, or sent, in one
  step when it is handled, with no notion of bit rate. `CTRL.RD` exists because the
  hardware buffers several RMAP replies and may reorder them; the device executes each
  RMAP command at once, in arrival order, so there is nothing to reorder.
- **No GRSPW2 features:** several DMA channels, distributed interrupts, CCSDS CRC-16.
  *Why:* the device models the GRSPW core of the manual (GRSPW1); the GRSPW2 is a
  later, different core. The RTEMS drivers detect a GRSPW1 from the registers that
  read 0, and then do not use these features.
- **At most 64 received packets per core wait for a descriptor in no spill mode.**
  Later ones are discarded.
  *Why:* the hardware keeps such a packet in the link, by giving no more flow control
  credits to the sender, and never loses it. The device cannot hold the peer back this
  way: packets, acknowledges and time codes all come through `/devq`, and it must keep
  reading this queue to get the acknowledges of its own packets. It keeps the waiting
  packets in a list instead, whose size has to be bounded.
- **The link is always up**, and the character-level errors (`STS` WE, PE, DE, ER,
  CE) are never set: `LS`, or `AS`, puts the link in the run state at once.
  *Why:* the physical layer is not simulated: there are no Data/Strobe signals, NULL
  characters or credits, and nothing in the message format tells whether the peer is
  there or the line is faulty. These errors need a fault injection message, which
  the format does not have yet.
- **The acknowledge does not say which packet it acknowledges.** If the peer
  acknowledges a packet ended by an abort, a link disable or a reset after the next
  packet has been sent, it completes that next packet.
  *Why:* the ACK message only carries the core number. The device ends an aborted
  packet at once, but the peer has already received it in full and may still
  acknowledge it. Ignoring the next acknowledge would not work either, since nothing
  requires the peer to acknowledge an aborted packet.
- **Byte and halfword accesses to the registers are undefined**: use 32-bit accesses.
  *Why:* this is the rule of the GRSPW manual (section 77.7.1), and the RTEMS drivers
  follow it. The device only accepts 4-byte accesses, so QEMU rejects the others.

The first, second, fifth, sixth and seventh limitations come from the message format.
A new version of the format, with packets split over several messages, an EEP flag, a
packet number in the ACK, flow control and fault injection, would remove them.

## License

This project is released under the GNU General Public License, version 2 or
(at your option) any later version (`GPL-2.0-or-later`). See [LICENSE](LICENSE) for the full text.
