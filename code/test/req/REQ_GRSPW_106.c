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
 * REQ_GRSPW_106: short packets
 *
 * @tc     TC_GRSPW_022
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42 send=4201 send=42f0 send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t rmap2[] = { NODE_ADDRESS, 0x01 };
  static const uint8_t user2[] = { NODE_ADDRESS, PROTOCOL_ID };
  static const uint8_t marker[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };
  int next = 0;
  int i;

  setup();
  regs->ctr = CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0);
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0);
  }
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(IS_RMAP_CORE ? 1 : 2);
  if (!IS_RMAP_CORE) {
    check(rx_is(next++, rmap2, sizeof(rmap2)), "2 bytes with protocol 1 stored without RMAP");
  }
  check(rx_is(next++, user2, sizeof(user2)), "2 bytes with another protocol stored");
  check(rx_is(next, marker, sizeof(marker)), "shorter packets discarded");
  check((regs->str & STR_EE) != 0, "EE set");
}
