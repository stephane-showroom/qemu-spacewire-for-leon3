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
 * REQ_GRSPW_077: DMATXDESC.DESCSEL
 *
 * @tc     TC_GRSPW_062, TC_GRSPW_010
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t values[] = { 0x3F0U, 0x150U, 0x010U, 0U };
  size_t i;
  int n;

  check((regs->txd & 0x3F0U) == 0, "power-on value");
  setup();
  for (i = 0; i < 4; ++i) {
    regs->txd = (uint32_t) tx_table | values[i];
    check((regs->txd & 0x3F0U) == values[i], "reads back the value written");
  }
  regs->ctr = CTR_RS;
  check((regs->txd & 0x3F0U) == 0, "back to the reset value after CTRL.RS");

  setup();
  regs->rxl = SMALL_SIZE;
  for (n = 0; n < 128; ++n) {
    enable_rx(n, small_rx[n], 0);
  }
  for (n = 0; n < 64 + 2; ++n) {
    uint8_t value = (uint8_t) (0x80 | (n & 0x7F));
    int rx = n % 128;
    int tx = n % 64;

    set_tx_bytes(tx, NODE_ADDRESS, &value, 1, 0);
    write_chn(CHN_RE | CHN_RD | CHN_TE);
    wait_tx(tx);
    WAIT_FOR((rx_table[rx].ctrl & RXD_EN) == 0, "packet not received");
    check((regs->txd & 0x3F0U) == (uint32_t) ((n + 1) % 64) << 4,
          "selector advances by one descriptor and wraps after 64");
    enable_rx(rx, small_rx[rx], 0);
  }

  setup();
  regs->rxl = SMALL_SIZE;
  for (n = 0; n < 4; ++n) {
    enable_rx(n, small_rx[n], n == 3 ? RXD_WR : 0);
  }
  for (n = 0; n < 5; ++n) {
    uint8_t value = (uint8_t) (0xC0 | n);
    int rx = n % 4;
    int tx = n % 2;

    set_tx_bytes(tx, NODE_ADDRESS, &value, 1, tx == 1 ? TXD_WR : 0);
    write_chn(CHN_RE | CHN_RD | CHN_TE);
    wait_tx(tx);
    WAIT_FOR((rx_table[rx].ctrl & RXD_EN) == 0, "packet not received");
    check((regs->txd & 0x3F0U) == (uint32_t) ((n + 1) % 2) << 4,
          "selector returns to 0 after a descriptor with WR");
    enable_rx(rx, small_rx[rx], rx == 3 ? RXD_WR : 0);
  }

  setup();
  regs->rxl = SMALL_SIZE;
  trigger(0);
  set_tx_bytes(3, NODE_ADDRESS, "\xD5", 1, 0);
  enable_rx(0, small_rx[0], 0);
  regs->txd = (uint32_t) tx_table | (3U << 4);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_tx(3);
  check((tx_table[0].ctrl & TXD_EN) != 0, "descriptor 0 skipped");
  check(small_rx[0][2] == 0xD5, "transmission starts at the selector written");
  check((regs->txd & 0x3F0U) == 4U << 4, "selector advances from the value written");
}
