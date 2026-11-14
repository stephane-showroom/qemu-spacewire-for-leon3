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
 * REQ_GRSPW_100: RX descriptor EN
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  memset(rx_buffer[0], 0, 4);
  rx_table[0].addr = (uint32_t) rx_buffer[0];
  rx_table[0].ctrl = 0;
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(0);
  WAIT_FOR((regs->chn & CHN_RD) == 0, "RD not cleared by a disabled descriptor");
  check(rx_buffer[0][2] == 0, "nothing stored in a disabled descriptor");
  check(rx_table[0].ctrl == 0, "disabled descriptor not written");
  check((regs->rxd & 0x3F8U) == 0, "selector not advanced");

  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(1, NODE_ADDRESS, "\x82", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(rx_buffer[0][2] == 0x82, "packet stored in the enabled descriptor");
  check((rx_table[0].ctrl & RXD_EN) == 0, "EN cleared when the packet ends");
}
