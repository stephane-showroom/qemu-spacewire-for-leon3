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
 * REQ_GRSPW_132: a disabled descriptor clears TE
 *
 * @tc     TC_GRSPW_031
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  int i;

  setup();
  for (i = 0; i < 3; ++i) {
    uint8_t value = (uint8_t) (0x80 | i);

    enable_rx(i, rx_buffer[i], 0);
    set_tx_bytes(i, NODE_ADDRESS, &value, 1, 0);
  }
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((rx_table[2].ctrl & RXD_EN) == 0, "chain not transmitted");
  WAIT_FOR((regs->chn & CHN_TE) == 0, "TE not cleared on the disabled descriptor");
  set_tx_bytes(3, NODE_ADDRESS, "\x83", 1, 0);
  settle();
  check((tx_table[3].ctrl & TXD_EN) != 0, "nothing sent until TE is written again");
}
