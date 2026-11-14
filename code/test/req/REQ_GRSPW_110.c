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
 * REQ_GRSPW_110: RD makes the core read the descriptor again
 *
 * @tc     TC_GRSPW_024
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42f0a1 time=01
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t kept[] = { NODE_ADDRESS, PROTOCOL_ID, 0xA1 };

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  trigger(0);
  write_chn(CHN_RE | CHN_NS | CHN_TE);
  WAIT_FOR((regs->str & STR_TO) != 0, "time code of the peer not received");
  check(rx_length(0) == 0, "packet kept while RD = 0");
  enable_rx(0, rx_buffer[0], 0);
  write_chn(CHN_RE | CHN_NS | CHN_RD);
  wait_rx(0);
  check(rx_is(0, kept, sizeof(kept)), "kept packet stored once RD is written");
}
