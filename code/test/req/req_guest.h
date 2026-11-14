/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 14, 2026                                      *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * Common part of the guest-side requirement tests (REQ_GRSPW_XXX.c with
 * "@side guest"). A test file includes this header and defines test(). The
 * program is built with -DCORE=<n>, prints one "CHECK FAILED" line per failed
 * check, and TEST OK when all checks passed.
 *
 * With peer_script, a packet "reported" by the peer is
 * 42 F0 <message type> <packet length, 4 bytes big-endian> <first bytes>.
 */

#ifndef REQ_GUEST_H_
#define REQ_GUEST_H_

#include <rtems.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CORE_COUNT        4
#define CORE_ADDRESS(core) (0x80000A00U + 0x100U * (core))
#define CORE_BASE         CORE_ADDRESS(CORE)
#define CORE_IRQ(core)    (10 + (core))
#define IS_RMAP_CORE      (CORE >= 2)

#define CTR_LD            (1U << 0)
#define CTR_LS            (1U << 1)
#define CTR_AS            (1U << 2)
#define CTR_IE            (1U << 3)
#define CTR_TI            (1U << 4)
#define CTR_PM            (1U << 5)
#define CTR_RS            (1U << 6)
#define CTR_TQ            (1U << 8)
#define CTR_LI            (1U << 9)
#define CTR_TT            (1U << 10)
#define CTR_TR            (1U << 11)
#define CTR_RE            (1U << 16)
#define CTR_RD            (1U << 17)
#define CTR_NP            (1U << 20)
#define CTR_PS            (1U << 21)
#define CTR_PO            (1U << 26)
#define CTR_NCH           (3U << 27)
#define CTR_RC            (1U << 29)
#define CTR_RX            (1U << 30)
#define CTR_RA            (1U << 31)
#define CTR_CAPABILITIES  (CTR_RA | CTR_RX | CTR_RC)
#define CTR_RESERVED      0x1FFCF080U

#define STR_TO            (1U << 0)
#define STR_IA            (1U << 7)
#define STR_EE            (1U << 8)
#define STR_ERRORS        0x1FEU
#define STR_RESERVED      0xFF1FFE20U
#define STR_LINK(str)     (((str) >> 21) & 7U)
#define LINK_RUN          5U

#define CHN_TE            (1U << 0)
#define CHN_RE            (1U << 1)
#define CHN_TI            (1U << 2)
#define CHN_RI            (1U << 3)
#define CHN_AI            (1U << 4)
#define CHN_PS            (1U << 5)
#define CHN_PR            (1U << 6)
#define CHN_TA            (1U << 7)
#define CHN_RA            (1U << 8)
#define CHN_AT            (1U << 9)
#define CHN_RX            (1U << 10)
#define CHN_RD            (1U << 11)
#define CHN_NS            (1U << 12)
#define CHN_LE            (1U << 16)
#define CHN_RESERVED      0xFFFEE000U

#define RXD_LEN_MASK      0x1ffffffU
#define RXD_EN            (1U << 25)
#define RXD_WR            (1U << 26)
#define RXD_IE            (1U << 27)
#define RXD_EP            (1U << 28)
#define RXD_HC            (1U << 29)
#define RXD_DC            (1U << 30)
#define RXD_TR            (1U << 31)
#define TXD_EN            (1U << 12)
#define TXD_WR            (1U << 13)
#define TXD_IE            (1U << 14)
#define TXD_LE            (1U << 15)
#define TXD_HC            (1U << 16)
#define TXD_DC            (1U << 17)
#define TXD_NON_CRC(n)    ((uint32_t) (n) << 8)

#define NODE_ADDRESS      0x42
#define PEER_ADDRESS      0x67
#define PROTOCOL_ID       0xF0
#define DESTINATION_KEY   0x20
#define MSG_DATA          1
#define MSG_TIME          2
#define MAX_LENGTH        1024
#define BUFFERS           8
#define SMALL_BUFFERS     130
#define SMALL_SIZE        16
#define TRANSPORT_LIMIT   8185
#define UNMAPPED_ADDRESS  0x10000000U
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
__attribute__((unused)) static uint8_t rx_buffer[BUFFERS][MAX_LENGTH] __attribute__((aligned(4)));
__attribute__((unused)) static uint8_t tx_buffer[BUFFERS][MAX_LENGTH] __attribute__((aligned(4)));
__attribute__((unused)) static uint8_t small_rx[SMALL_BUFFERS][SMALL_SIZE] __attribute__((aligned(4)));
__attribute__((unused)) static volatile uint8_t area[16] __attribute__((aligned(4)));

static volatile int irq_count[CORE_COUNT];
static int failures;

static void test(void);

__attribute__((unused)) static void fail(const char *message)
{
  printf("TEST FAILED: %s\n", message);
  exit(1);
}

__attribute__((unused)) static void check(int condition, const char *message)
{
  if (!condition) {
    printf("CHECK FAILED: %s\n", message);
    ++failures;
  }
}

