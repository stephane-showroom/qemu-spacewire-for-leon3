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
 * REQ_GRSPW_061: DMACTRL.AT
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_AT) == 0, "power-on value");
  setup();
  write_chn(CHN_AT);
  check((regs->chn & CHN_AT) == 0, "AT reads 0 after a write of 1");
  write_chn(CHN_RE | CHN_AT);
  check((regs->chn & (CHN_AT | CHN_RE)) == CHN_RE, "AT reads 0, the other bits are written");
}
