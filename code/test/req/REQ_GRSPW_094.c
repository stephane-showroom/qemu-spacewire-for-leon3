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
 * REQ_GRSPW_094: RX descriptor TR
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t packet[12] = { NODE_ADDRESS, PROTOCOL_ID, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

  setup();
  regs->rxl = 8;
  enable_rx(0, rx_buffer[0], RXD_TR);
  set_tx(0, packet, 8, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check((rx_table[0].ctrl & RXD_TR) == 0, "0 for a packet not cut, though the guest set it");
  enable_rx(1, rx_buffer[1], 0);
  set_tx(1, packet, sizeof(packet), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  check((rx_table[1].ctrl & RXD_TR) != 0, "1 for a packet longer than DMAMAXLEN");
  check(rx_length(1) == 8, "the packet is cut at DMAMAXLEN");
}
