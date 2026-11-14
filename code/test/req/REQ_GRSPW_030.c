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
 * REQ_GRSPW_030: STS.LS
 *
 * @tc     TC_GRSPW_059
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  regs->ctr = CTR_LD;
  check(STR_LINK(regs->str) == 0, "error-reset with LD");
  regs->str = 7U << 21;
  check(STR_LINK(regs->str) == 0, "unchanged by a write of ones");
  regs->ctr = CTR_LS;
  check(STR_LINK(regs->str) == LINK_RUN, "run after LS");
  regs->str = 0;
  check(STR_LINK(regs->str) == LINK_RUN, "unchanged by a write of zeros");
}
