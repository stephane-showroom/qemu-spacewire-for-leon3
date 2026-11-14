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
 * REQ_GRSPW_008: unused offsets read 0 and ignore writes
 *
 * @tc     TC_GRSPW_005
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  volatile uint32_t *const word = (volatile uint32_t *) CORE_BASE;
  uint32_t before[12], offset;
  int i;

  setup();
  regs->tim = 0x33;
  for (i = 0; i < 12; ++i) {
    before[i] = word[i];
  }
  for (offset = 0x1C; offset < 0x100; offset += 4) {
    if (offset == 0x1C || offset >= 0x30) {
      word[offset / 4] = 0xA5A5A5A5U;
    }
  }
  for (offset = 0x1C; offset < 0x100; offset += 4) {
    if (offset == 0x1C || offset >= 0x30) {
      check(word[offset / 4] == 0, "unused offset reads 0");
    }
  }
  for (i = 0; i < 12; ++i) {
    check(i == 7 || word[i] == before[i], "registers unchanged by writes to unused offsets");
  }
}
