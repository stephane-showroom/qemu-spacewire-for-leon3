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
 * REQ_GRSPW_017: CTRL.RE
 *
 * @tc     TC_GRSPW_058
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t reset = IS_RMAP_CORE ? CTR_RE : 0U;

  check((regs->ctr & CTR_RE) == reset, "power-on value: rmapen on the RMAP cores");
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_RE) == reset, "reset value: rmapen on the RMAP cores");
  setup();
  regs->ctr = CTR_LS | CTR_RE;
  check((regs->ctr & CTR_RE) == reset, "reads back 1 on the RMAP cores, stays 0 on the others");
  regs->ctr = CTR_LS;
  check((regs->ctr & CTR_RE) == 0, "reads back 0");
  regs->ctr = CTR_RS;
  check((regs->ctr & CTR_RE) == reset, "back to the reset value after CTRL.RS");
}
