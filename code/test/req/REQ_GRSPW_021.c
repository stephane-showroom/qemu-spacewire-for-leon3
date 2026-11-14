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
 * REQ_GRSPW_021: CTRL.TQ
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { CTR_TQ, 0U };
  const uint32_t mask = CTR_TQ;
  size_t i;

  check((regs->ctr & mask) == 0, "NR field: 0 at power-on");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->ctr = CTR_LS | values[i];
    check((regs->ctr & mask) == values[i], "reads back the value written");
  }
  regs->ctr = CTR_LS | CTR_TQ;
  regs->ctr = CTR_RS | CTR_TQ;
  check((regs->ctr & mask) == (CTR_TQ), "NR field: kept through CTRL.RS");
}
