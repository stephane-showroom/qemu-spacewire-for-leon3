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
 * REQ_GRSPW_154: DMA event interrupts
 *
 * The AHB error interrupt is checked with REQ_GRSPW_114 and REQ_GRSPW_137.
 *
 * @tc     TC_GRSPW_046, TC_GRSPW_047
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t descriptor_ie[] = { TXD_IE, TXD_IE, 0 };
  static const uint32_t channel_ti[] = { CHN_TI, 0, CHN_TI };
  int base = irq_count[CORE];
  int i;

  setup();
  enable_rx(0, rx_buffer[0], RXD_IE);
  set_tx_bytes(0, NODE_ADDRESS, "\x80", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_RI | CHN_TE);
  wait_rx(0);
  rtems_task_wake_after(2);
  check(irq_count[CORE] == base + 1, "receive interrupt with descriptor IE and RI, CTRL.IE = 0");

  for (i = 0; i < 3; ++i) {
    uint8_t value = (uint8_t) (0x81 + i);

    base = irq_count[CORE];
    enable_rx(i + 1, rx_buffer[i + 1], 0);
    set_tx_bytes(i + 1, NODE_ADDRESS, &value, 1, descriptor_ie[i]);
    write_chn(CHN_RE | CHN_RD | CHN_TE | channel_ti[i]);
    wait_tx(i + 1);
    wait_rx(i + 1);
    rtems_task_wake_after(2);
    check(irq_count[CORE] == base + (i == 0),
          i == 0 ? "transmit interrupt with descriptor IE and TI"
                 : "no transmit interrupt without both IE and TI");
  }
}
