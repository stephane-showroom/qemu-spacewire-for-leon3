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
 * REQ_GRSPW_083: link state machine controlled by CTRL
 *
 * @tc     TC_GRSPW_011
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  regs->ctr = CTR_LD;
  check(STR_LINK(regs->str) == 0, "error-reset after LD");
  regs->ctr = CTR_LS;
  regs->ctr = CTR_LD | CTR_LS;
  check(STR_LINK(regs->str) == 0, "LD wins over LS");
  regs->ctr = CTR_LD | CTR_AS;
  check(STR_LINK(regs->str) == 0, "LD wins over AS");
}
