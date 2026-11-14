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
 * REQ_GRSPW_115: transmit descriptor layout
 *
 * @tc     TC_GRSPW_033
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t header[] = { PEER_ADDRESS, PROTOCOL_ID, 0x11 };
  static const uint8_t data[] = { 0x22, 0x33 };
  static const uint8_t expected[] = { PEER_ADDRESS, PROTOCOL_ID, 0x11, 0x22, 0x33 };

  setup();
  enable_rx(0, rx_buffer[0], 0);
  tx_table[0].header = (uint32_t) header;
  tx_table[0].len = sizeof(data);
  tx_table[0].data = (uint32_t) data;
  RTEMS_COMPILER_MEMORY_BARRIER();
  tx_table[0].ctrl = TXD_EN | TXD_WR | sizeof(header);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check_report(0, MSG_DATA, sizeof(expected), expected, sizeof(expected),
               "header from word 1 with the length of word 0, data from word 3 with the length of word 2");
  check((tx_table[0].ctrl & TXD_EN) == 0, "EN (bit 12) cleared");
  check((regs->txd & 0x3F0U) == 0, "WR (bit 13): next descriptor is the first one");
}
