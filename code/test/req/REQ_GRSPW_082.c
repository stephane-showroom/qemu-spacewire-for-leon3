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
 * REQ_GRSPW_082: descriptor table bases written by the guest
 *
 * @tc     TC_GRSPW_010
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait sleep=500 ack
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  check(regs->rxd == (uint32_t) rx_table && regs->txd == (uint32_t) tx_table,
        "table bases read back with the selector at 0");
  trigger(0);
  write_chn(CHN_TE);
  regs->txd = (uint32_t) tx_table;
  regs->rxd = (uint32_t) rx_table;
  check(regs->txd == (uint32_t) tx_table, "a write while the channel is active does not crash");
  wait_tx(0);
}
