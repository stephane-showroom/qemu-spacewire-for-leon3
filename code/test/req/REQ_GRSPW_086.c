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
 * REQ_GRSPW_086: character-level error bits
 *
 * Blocked until the fault injection of TEST_PLAN.md section 8 exists.
 *
 * @tc     TC_GRSPW_013
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 * @known  all: blocked: no fault injection of character-level errors (TEST_PLAN.md section 8)
 */

#include "req_guest.h"

static void test(void)
{
  fail("blocked: no way to inject a character-level error (TEST_PLAN.md, section 8)");
}
