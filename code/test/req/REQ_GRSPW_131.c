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
 * REQ_GRSPW_131: descriptor with both lengths 0
 *
 * @tc     TC_GRSPW_034
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { PEER_ADDRESS, PROTOCOL_ID, 'M' };

  setup();
  enable_rx(0, rx_buffer[0], 0);
  tx_table[0].header = 0;
  tx_table[0].len = 0;
  tx_table[0].data = 0;
  tx_table[0].ctrl = TXD_EN | (IS_RMAP_CORE ? TXD_HC | TXD_DC : 0);
  set_tx(1, marker, sizeof(marker), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check_report(0, MSG_DATA, sizeof(marker), marker, sizeof(marker), "nothing sent, not even an EOP");
  check((tx_table[0].ctrl & TXD_EN) == 0, "descriptor completed");
  check((regs->chn & CHN_PS) != 0, "PS set");
}
