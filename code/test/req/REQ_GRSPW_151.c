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
 * REQ_GRSPW_151: the guest is not blocked by the link
 *
 * @tc     TC_GRSPW_035, TC_GRSPW_045
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait send=42f0e1 sleep=500 ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };

  setup();
  enable_rx(0, rx_buffer[0], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, marker, sizeof(marker)), "packet received while the transmission waits");
  check((tx_table[0].ctrl & TXD_EN) != 0, "transmission still waiting for its acknowledge");
  wait_tx(0);
}
