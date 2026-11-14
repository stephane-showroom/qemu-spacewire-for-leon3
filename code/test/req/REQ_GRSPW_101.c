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
 * REQ_GRSPW_101: RX descriptor PACKETLENGTH
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t sizes[] = { 2, 5, 100, 1024 };
  int i;

  setup();
  tx_buffer[0][0] = NODE_ADDRESS;
  tx_buffer[0][1] = PROTOCOL_ID;
  for (i = 2; i < MAX_LENGTH; ++i) {
    tx_buffer[0][i] = (uint8_t) i;
  }
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0x12345);
    set_tx(i, tx_buffer[0], sizes[i], 0);
    write_chn(CHN_RE | CHN_RD | CHN_TE);
    wait_rx(i);
    check(rx_length(i) == sizes[i], "number of bytes stored, whatever the guest wrote");
  }
}
