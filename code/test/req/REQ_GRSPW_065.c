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
 * REQ_GRSPW_065: DMACTRL.PS
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_PS) == 0, "power-on value");
  setup();
  write_chn(CHN_PS);
  check((regs->chn & CHN_PS) == 0, "a write of 1 does not set it");

  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  WAIT_FOR((regs->chn & CHN_PS) != 0, "not set by a sent packet");
  write_chn(CHN_RE);
  check((regs->chn & CHN_PS) != 0, "unchanged by a write of 0");
  write_chn(CHN_RE | ((CHN_PS | CHN_PR | CHN_TA | CHN_RA) & ~CHN_PS));
  check((regs->chn & CHN_PS) != 0, "unchanged by a write of 1 to the other status bits");
  write_chn(CHN_RE | CHN_PS);
  check((regs->chn & CHN_PS) == 0, "cleared by a write of 1");

  enable_rx(1, rx_buffer[1], 0);
  set_tx_bytes(1, NODE_ADDRESS, "\x82", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(1);
  WAIT_FOR((regs->chn & CHN_PS) != 0, "not set again by a sent packet");
  regs->ctr = CTR_RS;
  check((regs->chn & CHN_PS) == 0, "back to the reset value after CTRL.RS");
}
