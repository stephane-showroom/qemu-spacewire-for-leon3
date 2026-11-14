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
 * REQ_GRSPW_054: TDR.DISCONNECT
 *
 * 31 at 40 MHz: ceil(40000 * 85 / 100000) - 3.
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0x3FF000U, 0x2A5000U, 0x000000U, 0x01F000U };
  const uint32_t mask = 0x3FF000U;
  size_t i;

  check((regs->tdr & mask) == (0x01F000U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->tdr & mask) == (0x01F000U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->tdr = 0 | values[i];
    check((regs->tdr & mask) == values[i], "reads back the value written");
  }
  regs->tdr = 0 | 0x155000U;
  regs->ctr = CTR_RS;
  check((regs->tdr & mask) == (0x01F000U), "back to the reset value after CTRL.RS");
}
