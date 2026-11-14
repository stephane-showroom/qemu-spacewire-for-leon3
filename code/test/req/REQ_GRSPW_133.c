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
 * REQ_GRSPW_133: transmit descriptor completion
 *
 * @tc     TC_GRSPW_031, TC_GRSPW_047
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
  enable_rx(1, rx_buffer[1], 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, TXD_IE);
  set_tx_bytes(1, NODE_ADDRESS, "\x82", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE | CHN_TI);
  wait_tx(1);
  rtems_task_wake_after(2);
  check((tx_table[0].ctrl & (TXD_EN | TXD_LE)) == 0, "EN cleared, LE clear");
  check((tx_table[0].ctrl & (TXD_IE | 0xFFU)) == TXD_IE, "other fields of word 0 kept");
  check((regs->txd & 0x3F0U) == 2U << 4, "selector advanced by one per packet");
  check((regs->chn & CHN_PS) != 0, "PS set");
  check(irq_count[CORE] == base + 1, "interrupt for the descriptor with IE only");
}
