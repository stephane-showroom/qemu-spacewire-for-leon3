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
 * REQ_GRSPW_048: DKEY.DESTKEY
 *
 * @tc     TC_GRSPW_060
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t values[] = { 0xFFU, 0x5AU, 0x00U, 0xA5U };
  size_t i;

  check((regs->key & 0xFFU) == 0, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->key & 0xFFU) == 0, "reset value");
  setup();
  for (i = 0; i < 4; ++i) {
    regs->key = values[i];
    check((regs->key & 0xFFU) == (IS_RMAP_CORE ? values[i] : 0U),
          "reads back the key on the RMAP cores, 0 on the others");
  }
  regs->key = 0x11;
  regs->ctr = CTR_RS;
  check((regs->key & 0xFFU) == 0, "back to the reset value after CTRL.RS");
}
