/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 10, 2026                                      *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * Onboard computer of the demonstration, on the LEON3.
 *
 * - Core 0: receives the packets of the inertial measurement unit (IMU) in DMA.
 *   The RMAP target of this core does not exist.
 * - Core 2: at startup, sends the address of the state variable to the ground,
 *   which then reads and writes this variable with RMAP. The RMAP target works
 *   on the guest memory without the processor, so the application only sees the
 *   variable change.
 *
 * States: INIT -> WARMUP -> OPERATIONAL, and FAIL from any state. An invalid
 * value or transition written by the ground is reported and undone. In INIT the
 * IMU packets are discarded, in WARMUP they are received but not used, in
 * OPERATIONAL they are printed. Without IMU packet for IMU_TIMEOUT_S seconds in
 * OPERATIONAL, the application goes to FAIL by itself.
 *
 * A status line is printed every second, and each state change is highlighted.
 * See demo_protocol.h for the packet formats.
 */

#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drvmgr/drvmgr.h>
#include <grlib/ambapp.h>
#include <grlib/ambapp_bus_grlib.h>
#include <grlib/grspw_pkt.h>

#include "demo_protocol.h"

#define IMU_TIMEOUT_S     3
#define POLL_PERIOD_MS    50
#define RX_BUFFER_COUNT   8
#define MAX_PACKET        256

#define COLOR_RESET       "\033[0m"
#define COLOR_CHANGE      "\033[1;33m"
#define COLOR_ERROR       "\033[1;31m"

/* Everything is polled here: the driver's work task is not needed. */
int grspw_work_task_priority = -1;

/* Written by the ground with RMAP: the processor must read it from memory each time. */
static volatile uint32_t state_variable __attribute__((aligned(4))) = STATE_INIT;

static uint8_t rx_buffers[RX_BUFFER_COUNT][MAX_PACKET] __attribute__((aligned(4)));
static struct grspw_pkt rx_packets[RX_BUFFER_COUNT];
static uint8_t address_packet[ADDRESS_PACKET_SIZE] __attribute__((aligned(4)));
static struct grspw_pkt address_tx;

static struct {
  uint32_t state;              /* Last accepted state */
  rtems_interval last_imu;     /* Ticks of the last IMU packet */
  uint32_t received;           /* IMU packets received since startup */
  uint32_t used;               /* IMU packets used in OPERATIONAL */
  uint32_t last_sequence;
  int address_sent;
} onboard;

static void fail(const char *message)
{
  printf(COLOR_ERROR "ONBOARD FAILED: %s" COLOR_RESET "\n", message);
  exit(1);
}

static rtems_interval now(void)
{
  return rtems_clock_get_ticks_since_boot();
}

static double seconds(rtems_interval ticks)
{
  return (double) ticks / rtems_clock_get_ticks_per_second();
}

/*
 * The leon3 BSP is built without RTEMS_DRVMGR_STARTUP, so its console and
 * clock do not depend on the driver manager and the application starts it.
 */
static void start_driver_manager(void)
{
  static struct drvmgr_bus_res resources = {
    .next = NULL,
    .resource = { DRVMGR_RES_EMPTY },
  };
  static struct grlib_config config;

  config.abus = ambapp_plb();
  config.resources = &resources;

  if (ambapp_grlib_root_register(&config) != 0) {
    fail("cannot register the AMBA root bus");
  }

  drvmgr_init();
}

/* Opens a core with the node address NODE_ONBOARD and starts its DMA channel. */
static void *open_core(int core, int rmap_options)
{
  struct grspw_addr_config address;
  struct grspw_dma_config dma_config;
  int link_options = LINKOPTS_ENABLE | LINKOPTS_START;
  int status_config = -1;
  int clock_divisor = -1;
  int key = RMAP_KEY;
  void *device;
  void *dma;

  device = grspw_open(core);
  if (device == NULL) {
    fail("grspw_open");
  }

  memset(&address, 0, sizeof(address));
  address.def_addr = NODE_ONBOARD;
  grspw_addr_ctrl(device, &address);

  if (rmap_options != 0 && grspw_rmap_ctrl(device, &rmap_options, &key) != 0) {
    fail("grspw_rmap_ctrl");
  }

  grspw_link_ctrl(device, &link_options, &status_config, &clock_divisor);
  if (grspw_link_state(device) != SPW_LS_RUN) {
    fail("the link is not in the run state");
  }

  dma = grspw_dma_open(device, 0);
  if (dma == NULL) {
    fail("grspw_dma_open");
  }

  memset(&dma_config, 0, sizeof(dma_config));
  dma_config.flags = DMAFLAG_NO_SPILL;
  dma_config.rxmaxlen = MAX_PACKET;
  if (grspw_dma_config(dma, &dma_config) != 0) {
    fail("grspw_dma_config");
  }

  if (grspw_dma_start(dma) != 0) {
    fail("grspw_dma_start");
  }

  return dma;
}

