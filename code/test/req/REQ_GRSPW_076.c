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
 * REQ_GRSPW_076: DMATXDESC.DESCBASEADDR
 *
 * @tc     TC_GRSPW_062
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 0xFFFFFC00U, 0x40000400U, (uint32_t) tx_table };
  const uint32_t mask = 0xFFFFFC00U;
  size_t i;

  check((regs->txd & mask) == 0, "NR field: 0 at power-on");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->txd = 0 | values[i];
    check((regs->txd & mask) == values[i], "reads back the value written");
  }
  regs->txd = 0 | (uint32_t) tx_table;
  regs->ctr = CTR_RS;
  check((regs->txd & mask) == ((uint32_t) tx_table), "NR field: kept through CTRL.RS");
}
