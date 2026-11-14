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
 * REQ_GRSPW_103: receive descriptor completion
 *
 * @tc     TC_GRSPW_027
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t packet[] = { NODE_ADDRESS, PROTOCOL_ID, 0x81, 0x82, 0x83, 0x84 };

  setup();
  enable_rx(0, rx_buffer[0], RXD_IE | RXD_WR);
  set_tx(0, packet, sizeof(packet), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check((rx_table[0].ctrl & (RXD_IE | RXD_WR)) == (RXD_IE | RXD_WR), "IE and WR kept");
  check(rx_length(0) == sizeof(packet), "length written");
  check((rx_table[0].ctrl & (RXD_TR | RXD_EP)) == 0, "no error bit");
}
