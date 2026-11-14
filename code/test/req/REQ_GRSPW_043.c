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
 * REQ_GRSPW_043: NODEADDR.NODEADDR
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0x00U, 0xFFU, 0x42U, 0xA5U };
  const uint32_t mask = 0xFFU;
  size_t i;

  check((regs->ndr & mask) == (254U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->ndr & mask) == (254U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->ndr = 0 | values[i];
    check((regs->ndr & mask) == values[i], "reads back the value written");
  }
  regs->ndr = 0 | 0x11U;
  regs->ctr = CTR_RS;
  check((regs->ndr & mask) == (254U), "back to the reset value after CTRL.RS");
}
