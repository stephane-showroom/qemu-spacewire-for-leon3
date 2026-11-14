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
 * REQ_GRSPW_074: DMAMAXLEN.RXMAXLEN
 *
 * @tc     TC_GRSPW_062
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  const uint32_t values[] = { 4U, 1024U, 0x01FFFFFCU, 8U };
  const uint32_t mask = 0x01FFFFFCU;
  size_t i;

  check((regs->rxl & mask) == 0, "NR field: 0 at power-on");
  setup();
  for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    regs->rxl = 0 | values[i];
    check((regs->rxl & mask) == values[i], "reads back the value written");
  }
  regs->rxl = 0 | 64U;
  regs->ctr = CTR_RS;
  check((regs->rxl & mask) == (64U), "NR field: kept through CTRL.RS");
}
