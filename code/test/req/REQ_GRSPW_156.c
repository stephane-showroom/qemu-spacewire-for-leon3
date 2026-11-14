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
 * REQ_GRSPW_156: power-on values
 *
 * The test makes no access to the core before these reads.
 *
 * @tc     TC_GRSPW_003
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  uint32_t expected_ctr = IS_RMAP_CORE ? (CTR_CAPABILITIES | CTR_RE) : 0;

  check((regs->ctr & ~CTR_AS) == expected_ctr, "CTRL power-on value, other than AS");
  check((regs->ctr & CTR_AS) == (IS_RMAP_CORE ? CTR_AS : 0U), "CTRL.AS power-on value");
  check(regs->str == 0, "STS power-on value");
  check(regs->ndr == 254, "NODEADDR power-on value");
  check(regs->clk == 0x0404, "CLKDIV power-on value");
  check(regs->key == 0, "DKEY power-on value");
  check(regs->tim == 0, "TIME power-on value");
  check(regs->tdr == 0x0001F100, "TDR power-on value");
  check(regs->chn == 0, "DMACTRL power-on value, NR fields at 0");
  check(regs->rxl == 0, "DMAMAXLEN power-on value");
  check(regs->txd == 0 && regs->rxd == 0, "descriptor table registers power-on value");
}
