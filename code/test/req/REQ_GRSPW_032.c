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
 * REQ_GRSPW_032: STS.EE
 *
 * @tc     TC_GRSPW_059
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42 send=42f0e1 wait ack send=42 send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };

  check((regs->str & STR_EE) == 0, "power-on value");
  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS;
  regs->str = STR_EE;
  check((regs->str & STR_EE) == 0, "a write of 1 does not set it");

  enable_rx(0, rx_buffer[0], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, marker, sizeof(marker)), "only the marker is stored");
  check((regs->str & STR_EE) != 0, "set by a packet of 1 byte");
  regs->str = 0;
  check((regs->str & STR_EE) != 0, "unchanged by a write of 0");
  regs->str = (STR_ERRORS | STR_TO) & ~STR_EE;
  check((regs->str & STR_EE) != 0, "unchanged by a write of 1 to the other bits");
  regs->str = STR_EE;
  check((regs->str & STR_EE) == 0, "cleared by a write of 1");

  enable_rx(1, rx_buffer[1], 0);
  trigger(1);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  check((regs->str & STR_EE) != 0, "set again by a packet of 1 byte");
  regs->ctr = CTR_RS;
  check((regs->str & STR_EE) == 0, "back to the reset value after CTRL.RS");
}
