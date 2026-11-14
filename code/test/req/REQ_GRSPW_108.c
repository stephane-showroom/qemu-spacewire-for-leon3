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
 * REQ_GRSPW_108: storing a packet
 *
 * @tc     TC_GRSPW_017, TC_GRSPW_026
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t header[] = { NODE_ADDRESS, PROTOCOL_ID };
  static const uint8_t expected[] = { NODE_ADDRESS, PROTOCOL_ID, 0x81, 0x82, 0x83 };
  int i;

  setup();
  enable_rx(0, rx_buffer[0], 0);
  tx_buffer[0][0] = 0x81;
  tx_buffer[0][1] = 0x82;
  tx_buffer[0][2] = 0x83;
  tx_table[0].header = (uint32_t) header;
  tx_table[0].len = 3;
  tx_table[0].data = (uint32_t) tx_buffer[0];
  tx_table[0].ctrl = TXD_EN | sizeof(header);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, expected, sizeof(expected)), "whole packet stored, address included, EOP excluded");
  check((regs->chn & CHN_PR) != 0, "PR set");
  check((regs->rxd & 0x3F8U) == 1U << 3, "selector advanced");

  regs->rxl = 16;
  memset(rx_buffer[1], 0xAA, 64);
  for (i = 0; i < 40; ++i) {
    tx_buffer[1][i] = (uint8_t) (0x80 | i);
  }
  tx_buffer[1][0] = NODE_ADDRESS;
  tx_buffer[1][1] = PROTOCOL_ID;
  enable_rx(1, rx_buffer[1], 0);
  set_tx(1, tx_buffer[1], 40, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  check(rx_length(1) == 16 && (rx_table[1].ctrl & RXD_TR) != 0, "truncated to DMAMAXLEN, TR set");
  check(memcmp(rx_buffer[1], tx_buffer[1], 16) == 0, "first bytes stored");
  for (i = 16; i < 64; ++i) {
    check(rx_buffer[1][i] == 0xAA, "nothing written after the maximum length");
  }
  enable_rx(2, rx_buffer[2], 0);
  set_tx(2, tx_buffer[1], 16, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(2);
  check(rx_length(2) == 16 && (rx_table[2].ctrl & RXD_TR) == 0, "packet of exactly DMAMAXLEN bytes");
}
