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
 * REQ_GRSPW_089: tick-in without TT
 *
 * @tc     TC_GRSPW_015
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { PEER_ADDRESS, PROTOCOL_ID, '0' };

  setup();
  enable_rx(0, rx_buffer[0], 0);
  write_chn(CHN_RE | CHN_RD);
  regs->tim = 0x41;
  regs->ctr = CTR_LS | CTR_TI;
  check(regs->tim == 0x41, "no increment without TT");
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check_report(0, MSG_DATA, sizeof(marker), marker, sizeof(marker), "nothing sent without TT");
}
