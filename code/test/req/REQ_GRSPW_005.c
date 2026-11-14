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
 * REQ_GRSPW_005: values of the input signals
 *
 * @tc     TC_GRSPW_004
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_RE) == (IS_RMAP_CORE ? CTR_RE : 0U), "CTRL.RE reset value is rmapen");
  check((regs->ctr & CTR_AS) == (IS_RMAP_CORE ? CTR_AS : 0U), "CTRL.AS reset value is rmapen");
  check(regs->clk == 0x0404, "CLKDIV reset value from clkdiv10");
  check(regs->ndr == 254, "NODEADDR reset value from the nodeaddr generic");
  check(regs->key == 0, "DKEY reset value from the destkey generic");
  check(regs->tdr == 0x0001F100, "TDR reset value from the system frequency");
}
