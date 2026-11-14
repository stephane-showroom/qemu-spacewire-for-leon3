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
 * REQ_GRSPW_109: packets kept until a descriptor is available
 *
 * @tc     TC_GRSPW_025
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack seq=66 time=01
 */

#include "req_guest.h"

static void test(void)
{
  int i;

  setup();
  regs->rxl = SMALL_SIZE;
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  trigger(0);
  write_chn(CHN_RE | CHN_NS | CHN_TE);
  WAIT_FOR((regs->str & STR_TO) != 0, "time code of the peer not received");
  for (i = 0; i < 66; ++i) {
    enable_rx(i, small_rx[i], 0);
  }
  write_chn(CHN_RE | CHN_NS | CHN_RD);
  WAIT_FOR((rx_table[63].ctrl & RXD_EN) == 0, "the 64 kept packets are not stored");
  settle();
  for (i = 0; i < 64; ++i) {
    check((rx_table[i].ctrl & RXD_LEN_MASK) == 3 && small_rx[i][2] == i,
          "kept packets stored in arrival order");
  }
  check((rx_table[64].ctrl & RXD_EN) != 0 && (rx_table[65].ctrl & RXD_EN) != 0,
        "packets beyond 64 discarded");
}
