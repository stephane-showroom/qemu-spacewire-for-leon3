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
 * REQ_GRSPW_084: immediate link start
 *
 * @tc     TC_GRSPW_011
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  regs->ctr = CTR_RS;
  regs->ctr = CTR_LS;
  check(STR_LINK(regs->str) == LINK_RUN, "run after LS");
  regs->ctr = CTR_LD;
  regs->ctr = 0;
  check(STR_LINK(regs->str) == 0, "unchanged when LD, LS and AS are 0 (error-reset)");
  regs->ctr = CTR_AS;
  check(STR_LINK(regs->str) == LINK_RUN, "run after AS");
  regs->ctr = 0;
  check(STR_LINK(regs->str) == LINK_RUN, "unchanged when LD, LS and AS are 0 (run)");
}
