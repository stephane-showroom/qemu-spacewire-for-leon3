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
 * REQ_GRSPW_092: TT and TR together
 *
 * @tc     TC_GRSPW_015
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report time=0c send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  enable_rx(0, rx_buffer[0], 0);
  enable_rx(1, rx_buffer[1], 0);
  write_chn(CHN_RE | CHN_RD);
  regs->tim = 0x0A;
  regs->ctr = CTR_LS | CTR_TT | CTR_TR | CTR_TI;
  wait_rx(1);
  check_report(0, MSG_TIME, 1, "\x0b", 1, "time code sent with TT and TR");
  check(regs->tim == 0x0C && (regs->str & STR_TO) != 0, "time code received with TT and TR");
}
