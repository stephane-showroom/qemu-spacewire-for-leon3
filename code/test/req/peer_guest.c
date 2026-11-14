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
 * Guest of the peer-side requirement tests (see req_peer.h). It enables the RMAP
 * target on cores 2 and 3 with node address 0x42 and destination key 0x20, sends
 * the address of a 64-byte memory area to the peer, then waits for the end packet
 * 42 F0 <status>. The checks themselves are done by the peer.
 */

#include "req_guest.h"

#define END_TICKS 900

static volatile uint8_t target_area[64] __attribute__((aligned(4)));

static void test(void)
{
  uint32_t address = (uint32_t) target_area;
  int ticks = 0;

  setup();
  regs->ctr = CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0);
  regs->key = DESTINATION_KEY;
  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(0, PEER_ADDRESS, &address, sizeof(address), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);

  while ((rx_table[0].ctrl & RXD_EN) != 0) {
    if (++ticks > END_TICKS) {
      fail("no end packet from the peer");
    }
    rtems_task_wake_after(1);
  }
  RTEMS_COMPILER_MEMORY_BARRIER();
  check(rx_buffer[0][2] == 0, "peer-side checks (see the peer log)");
}
