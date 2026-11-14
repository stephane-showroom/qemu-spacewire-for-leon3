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
 * REQ_GRSPW_113: EEP flag
 *
 * Blocked until the queue format can carry an EEP (TEST_PLAN.md section 8).
 *
 * @tc     TC_GRSPW_029
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 * @known  all: blocked: the queue format has no EEP marker (TEST_PLAN.md section 8)
 */

#include "req_guest.h"

static void test(void)
{
  fail("blocked: the queue format has no EEP marker (TEST_PLAN.md, section 8)");
}