static void prepare_reception(void *dma, struct grspw_pkt *packet)
{
  struct grspw_list list = { packet, packet };

  packet->next = NULL;
  packet->flags = 0;
  packet->dlen = 0;
  packet->hlen = 0;
  if (grspw_dma_rx_prepare(dma, 0, &list, 1) != 0) {
    fail("grspw_dma_rx_prepare");
  }
}

static void send_address(void *dma)
{
  struct grspw_list list = { &address_tx, &address_tx };

  address_packet[0] = NODE_GROUND;
  address_packet[1] = PROTOCOL_ADDRESS;
  put_be32(&address_packet[2], (uint32_t) (uintptr_t) &state_variable);

  memset(&address_tx, 0, sizeof(address_tx));
  address_tx.data = address_packet;
  address_tx.dlen = ADDRESS_PACKET_SIZE;

  if (grspw_dma_tx_send(dma, 0, &list, 1) != 0) {
    fail("grspw_dma_tx_send");
  }

  printf("state variable at 0x%08lx, address sent to the ground on core %d\n",
         (unsigned long) (uintptr_t) &state_variable, CORE_GROUND);
}

/* The transmission ends when the simulator acknowledges the packet. */
static void reclaim_address(void *dma)
{
  struct grspw_list list = { NULL, NULL };
  int count = -1;

  if (onboard.address_sent) {
    return;
  }

  if (grspw_dma_tx_reclaim(dma, 0, &list, &count) != 0) {
    fail("grspw_dma_tx_reclaim");
  }

  if (count == 1) {
    onboard.address_sent = 1;
    printf("address acknowledged by the ground\n");
  }
}

static void print_imu(const uint8_t *packet)
{
  int32_t value[IMU_VALUE_COUNT];
  int i;

  for (i = 0; i < IMU_VALUE_COUNT; ++i) {
    value[i] = (int32_t) get_be32(&packet[IMU_VALUES_OFFSET + 4 * i]);
  }

  printf("  IMU #%-5lu att %7.2f %7.2f %7.2f deg | rate %6.2f %6.2f %6.2f deg/s"
         " | acc %6.2f %6.2f %6.2f m/s2\n",
         (unsigned long) get_be32(&packet[IMU_SEQUENCE_OFFSET]),
         value[IMU_ROLL] / 1000.0, value[IMU_PITCH] / 1000.0, value[IMU_YAW] / 1000.0,
         value[IMU_P] / 1000.0, value[IMU_Q] / 1000.0, value[IMU_R] / 1000.0,
         value[IMU_AX] / 1000.0, value[IMU_AY] / 1000.0, value[IMU_AZ] / 1000.0);
}

static void handle_imu_packet(const struct grspw_pkt *packet)
{
  const uint8_t *data = packet->data;

  /* In INIT the IMU is not listened to: the buffer is only given back. */
  if (onboard.state == STATE_INIT) {
    return;
  }

  if (packet->dlen != IMU_PACKET_SIZE || data[0] != NODE_ONBOARD
      || data[1] != PROTOCOL_IMU || (packet->flags & RXPKT_FLAG_RX) == 0) {
    printf(COLOR_ERROR "  discarded an invalid IMU packet (%lu bytes)" COLOR_RESET "\n",
           (unsigned long) packet->dlen);
    return;
  }

  onboard.last_imu = now();
  onboard.last_sequence = get_be32(&data[IMU_SEQUENCE_OFFSET]);
  ++onboard.received;

  if (onboard.state == STATE_OPERATIONAL) {
    ++onboard.used;
    print_imu(data);
  }
}

