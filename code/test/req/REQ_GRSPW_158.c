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
 * REQ_GRSPW_158: link state after a reset
 *
 * @tc     TC_GRSPW_004, TC_GRSPW_011
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  regs->ctr = CTR_LS;
  check(STR_LINK(regs->str) == LINK_RUN, "run before the reset");
  regs->ctr = CTR_RS;
  check(STR_LINK(regs->str) == 0, "error-reset after the reset");
  regs->ctr = CTR_LS;
  check(STR_LINK(regs->str) == LINK_RUN, "LS starts the link again");
}
