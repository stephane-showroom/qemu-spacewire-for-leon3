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
 * REQ_GRSPW_056: TDR reserved bits
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = 0xFFC00000U;
  const uint32_t value = 0U;

  check((regs->tdr & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->tdr & mask) == value, "value after a reset");
  setup();
  regs->tdr = 0 | mask;
  check((regs->tdr & mask) == value, "unchanged by a write of ones");
  regs->tdr = 0;
  check((regs->tdr & mask) == value, "unchanged by a write of zeros");
}
