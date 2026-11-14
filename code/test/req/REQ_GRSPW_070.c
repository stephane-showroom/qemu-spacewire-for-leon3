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
 * REQ_GRSPW_070: DMACTRL.TE
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait sleep=1000 ack
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_TE) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->chn & CHN_TE) == 0, "reset value");
  setup();
  write_chn(CHN_TE);
  check((regs->chn & CHN_TE) == 0, "cleared by the core on a disabled descriptor");
  trigger(0);
  write_chn(CHN_TE);
  check((regs->chn & CHN_TE) != 0, "reads 1 while the packet waits for its acknowledge");
  wait_tx(0);
  WAIT_FOR((regs->chn & CHN_TE) == 0, "TE not cleared on the next, disabled, descriptor");
}
