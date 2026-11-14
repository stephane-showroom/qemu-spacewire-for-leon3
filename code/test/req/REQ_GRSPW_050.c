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
 * REQ_GRSPW_050: TIME.TCTRL
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0xC0U, 0x40U, 0x80U, 0x00U };
  const uint32_t mask = 0xC0U;
  size_t i;

  check((regs->tim & mask) == (0U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->tim & mask) == (0U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->tim = 0 | values[i];
    check((regs->tim & mask) == values[i], "reads back the value written");
  }
  regs->tim = 0 | 0x80U;
  regs->ctr = CTR_RS;
  check((regs->tim & mask) == (0U), "back to the reset value after CTRL.RS");
}
