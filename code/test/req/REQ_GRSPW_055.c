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
 * REQ_GRSPW_055: TDR.TIMER64
 *
 * 256 at 40 MHz: ceil(40000 * 64 / 10000).
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0xFFFU, 0xA5AU, 0x000U, 0x100U };
  const uint32_t mask = 0xFFFU;
  size_t i;

  check((regs->tdr & mask) == (0x100U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->tdr & mask) == (0x100U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->tdr = 0 | values[i];
    check((regs->tdr & mask) == values[i], "reads back the value written");
  }
  regs->tdr = 0 | 0x555U;
  regs->ctr = CTR_RS;
  check((regs->tdr & mask) == (0x100U), "back to the reset value after CTRL.RS");
}
