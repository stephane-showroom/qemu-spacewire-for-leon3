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
 * REQ_GRSPW_042: STS bits not written back stay set
 *
 * The race itself (a bit set between the read and the write of the interrupt
 * handler) cannot be produced on purpose: the test checks the property it
 * relies on, that a bit not written with 1 is not cleared.
 *
 * @tc     TC_GRSPW_008
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=55f001 time=01 send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  uint32_t seen;

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  enable_rx(0, rx_buffer[0], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  seen = regs->str & STR_IA;
  check(seen != 0 && (regs->str & STR_TO) != 0, "IA and TO set");
  regs->str = seen;
  check((regs->str & (STR_IA | STR_TO)) == STR_TO, "writing back IA leaves TO set");
  regs->str = STR_TO;
  check((regs->str & STR_TO) == 0, "TO cleared when written");
}
