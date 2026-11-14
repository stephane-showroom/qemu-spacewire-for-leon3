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
 * REQ_GRSPW_060: DMACTRL.RX
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_RX) == 0, "power-on value");
  setup();
  write_chn(CHN_RX);
  check((regs->chn & CHN_RX) == 0, "unchanged by a write of 1");
  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check((regs->chn & CHN_RX) == 0, "RX reads 0 once the packet is stored");
}
