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
 * REQ_GRSPW_129: TE starts the transmitter
 *
 * @tc     TC_GRSPW_031
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
  write_chn(CHN_RE | CHN_RD);
  settle();
  check((tx_table[0].ctrl & TXD_EN) != 0, "nothing sent before TE");
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  wait_rx(0);
  check(rx_length(0) == 3, "packet sent once TE is written");
}
