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
 * REQ_GRSPW_075: DMAMAXLEN reserved bits
 *
 * Bits 31:25 and 1:0.
 *
 * @tc     TC_GRSPW_062
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = 0xFE000003U;
  const uint32_t value = 0U;

  check((regs->rxl & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->rxl & mask) == value, "value after a reset");
  setup();
  regs->rxl = 1024U | mask;
  check((regs->rxl & mask) == value, "unchanged by a write of ones");
  regs->rxl = 1024U;
  check((regs->rxl & mask) == value, "unchanged by a write of zeros");
}
