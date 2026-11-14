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
 * REQ_GRSPW_016: CTRL.RD
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->ctr & CTR_RD) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_RD) == 0, "reset value");
  setup();
  regs->ctr = CTR_LS | CTR_RD;
  check((regs->ctr & CTR_RD) == (IS_RMAP_CORE ? CTR_RD : 0U),
        "reads back 1 on the RMAP cores, stays 0 on the others");
  regs->ctr = CTR_LS;
  check((regs->ctr & CTR_RD) == 0, "reads back 0");
  regs->ctr = CTR_LS | CTR_RD;
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_RD) == 0, "back to the reset value after CTRL.RS");
}
