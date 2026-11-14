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
 * REQ_GRSPW_093: receive descriptor layout
 *
 * @tc     TC_GRSPW_027
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t packet[] = { NODE_ADDRESS, PROTOCOL_ID, 0x81, 0x82, 0x83 };

  setup();
  memset(rx_buffer[2], 0, 16);
  enable_rx(0, rx_buffer[2], 0);
  set_tx(0, packet, sizeof(packet), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(memcmp(rx_buffer[2], packet, sizeof(packet)) == 0, "packet stored at the address of word 1");
  check((rx_table[0].ctrl & RXD_EN) == 0, "EN (bit 25) cleared");
  check((rx_table[0].ctrl & RXD_LEN_MASK) == sizeof(packet), "length in bits 24:0");
}
