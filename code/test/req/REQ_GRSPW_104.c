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
 * REQ_GRSPW_104: address check and promiscuous mode
 *
 * @tc     TC_GRSPW_018, TC_GRSPW_019
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=55f0a1 send=42f0e1 wait ack send=55f0a2 send=42f0e2
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker1[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };
  static const uint8_t other[] = { 0x55, PROTOCOL_ID, 0xA2 };
  static const uint8_t marker2[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE2 };

  setup();
  enable_rx(0, rx_buffer[0], 0);
  enable_rx(1, rx_buffer[1], 0);
  enable_rx(2, rx_buffer[2], 0);
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, marker1, sizeof(marker1)), "packet for another address discarded");
  check((regs->str & STR_IA) != 0, "IA set");
  regs->str = STR_IA;
  regs->ctr = CTR_LS | CTR_PM;
  trigger(1);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(2);
  check(rx_is(1, other, sizeof(other)), "packet for another address stored in promiscuous mode");
  check(rx_is(2, marker2, sizeof(marker2)), "marker stored");
  check((regs->str & STR_IA) == 0, "IA not set in promiscuous mode");
}
