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
 * REQ_GRSPW_137: memory error on transmission
 *
 * @tc     TC_GRSPW_037
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  int base = irq_count[CORE];

  setup();
  enable_rx(0, rx_buffer[0], 0);
  set_tx(0, (const void *) UNMAPPED_ADDRESS, 4, 0);
  write_chn(CHN_AI | CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((regs->chn & CHN_TA) != 0, "TA not set by a memory error on the data");
  check((regs->chn & CHN_TE) == 0, "TE cleared");
  check((tx_table[0].ctrl & (TXD_EN | TXD_LE)) == TXD_EN, "descriptor stays enabled, no LE");
  rtems_task_wake_after(2);
  check(irq_count[CORE] == base + 1, "AHB error interrupt with AI");

  write_chn(CHN_TA);
  regs->txd = UNMAPPED_ADDRESS;
  write_chn(CHN_AI | CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((regs->chn & CHN_TA) != 0, "TA not set by an unmapped descriptor table");
}
