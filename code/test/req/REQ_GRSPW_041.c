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
 * REQ_GRSPW_041: STS cleared by a write of all ones
 *
 * @tc     TC_GRSPW_008
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=55f001 send=42 time=01 send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR | (IS_RMAP_CORE ? CTR_RE : 0);
  enable_rx(0, rx_buffer[0], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, marker, sizeof(marker)), "only the marker is stored");
  check((regs->str & (STR_IA | STR_EE | STR_TO)) == (STR_IA | STR_EE | STR_TO), "IA, EE and TO set");
  regs->str = 0xFFFFFFFFU;
  check((regs->str & (STR_IA | STR_EE | STR_TO)) == 0, "IA, EE and TO cleared");
  check(STR_LINK(regs->str) == LINK_RUN, "link state unchanged");
}
