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
 * REQ_GRSPW_107: spill rules
 *
 * @tc     TC_GRSPW_023, TC_GRSPW_024
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42f0a1 time=01 wait ack send=42f0a2 time=02 wait ack send=42f0a3 time=03 wait ack send=42f0ff time=04
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t last[] = { NODE_ADDRESS, PROTOCOL_ID, 0xFF };
  static const uint32_t config[] = { 0, CHN_RE, CHN_RE | CHN_RD };
  static const char *const names[] = {
    "discarded with RE = 0", "discarded with RD = 0 and NS = 0",
    "discarded on a disabled descriptor with NS = 0"
  };
  int i;

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  for (i = 0; i < 3; ++i) {
    if (i == 2) {
      rx_table[0].ctrl = 0;
    } else {
      enable_rx(0, rx_buffer[0], 0);
    }
    trigger(i);
    write_chn(config[i] | CHN_TE);
    WAIT_FOR((regs->str & STR_TO) != 0, "time code of the peer not received");
    regs->str = STR_TO;
    check(rx_length(0) == 0, names[i]);
    if (i == 2) {
      check((regs->chn & CHN_RD) == 0, "RD cleared by a disabled descriptor");
    }
  }
  enable_rx(0, rx_buffer[0], 0);
  trigger(3);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_is(0, last, sizeof(last)), "packet stored with RE, RD and an enabled descriptor");
}
