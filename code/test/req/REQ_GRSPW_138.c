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
 * REQ_GRSPW_138: packet longer than the transport limit
 *
 * @tc     TC_GRSPW_038
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static uint8_t big[TRANSPORT_LIMIT + 1] __attribute__((aligned(4)));
  static const uint8_t marker[] = { PEER_ADDRESS, PROTOCOL_ID, 'M' };

  setup();
  big[0] = PEER_ADDRESS;
  big[1] = PROTOCOL_ID;
  enable_rx(0, rx_buffer[0], 0);
  set_tx(0, big, sizeof(big), 0);
  set_tx(1, marker, sizeof(marker), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check((tx_table[0].ctrl & (TXD_EN | TXD_LE)) == TXD_LE, "completed with LE");
  check_report(0, MSG_DATA, sizeof(marker), marker, sizeof(marker), "the long packet is not sent");
}
