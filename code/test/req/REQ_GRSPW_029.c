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
 * REQ_GRSPW_029: CTRL reserved bits
 *
 * Bits 28:27, 25:22, 19:18, 15:12 and 7.
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = 0x1BCCF080U;
  const uint32_t value = 0U;

  check((regs->ctr & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->ctr & mask) == value, "value after a reset");
  setup();
  regs->ctr = CTR_LS | mask;
  check((regs->ctr & mask) == value, "unchanged by a write of ones");
  regs->ctr = CTR_LS;
  check((regs->ctr & mask) == value, "unchanged by a write of zeros");
}
