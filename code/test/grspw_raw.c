/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * GRSPW scenarios at register level, without driver, for the behaviours of the
 * GRIP documentation
 * (https://download.gaisler.com/products/GRLIB/doc/grlib.pdf) that the drivers
 * do not reveal. Each
 * scenario is selected at build time with -DSCENARIO_<name>, the core with
 * -DCORE=<n>, and prints TEST OK on success. They run against device_simu, except rmap (rmap_test.c) and
 * time_codes (peer_time_codes.c).
 */

#include <rtems.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CORE_COUNT        4
#define CORE_ADDRESS(core) (0x80000A00U + 0x100U * (core))
#define CORE_BASE         CORE_ADDRESS(CORE)
#define CORE_IRQ          (10 + CORE)
#define FIRST_RMAP_CORE   2             /* Cores 2 and 3 have RMAP */

#define CTR_LS            (1U << 1)
#define CTR_AS            (1U << 2)
#define CTR_IE            (1U << 3)
#define CTR_RS            (1U << 6)
#define CTR_TQ            (1U << 8)
#define CTR_LI            (1U << 9)
#define CTR_TR            (1U << 11)
#define CTR_RMAP_ENABLE   (1U << 16)
#define CTR_RC            (1U << 29)
#define CTR_RX            (1U << 30)
#define CTR_RA            (1U << 31)

#define STR_TO            (1U << 0)
#define STR_IA            (1U << 7)
#define STR_LINK_MASK     (7U << 21)

#define CHN_TE            (1U << 0)
#define CHN_RE            (1U << 1)
#define CHN_RI            (1U << 3)
#define CHN_RD            (1U << 11)
#define CHN_NS            (1U << 12)

#define RXD_LEN_MASK      0x1ffffffU
#define RXD_EN            (1U << 25)
#define RXD_IE            (1U << 27)
#define RXD_HC            (1U << 29)
#define RXD_DC            (1U << 30)
#define TXD_EN            (1U << 12)
#define TXD_HC            (1U << 16)
#define TXD_DC            (1U << 17)
#define TXD_NON_CRC_SHIFT 8

#define NODE_ADDRESS      0x42
#define OTHER_ADDRESS     0x55
#define PROTOCOL_ID       0xF0
#define MAX_LENGTH        1024
#define DESCRIPTORS       3
#define WAIT_TICKS        500
#define SETTLE_TICKS      20

typedef struct {
  volatile uint32_t ctr, str, ndr, clk, key, tim, tdr, pad;
  volatile uint32_t chn, rxl, txd, rxd;
} registers;

typedef struct { volatile uint32_t ctrl, addr; } rx_descriptor;
typedef struct { volatile uint32_t ctrl, header, len, data; } tx_descriptor;

static registers *const regs = (registers *) CORE_BASE;

static rx_descriptor rx_table[128] __attribute__((aligned(1024)));
static tx_descriptor tx_table[64] __attribute__((aligned(1024)));
static uint8_t rx_buffer[DESCRIPTORS][MAX_LENGTH] __attribute__((aligned(4)));
static uint8_t tx_buffer[DESCRIPTORS][MAX_LENGTH] __attribute__((aligned(4)));

static volatile int irq_count;

static void fail(const char *message)
{
  printf("TEST FAILED: %s\n", message);
  exit(1);
}

#define WAIT_FOR(condition, message) \
  do { \
    int ticks = 0; \
    while (!(condition)) { \
      if (++ticks > WAIT_TICKS) { \
        fail(message); \
      } \
      rtems_task_wake_after(1); \
    } \
  } while (0)

/* The compiler must not move the buffer accesses after the start of the DMA. */
__attribute__((unused)) static void start_dma(uint32_t value)
{
  RTEMS_COMPILER_MEMORY_BARRIER();
  regs->chn = value;
}

static void count_irq(void *arg)
{
  (void) arg;
  ++irq_count;
}

__attribute__((unused)) static void setup(void)
{
  regs->ctr = CTR_RS;
  regs->ctr = CTR_LS;
  regs->ndr = NODE_ADDRESS;
  regs->rxl = MAX_LENGTH;
  regs->rxd = (uint32_t) rx_table;
  regs->txd = (uint32_t) tx_table;
}

__attribute__((unused)) static void enable_rx_descriptor(int index, uint32_t flags)
{
  rx_table[index].addr = (uint32_t) rx_buffer[index];
  rx_table[index].ctrl = RXD_EN | flags;
}

__attribute__((unused)) static void fill_tx_descriptor(int index, uint8_t destination)
{
  tx_buffer[index][0] = destination;
  tx_buffer[index][1] = PROTOCOL_ID;
  tx_buffer[index][2] = (uint8_t) index;

  tx_table[index].header = 0;
  tx_table[index].len = 3;
  tx_table[index].data = (uint32_t) tx_buffer[index];
  tx_table[index].ctrl = TXD_EN;
}

__attribute__((unused)) static void wait_tx_done(int index)
{
  WAIT_FOR((tx_table[index].ctrl & TXD_EN) == 0, "packet not transmitted");
}

#if defined(SCENARIO_tx_crc)
static uint8_t rmap_crc(const uint8_t *data, size_t size)
{
  uint8_t crc = 0;

  while (size-- > 0) {
    int bit;

    crc ^= *data++;
    for (bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (uint8_t) ((crc >> 1) ^ 0xE0) : (uint8_t) (crc >> 1);
    }
  }

  return crc;
}
#endif

static void run_scenario(void)
{
#if defined(SCENARIO_reset_values)
  uint32_t chn_interrupts = 0x1c;

  regs->rxl = 64;
  regs->rxd = (uint32_t) rx_table;
  regs->txd = (uint32_t) tx_table | 0x10;
  regs->ctr = CTR_LI | CTR_TQ | CTR_LS;
  regs->chn = chn_interrupts;

  /* LI and TQ keep the value of this write: the reset does not affect them. */
  regs->ctr = CTR_RS | CTR_LI | CTR_TQ;

#if CORE >= FIRST_RMAP_CORE
  /* RE and AS take the value of the rmapen input (1) on the RMAP cores. */
  if (regs->ctr != (CTR_LI | CTR_TQ | CTR_RA | CTR_RX | CTR_RC | CTR_RMAP_ENABLE | CTR_AS)) {
#else
  if (regs->ctr != (CTR_LI | CTR_TQ)) {
#endif
    fail("CTRL: LI and TQ are not reset, the other bits are");
  }
  if ((regs->str & STR_LINK_MASK) != 0) {
    fail("STS: the link state is reset");
  }
  if (regs->ndr != 254 || regs->tdr != 0x1F100) {
    fail("NODEADDR or TDR reset value");
  }
  if (regs->chn != chn_interrupts) {
    fail("DMACTRL: only AI, RI and TI are not reset");
  }
  if (regs->rxl != 64) {
    fail("RXMAXLEN is not reset");
  }
  if (regs->rxd != (uint32_t) rx_table || regs->txd != (uint32_t) tx_table) {
    fail("descriptor table: only the selector is reset");
  }
#elif defined(SCENARIO_rx_ri_only)
  setup();
  enable_rx_descriptor(0, RXD_IE);
  fill_tx_descriptor(0, NODE_ADDRESS);
  start_dma(CHN_RE | CHN_RD | CHN_RI | CHN_TE);

  WAIT_FOR(irq_count == 1, "no receive interrupt with RI alone");
#elif defined(SCENARIO_tx_burst)
  int i;

  setup();
  for (i = 0; i < DESCRIPTORS; ++i) {
    enable_rx_descriptor(i, 0);
    fill_tx_descriptor(i, NODE_ADDRESS);
  }

  /* One single write: the core has to go through the descriptors by itself. */
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((rx_table[DESCRIPTORS - 1].ctrl & RXD_EN) == 0, "burst not transmitted");
  WAIT_FOR((regs->chn & CHN_TE) == 0, "TE not cleared on a disabled descriptor");
#elif defined(SCENARIO_rx_no_spill)
  setup();
  fill_tx_descriptor(0, NODE_ADDRESS);
  start_dma(CHN_RE | CHN_NS | CHN_TE);
  wait_tx_done(0);
  rtems_task_wake_after(SETTLE_TICKS);

  enable_rx_descriptor(0, 0);
  start_dma(CHN_RE | CHN_NS | CHN_RD);

  WAIT_FOR((rx_table[0].ctrl & RXD_EN) == 0, "packet lost while waiting for a descriptor");
  if ((rx_table[0].ctrl & RXD_LEN_MASK) != 3 || rx_buffer[0][2] != 0) {
    fail("unexpected packet");
  }
#elif defined(SCENARIO_invalid_address)
  setup();
  enable_rx_descriptor(0, 0);
  fill_tx_descriptor(0, OTHER_ADDRESS);
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((regs->str & STR_IA) != 0, "IA not set");
  if ((rx_table[0].ctrl & RXD_EN) == 0) {
    fail("packet with an invalid address stored");
  }
#elif defined(SCENARIO_header)
  static const uint8_t header[] = { NODE_ADDRESS, PROTOCOL_ID };
  static const uint8_t expected[] = { NODE_ADDRESS, PROTOCOL_ID, 'A', 'B', 'C' };

  setup();
  enable_rx_descriptor(0, 0);
  memcpy(tx_buffer[0], "abc", 3);
  tx_table[0].header = (uint32_t) header;
  tx_table[0].len = 3;
  tx_table[0].data = (uint32_t) tx_buffer[0];
  tx_table[0].ctrl = TXD_EN | sizeof(header);
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((rx_table[0].ctrl & RXD_EN) == 0, "packet not received");
  if ((rx_table[0].ctrl & RXD_LEN_MASK) != sizeof(expected)
      || memcmp(rx_buffer[0], expected, sizeof(expected)) != 0) {
    fail("the header is not at the beginning of the packet");
  }
#elif defined(SCENARIO_time_codes)
  /* The peer answers to the packet with the time codes 62, 63 and 0. */
  setup();
  regs->tim = 61;
  regs->ctr = CTR_LS | CTR_TR | CTR_TQ | CTR_IE;
  fill_tx_descriptor(0, NODE_ADDRESS);
  start_dma(CHN_TE);

  WAIT_FOR(irq_count == 3, "the three tick-out interrupts were not taken");
  if (regs->tim != 0 || (regs->str & STR_TO) == 0) {
    fail("time counter not wrapped to 0, or TO not set");
  }
#elif defined(SCENARIO_tx_crc)
  static const uint8_t header[] = { NODE_ADDRESS, PROTOCOL_ID };
  static const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
  const uint8_t *packet = rx_buffer[0];

  setup();
  enable_rx_descriptor(0, 0);
  memcpy(tx_buffer[0], data, sizeof(data));
  tx_table[0].header = (uint32_t) header;
  tx_table[0].len = sizeof(data);
  tx_table[0].data = (uint32_t) tx_buffer[0];
  tx_table[0].ctrl = TXD_EN | TXD_HC | TXD_DC | (1U << TXD_NON_CRC_SHIFT) | sizeof(header);
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((rx_table[0].ctrl & RXD_EN) == 0, "packet not received");
  if ((rx_table[0].ctrl & RXD_LEN_MASK) != sizeof(header) + sizeof(data) + 2) {
    fail("the CRC bytes are missing");
  }
  if (packet[2] != rmap_crc(&header[1], sizeof(header) - 1)) {
    fail("bad header CRC, the first byte is not covered");
  }
  if (memcmp(&packet[3], data, sizeof(data)) != 0
      || packet[3 + sizeof(data)] != rmap_crc(data, sizeof(data))) {
    fail("bad data or data CRC");
  }
#elif defined(SCENARIO_no_rmap) || defined(SCENARIO_rmap_disabled)
  /*
   * A packet with the format of a RMAP read command and a wrong header CRC. It is
   * an ordinary packet for the DMA channel when the RMAP target is absent (cores 0
   * and 1) or disabled (cores 2 and 3: setup() clears RE).
   */
  static const uint8_t command[] = {
    NODE_ADDRESS, 0x01, 0x4C, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00
  };
  uint32_t tx_control = TXD_EN;
  uint32_t crc_flags = RXD_HC | RXD_DC;   /* The CRC goes on after a bad header */

#if defined(SCENARIO_no_rmap)
  /* No CRC logic on this core: the CRC bits have no effect. */
  tx_control |= TXD_HC | TXD_DC;
  crc_flags = 0;
#endif

  setup();
  enable_rx_descriptor(0, 0);
  memcpy(tx_buffer[0], command, sizeof(command));
  tx_table[0].header = 0;
  tx_table[0].len = sizeof(command);
  tx_table[0].data = (uint32_t) tx_buffer[0];
  tx_table[0].ctrl = tx_control;
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((rx_table[0].ctrl & RXD_EN) == 0, "the packet did not reach the DMA channel");
  if ((rx_table[0].ctrl & RXD_LEN_MASK) != sizeof(command)
      || memcmp(rx_buffer[0], command, sizeof(command)) != 0) {
    fail("the packet was changed");
  }
  if ((rx_table[0].ctrl & (RXD_HC | RXD_DC)) != crc_flags) {
    fail("unexpected HC or DC");
  }
#elif defined(SCENARIO_all_cores)
  /*
   * All the cores at the same time, with a different size and content for each
   * one: a packet must come back on its own core.
   */
  static rx_descriptor rx_tables[CORE_COUNT][128] __attribute__((aligned(1024)));
  static tx_descriptor tx_tables[CORE_COUNT][64] __attribute__((aligned(1024)));
  static uint8_t rx_data[CORE_COUNT][64] __attribute__((aligned(4)));
  static uint8_t tx_data[CORE_COUNT][64] __attribute__((aligned(4)));
  int core;

  for (core = 0; core < CORE_COUNT; ++core) {
    registers *const r = (registers *) CORE_ADDRESS(core);
    uint32_t size = 16 + 8 * core;

    r->ctr = CTR_RS;
    r->ctr = CTR_LS;
    r->ndr = NODE_ADDRESS;
    r->rxl = sizeof(rx_data[core]);
    r->rxd = (uint32_t) rx_tables[core];
    r->txd = (uint32_t) tx_tables[core];

    rx_tables[core][0].addr = (uint32_t) rx_data[core];
    rx_tables[core][0].ctrl = RXD_EN;

    memset(tx_data[core], 'A' + core, size);
    tx_data[core][0] = NODE_ADDRESS;
    tx_data[core][1] = PROTOCOL_ID;
    tx_tables[core][0].header = 0;
    tx_tables[core][0].len = size;
    tx_tables[core][0].data = (uint32_t) tx_data[core];
    tx_tables[core][0].ctrl = TXD_EN;
  }

  RTEMS_COMPILER_MEMORY_BARRIER();
  for (core = 0; core < CORE_COUNT; ++core) {
    ((registers *) CORE_ADDRESS(core))->chn = CHN_RE | CHN_RD | CHN_TE;
  }

  for (core = 0; core < CORE_COUNT; ++core) {
    uint32_t size = 16 + 8 * core;

    WAIT_FOR((rx_tables[core][0].ctrl & RXD_EN) == 0, "packet not received");
    RTEMS_COMPILER_MEMORY_BARRIER();
    if ((rx_tables[core][0].ctrl & RXD_LEN_MASK) != size
        || memcmp(rx_data[core], tx_data[core], size) != 0) {
      fail("packet received on the wrong core");
    }
  }
#elif defined(SCENARIO_rmap)
  /* rmap_test.c accesses this memory with RMAP commands. */
  static const uint8_t magic[] = { 'R', 'M', 'A', 'P', '-', 'O', 'K', '!' };
  static volatile uint8_t area[64] __attribute__((aligned(4)));
  /* The other RMAP core has its target disabled. */
  registers *const other = (registers *) CORE_ADDRESS(CORE ^ 1);
  uint32_t address = (uint32_t) area;
  size_t i;

  setup();
  regs->ctr = CTR_LS | CTR_RMAP_ENABLE;
  regs->key = 0x20;
  other->ndr = NODE_ADDRESS;
  other->ctr = CTR_LS;

  enable_rx_descriptor(0, 0);
  enable_rx_descriptor(1, 0);
  tx_buffer[0][0] = 0x10;
  tx_buffer[0][1] = PROTOCOL_ID;
  memcpy(&tx_buffer[0][2], &address, sizeof(address));
  tx_table[0].header = 0;
  tx_table[0].len = 2 + sizeof(address);
  tx_table[0].data = (uint32_t) tx_buffer[0];
  tx_table[0].ctrl = TXD_EN;
  start_dma(CHN_RE | CHN_RD | CHN_TE);

  WAIT_FOR((rx_table[1].ctrl & RXD_EN) == 0, "end of the RMAP test not received");
  if ((rx_table[0].ctrl & (RXD_HC | RXD_DC)) != RXD_DC) {
    fail("HC and DC of the packet with a bad data CRC");
  }
  for (i = 0; i < sizeof(magic); ++i) {
    if (area[i] != magic[i]) {
      fail("the RMAP write did not reach the guest memory");
    }
  }
#else
#error "Unknown scenario"
#endif
}

static rtems_task Init(rtems_task_argument arg)
{
  (void) arg;

  if (rtems_interrupt_handler_install(CORE_IRQ, "grspw", RTEMS_INTERRUPT_UNIQUE,
                                      count_irq, NULL) != RTEMS_SUCCESSFUL) {
    fail("rtems_interrupt_handler_install");
  }

  run_scenario();

  printf("TEST OK\n");
  exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 1
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
/* newlib's printf uses the FPU, which is forbidden in non-floating-point tasks. */
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
