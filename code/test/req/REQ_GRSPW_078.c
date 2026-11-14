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
 * REQ_GRSPW_078: DMATXDESC reserved bits
 *
 * @tc     TC_GRSPW_062
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t mask = 0xFU;
  const uint32_t value = 0U;

  check((regs->txd & mask) == value, "power-on value");
  regs->ctr = CTR_RS;
  check((regs->txd & mask) == value, "value after a reset");
  setup();
  regs->txd = (uint32_t) tx_table | mask;
  check((regs->txd & mask) == value, "unchanged by a write of ones");
  regs->txd = (uint32_t) tx_table;
  check((regs->txd & mask) == value, "unchanged by a write of zeros");
}
