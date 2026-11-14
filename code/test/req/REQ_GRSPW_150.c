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
 * REQ_GRSPW_150: packet limit of 8185 bytes
 *
 * @tc     TC_GRSPW_038
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static uint8_t big[TRANSPORT_LIMIT] __attribute__((aligned(4)));

  setup();
  memset(big, 0x11, sizeof(big));
  big[0] = PEER_ADDRESS;
  big[1] = PROTOCOL_ID;
  enable_rx(0, rx_buffer[0], 0);
  set_tx(0, big, sizeof(big), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check_report(0, MSG_DATA, TRANSPORT_LIMIT, big, 8, "a packet of 8185 bytes is sent");
  check((tx_table[0].ctrl & TXD_LE) == 0, "completed without error");
}
