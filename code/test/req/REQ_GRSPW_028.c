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
 * REQ_GRSPW_028: CTRL.LD
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->ctr & CTR_LD) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_LD) == 0, "reset value");
  setup();
  regs->ctr = CTR_LD;
  check((regs->ctr & (CTR_LD | CTR_LS)) == CTR_LD, "CTRL[1:0] = 01b after a link disable");
  regs->ctr = CTR_LS;
  check((regs->ctr & (CTR_LD | CTR_LS)) == CTR_LS, "CTRL[1:0] = 10b after a link start");
  regs->ctr = CTR_LD;
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_LD) == 0, "back to the reset value after CTRL.RS");
}
