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
 * REQ_GRSPW_121: TX descriptor EN
 *
 * @tc     TC_GRSPW_064
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  tx_table[0].ctrl &= ~TXD_EN;
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  check((regs->chn & CHN_TE) == 0, "a disabled descriptor stops the transmitter");
  settle();
  check((rx_table[0].ctrl & RXD_EN) != 0, "nothing sent from a disabled descriptor");
  check((regs->txd & 0x3F0U) == 0, "selector not advanced");
  tx_table[0].ctrl |= TXD_EN;
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  wait_rx(0);
  check(rx_buffer[0][2] == 0x81, "the enabled descriptor is sent");
  check((tx_table[0].ctrl & TXD_EN) == 0, "EN cleared when the packet has been sent");
}
