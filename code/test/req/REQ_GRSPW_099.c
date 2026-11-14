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
 * REQ_GRSPW_099: RX descriptor WR
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  enable_rx(0, rx_buffer[0], RXD_WR);
  enable_rx(1, rx_buffer[1], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check((rx_table[0].ctrl & RXD_WR) != 0, "WR kept when the core writes word 0");
  check((regs->rxd & 0x3F8U) == 0, "next descriptor is the first of the table");
  enable_rx(0, rx_buffer[0], RXD_WR);
  set_tx_bytes(1, NODE_ADDRESS, "\x82", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_buffer[0][2] == 0x82, "second packet stored in descriptor 0");
  check((rx_table[1].ctrl & RXD_EN) != 0, "descriptor 1 not used");
}
