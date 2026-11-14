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
 * REQ_GRSPW_088: tick-in with TT
 *
 * @tc     TC_GRSPW_015
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report wait report wait report wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t marker[] = { PEER_ADDRESS, PROTOCOL_ID, '0' };
  static const uint8_t codes[] = { 0x7F, 0x40, 0x41 };
  int i;

  setup();
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0);
  }
  write_chn(CHN_RE | CHN_RD);
  regs->tim = 0x7E;
  for (i = 0; i < 3; ++i) {
    regs->ctr = CTR_LS | CTR_TT | CTR_TI;
  }
  check(regs->tim == 0x41, "counter incremented modulo 64, control flags kept");
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(3);
  for (i = 0; i < 3; ++i) {
    check_report(i, MSG_TIME, 1, &codes[i], 1, "time code with the new counter value");
  }
  check_report(3, MSG_DATA, sizeof(marker), marker, sizeof(marker), "one time code per tick-in");
}
