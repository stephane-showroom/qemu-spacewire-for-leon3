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
 * REQ_GRSPW_007: register offsets
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

  setup();
  regs->ndr = 0x11;
  regs->clk = 0x1234;
  regs->tim = 0x25;
  regs->tdr = 0x12345;
  regs->rxl = 0x100;
  regs->txd = (uint32_t) tx_table | 0x20;
  regs->rxd = (uint32_t) rx_table | 0x18;
  check(word[0x08 / 4] == 0x11, "NODEADDR at 0x08");
  check(word[0x0C / 4] == 0x1234, "CLKDIV at 0x0C");
  check(word[0x14 / 4] == 0x25, "TIME at 0x14");
  check(word[0x18 / 4] == 0x12345, "TDR at 0x18");
  check(word[0x24 / 4] == 0x100, "DMAMAXLEN at 0x24");
  check(word[0x28 / 4] == ((uint32_t) tx_table | 0x20), "DMATXDESC at 0x28");
  check(word[0x2C / 4] == ((uint32_t) rx_table | 0x18), "DMARXDESC at 0x2C");
  check(STR_LINK(word[0x04 / 4]) == LINK_RUN, "STS at 0x04");
  check((word[0x00 / 4] & CTR_LS) != 0, "CTRL at 0x00");
  if (IS_RMAP_CORE) {
    regs->key = 0x5A;
    check(word[0x10 / 4] == 0x5A, "DKEY at 0x10");
  }
}
