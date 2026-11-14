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
 * REQ_GRSPW_120: TX descriptor WR
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
  enable_rx(1, rx_buffer[1], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, TXD_WR);
  set_tx_bytes(1, NODE_ADDRESS, "\x82", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  wait_rx(0);
  WAIT_FOR((regs->chn & CHN_TE) == 0, "TE not cleared by descriptor 0, now disabled");
  check((tx_table[0].ctrl & TXD_WR) != 0, "WR kept when the core writes word 0");
  check((regs->txd & 0x3F0U) == 0, "next descriptor is the first of the table");
  check((tx_table[1].ctrl & TXD_EN) != 0, "descriptor 1 not sent");
}
