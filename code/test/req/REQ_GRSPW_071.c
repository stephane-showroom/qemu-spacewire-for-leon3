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
 * REQ_GRSPW_071: DMACTRL reserved bits
 *
 * Bits 31:17 and 15:13.
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = CHN_RESERVED;
  const uint32_t value = 0U;

  check((regs->chn & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->chn & mask) == value, "value after a reset");
  setup();
  write_chn(0 | mask);
  check((regs->chn & mask) == value, "unchanged by a write of ones");
  write_chn(0);
  check((regs->chn & mask) == value, "unchanged by a write of zeros");
}
