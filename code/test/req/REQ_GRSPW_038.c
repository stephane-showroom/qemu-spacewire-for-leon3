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
 * REQ_GRSPW_038: STS.CE
 *
 * Setting the bit needs the fault injection of TEST_PLAN.md section 8: it is
 * checked by REQ_GRSPW_086 (blocked).
 *
 * @tc     TC_GRSPW_059
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t bit = 1U << 1;

  check((regs->str & bit) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->str & bit) == 0, "reset value");
  setup();
  regs->str = bit;
  check((regs->str & bit) == 0, "a write of 1 does not set it");
  regs->ctr = CTR_LD;
  regs->ctr = CTR_LS;
  check((regs->str & bit) == 0, "not set by a link restart");
}