static void receive_imu(void *dma)
{
  for (;;) {
    struct grspw_list list = { NULL, NULL };
    struct grspw_pkt *packet;
    int count = -1;

    if (grspw_dma_rx_recv(dma, 0, &list, &count) != 0) {
      fail("grspw_dma_rx_recv");
    }
    if (count <= 0) {
      return;
    }

    packet = list.head;
    while (count-- > 0 && packet != NULL) {
      struct grspw_pkt *next = packet->next;

      handle_imu_packet(packet);
      prepare_reception(dma, packet);
      packet = next;
    }
  }
}

static void change_state(uint32_t to, const char *reason)
{
  printf(COLOR_CHANGE ">>> STATE %s -> %s (%s)" COLOR_RESET "\n",
         state_name(onboard.state), state_name(to), reason);

  /* The IMU timeout starts when the application begins to use the data. */
  if (to == STATE_OPERATIONAL) {
    onboard.last_imu = now();
  }

  onboard.state = to;
  state_variable = to;
}

/* Takes the value written by the ground, or undoes it. */
static void check_state_variable(void)
{
  uint32_t requested = state_variable;

  if (requested == onboard.state) {
    return;
  }

  if (!transition_is_valid(onboard.state, requested)) {
    printf(COLOR_ERROR "!!! INVALID TRANSITION %s -> %s (value %lu), state kept"
           COLOR_RESET "\n", state_name(onboard.state), state_name(requested),
           (unsigned long) requested);
    state_variable = onboard.state;
    return;
  }

  change_state(requested, "ground command");
}

static void check_imu_timeout(void)
{
  rtems_interval timeout = IMU_TIMEOUT_S * rtems_clock_get_ticks_per_second();

  if (onboard.state == STATE_OPERATIONAL && now() - onboard.last_imu > timeout) {
    change_state(STATE_FAIL, "no IMU data for 3 s");
  }
}

static void print_status(void)
{
  const char *imu = "ignored";

  if (onboard.state != STATE_INIT) {
    imu = onboard.received == 0 ? "waiting" : "received";
  }

  printf("[%7.1f s] state %-11s | IMU %-8s last #%-5lu %4.1f s ago | used %lu\n",
         seconds(now()), state_name(onboard.state), imu,
         (unsigned long) onboard.last_sequence,
         onboard.received == 0 ? 0.0 : seconds(now() - onboard.last_imu),
         (unsigned long) onboard.used);
}

static rtems_task Init(rtems_task_argument arg)
{
  rtems_interval period = RTEMS_MILLISECONDS_TO_TICKS(POLL_PERIOD_MS);
  rtems_interval next_status;
  void *imu_dma;
  void *ground_dma;
  int i;

  (void) arg;

  start_driver_manager();
  printf("\nONBOARD COMPUTER, %d GRSPW cores\n", grspw_dev_count());
  imu_dma = open_core(CORE_IMU, 0);
  ground_dma = open_core(CORE_GROUND, RMAPOPTS_EN_RMAP);

  for (i = 0; i < RX_BUFFER_COUNT; ++i) {
    memset(&rx_packets[i], 0, sizeof(rx_packets[i]));
    rx_packets[i].data = rx_buffers[i];
    prepare_reception(imu_dma, &rx_packets[i]);
  }

  onboard.state = STATE_INIT;
  state_variable = STATE_INIT;
  printf("IMU on core %d (DMA), ground on core %d (RMAP, key 0x%02x)\n",
         CORE_IMU, CORE_GROUND, RMAP_KEY);
  send_address(ground_dma);

  next_status = now();
  for (;;) {
    reclaim_address(ground_dma);
    check_state_variable();
    receive_imu(imu_dma);
    check_imu_timeout();

    if ((int32_t) (now() - next_status) >= 0) {
      print_status();
      next_status += rtems_clock_get_ticks_per_second();
    }

    rtems_task_wake_after(period);
  }
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 16
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
/* newlib's printf uses the FPU, which is forbidden in non-floating-point tasks. */
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

#define CONFIGURE_DRIVER_AMBAPP_GAISLER_GRSPW2
#include <drvmgr/drvmgr_confdefs.h>
