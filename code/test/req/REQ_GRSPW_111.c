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
 * REQ_GRSPW_111: clearing RE
 *
 * @tc     TC_GRSPW_023
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42f0a1 time=01 wait ack send=42f0a2 time=02
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t first[] = { NODE_ADDRESS, PROTOCOL_ID, 0xA1 };

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  enable_rx(0, rx_buffer[0], 0);
  enable_rx(1, rx_buffer[1], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((regs->str & STR_TO) != 0, "first time code of the peer not received");
  regs->str = STR_TO;
  check(rx_is(0, first, sizeof(first)), "packet stored with RE");
  trigger(1);
  write_chn(CHN_RD | CHN_TE);
  WAIT_FOR((regs->str & STR_TO) != 0, "second time code of the peer not received");
  check((rx_table[1].ctrl & RXD_EN) != 0, "packet discarded once RE is cleared");
}
