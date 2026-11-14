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
 * REQ_GRSPW_023: CTRL.PM
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { CTR_PM, 0U };
  const uint32_t mask = CTR_PM;
  size_t i;

  check((regs->ctr & mask) == (0U), "power-on value");
  regs->ctr = CTR_RS;
  check((regs->ctr & mask) == (0U), "reset value");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->ctr = CTR_LS | values[i];
    check((regs->ctr & mask) == values[i], "reads back the value written");
  }
  regs->ctr = CTR_LS | CTR_PM;
  regs->ctr = CTR_RS;
  check((regs->ctr & mask) == (0U), "back to the reset value after CTRL.RS");
}
