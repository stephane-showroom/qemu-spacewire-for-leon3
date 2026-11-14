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
 * REQ_GRSPW_040: STS reserved bits
 *
 * Bits 31:24, 20:10 and 5.
 *
 * @tc     TC_GRSPW_059
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = STR_RESERVED;
  const uint32_t value = 0U;

  check((regs->str & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->str & mask) == value, "value after a reset");
  setup();
  regs->str = 0 | mask;
  check((regs->str & mask) == value, "unchanged by a write of ones");
  regs->str = 0;
  check((regs->str & mask) == value, "unchanged by a write of zeros");
}
