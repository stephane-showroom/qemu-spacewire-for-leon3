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
 * REQ_GRSPW_102: RX descriptor PACKETADDRESS
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t packet[] = { NODE_ADDRESS, PROTOCOL_ID, 0x81, 0x82, 0x83, 0x84 };
  uint8_t *const guard = rx_buffer[1];
  size_t start = IS_RMAP_CORE ? 5 : 4;
  size_t i;

  setup();
  memset(rx_buffer[2], 0, 16);
  enable_rx(0, rx_buffer[2], 0);
  set_tx(0, packet, sizeof(packet), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(memcmp(rx_buffer[2], packet, sizeof(packet)) == 0, "packet stored at the address of word 1");
  check(rx_table[0].addr == (uint32_t) rx_buffer[2], "word 1 not changed by the core");

  memset(guard, 0xCC, 16);
  enable_rx(1, guard + 5, 0);
  set_tx(1, packet, sizeof(packet), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  check(memcmp(guard + start, packet, sizeof(packet)) == 0,
        IS_RMAP_CORE ? "packet stored at the unaligned address"
                     : "packet stored at the word boundary below the address");
  for (i = 0; i < start; ++i) {
    check(guard[i] == 0xCC, "nothing written before the packet");
  }
  if (IS_RMAP_CORE) {
    check(guard[start + sizeof(packet)] == 0xCC, "nothing written after the packet");
  }
  check(rx_table[1].addr == (uint32_t) (guard + 5), "unaligned word 1 not changed by the core");
}