#define WAIT_FOR(condition, message) \
  do { \
    int wait_ticks = 0; \
    while (!(condition)) { \
      if (++wait_ticks > WAIT_TICKS) { \
        fail(message); \
      } \
      rtems_task_wake_after(1); \
    } \
    RTEMS_COMPILER_MEMORY_BARRIER(); \
  } while (0)

static void count_irq(void *arg)
{
  ++irq_count[(int) (intptr_t) arg];
}

/* The compiler must not move the buffer accesses after the start of the DMA. */
__attribute__((unused)) static void write_chn(uint32_t value)
{
  RTEMS_COMPILER_MEMORY_BARRIER();
  regs->chn = value;
}

__attribute__((unused)) static void settle(void)
{
  rtems_task_wake_after(SETTLE_TICKS);
}

/* Reset of the core, link started, node address and tables set. RE is cleared. */
__attribute__((unused)) static void setup(void)
{
  memset((void *) rx_table, 0, sizeof(rx_table));
  memset((void *) tx_table, 0, sizeof(tx_table));
  regs->ctr = CTR_RS;
  regs->ctr = CTR_LS;
  regs->ndr = NODE_ADDRESS;
  regs->rxl = MAX_LENGTH;
  regs->rxd = (uint32_t) rx_table;
  regs->txd = (uint32_t) tx_table;
}

__attribute__((unused)) static void enable_rx(int index, void *buffer, uint32_t flags)
{
  rx_table[index].addr = (uint32_t) buffer;
  RTEMS_COMPILER_MEMORY_BARRIER();
  rx_table[index].ctrl = RXD_EN | flags;
}

/* A packet without header; flags are added to word 0. */
__attribute__((unused)) static void set_tx(int index, const void *data, uint32_t size, uint32_t flags)
{
  tx_table[index].header = 0;
  tx_table[index].len = size;
  tx_table[index].data = (uint32_t) data;
  RTEMS_COMPILER_MEMORY_BARRIER();
  tx_table[index].ctrl = TXD_EN | flags;
}

/* A packet in tx_buffer[index]: destination, protocol identifier, then the bytes. */
__attribute__((unused)) static void set_tx_bytes(int index, uint8_t destination,
                                                 const void *bytes, uint32_t size, uint32_t flags)
{
  tx_buffer[index][0] = destination;
  tx_buffer[index][1] = PROTOCOL_ID;
  memcpy(&tx_buffer[index][2], bytes, size);
  set_tx(index, tx_buffer[index], size + 2, flags);
}

/* A one-byte packet to the peer, used to start a step of peer_script. */
__attribute__((unused)) static void trigger(int index)
{
  uint8_t value = (uint8_t) ('0' + index);

  set_tx_bytes(index, PEER_ADDRESS, &value, 1, 0);
}

/* A packet for peer_script whose bytes 2 to 5 are the address of area. */
__attribute__((unused)) static void set_tx_area_address(int index)
{
  uint32_t address = (uint32_t) area;

  set_tx_bytes(index, PEER_ADDRESS, &address, sizeof(address), 0);
}

__attribute__((unused)) static void wait_tx(int index)
{
  WAIT_FOR((tx_table[index].ctrl & TXD_EN) == 0, "packet not transmitted");
}

__attribute__((unused)) static void wait_rx(int index)
{
  WAIT_FOR((rx_table[index].ctrl & RXD_EN) == 0, "packet not received");
}

__attribute__((unused)) static uint32_t rx_length(int index)
{
  return rx_table[index].ctrl & RXD_LEN_MASK;
}

__attribute__((unused)) static int rx_is(int index, const void *bytes, uint32_t size)
{
  return rx_length(index) == size && memcmp(rx_buffer[index], bytes, size) == 0;
}

/* Checks a packet reported by peer_script in rx_buffer[index]. */
__attribute__((unused)) static void check_report(int index, uint8_t type, uint32_t size,
                                                 const void *bytes, uint32_t shown,
                                                 const char *message)
{
  const uint8_t *report = rx_buffer[index];
  uint32_t reported = (uint32_t) report[3] << 24 | (uint32_t) report[4] << 16
                    | (uint32_t) report[5] << 8 | report[6];

  check(report[0] == NODE_ADDRESS && report[1] == PROTOCOL_ID && report[2] == type
        && reported == size && memcmp(&report[7], bytes, shown) == 0, message);
}

__attribute__((unused)) static uint8_t rmap_crc(const uint8_t *data, size_t size)
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

/* peer_script "rmapw" writes "OK!!" in area. */
__attribute__((unused)) static int area_written(void)
{
  return area[0] == 'O' && area[1] == 'K' && area[2] == '!' && area[3] == '!';
}

static rtems_task Init(rtems_task_argument arg)
{
  (void) arg;

  if (rtems_interrupt_handler_install(CORE_IRQ(CORE), "grspw", RTEMS_INTERRUPT_UNIQUE,
                                      count_irq, (void *) (intptr_t) CORE) != RTEMS_SUCCESSFUL) {
    fail("rtems_interrupt_handler_install");
  }

  test();

  if (failures != 0) {
    printf("TEST FAILED: %d check(s)\n", failures);
    exit(1);
  }
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

#endif /* REQ_GUEST_H_ */
