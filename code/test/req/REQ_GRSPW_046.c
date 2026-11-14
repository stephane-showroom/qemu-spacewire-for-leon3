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
 * REQ_GRSPW_046: CLKDIV.CLKDIVRUN
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0x00FFU, 0x005AU, 0x0000U, 0x00A5U };
  const uint32_t mask = 0x00FFU;
  size_t i;

  check((regs->clk & mask) == (0x0004U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->clk & mask) == (0x0004U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->clk = 0 | values[i];
    check((regs->clk & mask) == values[i], "reads back the value written");
  }
  regs->clk = 0 | 0x0012U;
  regs->ctr = CTR_RS;
  check((regs->clk & mask) == (0x0004U), "back to the reset value after CTRL.RS");
}
