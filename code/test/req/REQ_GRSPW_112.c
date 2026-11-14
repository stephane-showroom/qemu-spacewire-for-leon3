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
 * REQ_GRSPW_112: receive CRC check
 *
 * @tc     TC_GRSPW_021, TC_GRSPW_028
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42010c00670001000000045c11223344ca send=42010c0067000100000004a311223344ca send=42010c00670001000000045c1122334435 send=42f01020304050607080b3
 */

#include "req_guest.h"

static void test(void)
{
  int i;

  setup();
  regs->ctr = CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0);
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0);
  }
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(3);
  if (IS_RMAP_CORE) {
    check((rx_table[0].ctrl & (RXD_HC | RXD_DC)) == 0, "valid read reply: HC = DC = 0");
    check((rx_table[1].ctrl & (RXD_HC | RXD_DC)) == (RXD_HC | RXD_DC), "bad header CRC: HC = DC = 1");
    check((rx_table[2].ctrl & (RXD_HC | RXD_DC)) == RXD_DC, "bad data CRC: HC = 0, DC = 1");
    check((rx_table[3].ctrl & RXD_DC) == 0, "packet ended by its RMAP CRC: DC = 0");
  } else {
    for (i = 0; i < 4; ++i) {
      check((rx_table[i].ctrl & (RXD_HC | RXD_DC)) == 0, "HC and DC always 0 without RMAP CRC");
    }
  }
}
