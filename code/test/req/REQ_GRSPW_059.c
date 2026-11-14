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
 * REQ_GRSPW_059: DMACTRL.RD
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_RD) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->chn & CHN_RD) == 0, "reset value");
  setup();
  write_chn(CHN_RD);
  check((regs->chn & CHN_RD) != 0, "reads back 1");
  write_chn(0);
  check((regs->chn & CHN_RD) == 0, "reads back 0");
  write_chn(CHN_RD);
  regs->ctr = CTR_RS;
  check((regs->chn & CHN_RD) == 0, "back to the reset value after CTRL.RS");

  setup();
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  WAIT_FOR((regs->chn & CHN_RD) == 0, "RD not cleared by a packet with a disabled descriptor");
}
