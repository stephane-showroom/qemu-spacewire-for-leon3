# qemu-spacewire-for-leon3

A SpaceWire simulation environment for LEON3 flight software. It adds a GRSPW
SpaceWire codec to QEMU's `leon3_generic` SPARC machine, so that unmodified RTEMS
applications can use the RTEMS GRSPW drivers without any hardware.

- Four GRSPW cores with registers, interrupts and DMA, modelled on the GRIP
  documentation (`doc/grspw-1.md`).
- An RMAP target on cores 2 and 3, following ECSS-E-ST-50-52C.
- Works with the RTEMS 6 `grspw` and `grspw_pkt` drivers.
- The other end of the link is an ordinary Linux program, connected through POSIX
  message queues.

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
| `code/test` | Both | Replayable test suite, on the four cores. |
| `doc` | | GRSPW (`grspw-1.md`) and GRSPW2 (`grspw-2.md`) documentation. The device models the former. |

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
lines and the one `XFAIL`.

### Running the tests

Run the suite, which takes about 15 seconds:

```sh
make -C code/test test
```

Each test prints one line, and its name ends with the core it ran on:

```
PASS   hello
XFAIL  echo_pkt_no_peer             no peer: the packet is never acknowledged, by design
PASS   all_cores
PASS   echo_pkt.0
PASS   irq_rx_blocking.0
...
```

| Result | Meaning |
|--------|---------|
| `PASS` | The test succeeded. |
| `FAIL` | The test failed. See its log in `code/test/build/logs/<test>.log`. |
| `XFAIL` | Expected failure, marked `known` in `code/test/run_tests.sh`. |
| `XPASS` | A test marked `known` succeeded: remove the mark. |

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

`rmap_test` is the RMAP initiator of the `rmap` test. It first checks its CRC
against the test patterns of the standard.

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

tmux comes with `Containerfile.qemu-dev`. In an older container, install it with
`apt-get install tmux`, or use two terminals and start the simulator first:

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

### RMAP

Cores 2 and 3 execute RMAP commands when `CTRL.RE` is set, which is the reset value.
Their memory accesses go directly to the guest's memory. With `RE` cleared, RMAP
packets go to the DMA channel like any other packet. The RMAP CRC is the 8-bit CRC of
ECSS-E-ST-50-52C, which the DMA channel can also append and check.

### Limitations

- A packet is limited to 8185 bytes by the size of the queue messages. A larger
  transmission ends with a link error, and an RMAP read whose reply would not fit is
  refused with error code 10.
- The queue format has no EEP marker: an RMAP read of invalid memory gives a truncated
  reply without EEP.
- The timing of the link is not modelled, and `CTRL.RD`, which selects the number of
  RMAP buffers, has no effect.
- GRSPW2 features are not modelled: several DMA channels, distributed interrupts,
  CCSDS CRC-16.
- Up to 64 received packets per core can wait for a descriptor in no spill mode. Later
  ones are discarded.

## License

This project is released under the GNU General Public License, version 2 or
(at your option) any later version (`GPL-2.0-or-later`). See [LICENSE](LICENSE) for the full text.
