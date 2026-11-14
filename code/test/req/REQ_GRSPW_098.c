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
 * REQ_GRSPW_098: RX descriptor IE
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  int base = irq_count[CORE];

  setup();
  enable_rx(0, rx_buffer[0], RXD_IE);
  set_tx_bytes(0, NODE_ADDRESS, "\x80", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_RI | CHN_TE);
  wait_rx(0);
  rtems_task_wake_after(2);
  check(irq_count[CORE] == base + 1, "interrupt with IE and RI");
  check((rx_table[0].ctrl & RXD_IE) != 0, "IE kept when the core writes word 0");

  base = irq_count[CORE];
  enable_rx(1, rx_buffer[1], 0);
  set_tx_bytes(1, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_RI | CHN_TE);
  wait_rx(1);
  rtems_task_wake_after(2);
  check(irq_count[CORE] == base, "no interrupt without IE, even with RI");
  check((rx_table[1].ctrl & RXD_IE) == 0, "IE still 0");
}
