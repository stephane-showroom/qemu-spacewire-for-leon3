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
 * REQ_GRSPW_119: TX descriptor IE
 *
 * @tc     TC_GRSPW_064
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t descriptor_ie[] = { TXD_IE, TXD_IE, 0 };
  static const uint32_t channel_ti[] = { CHN_TI, 0, CHN_TI };
  int base, i;

  setup();
  for (i = 0; i < 3; ++i) {
    uint8_t value = (uint8_t) (0x81 + i);

    base = irq_count[CORE];
    enable_rx(i, rx_buffer[i], 0);
    set_tx_bytes(i, NODE_ADDRESS, &value, 1, descriptor_ie[i]);
    write_chn(CHN_RE | CHN_RD | CHN_TE | channel_ti[i]);
    wait_tx(i);
    wait_rx(i);
    rtems_task_wake_after(2);
    check(irq_count[CORE] == base + (i == 0),
          i == 0 ? "interrupt with IE and TI" : "no interrupt without both IE and TI");
    check((tx_table[i].ctrl & TXD_IE) == descriptor_ie[i], "IE kept when the core writes word 0");
  }
}
