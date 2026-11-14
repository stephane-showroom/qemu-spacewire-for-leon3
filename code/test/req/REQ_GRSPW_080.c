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
 * REQ_GRSPW_080: DMARXDESC.DESCSEL
 *
 * @tc     TC_GRSPW_062, TC_GRSPW_010
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t values[] = { 0x3F8U, 0x2A8U, 0x008U, 0U };
  size_t i;
  int n;

  check((regs->rxd & 0x3F8U) == 0, "power-on value");
  setup();
  for (i = 0; i < 4; ++i) {
    regs->rxd = (uint32_t) rx_table | values[i];
    check((regs->rxd & 0x3F8U) == values[i], "reads back the value written");
  }
  regs->ctr = CTR_RS;
  check((regs->rxd & 0x3F8U) == 0, "back to the reset value after CTRL.RS");

  setup();
  regs->rxl = SMALL_SIZE;
  for (n = 0; n < 128; ++n) {
    enable_rx(n, small_rx[n], 0);
  }
  for (n = 0; n < 128 + 2; ++n) {
    uint8_t value = (uint8_t) (0x80 | (n & 0x7F));
    int rx = n % 128;
    int tx = n % 64;

    set_tx_bytes(tx, NODE_ADDRESS, &value, 1, 0);
    write_chn(CHN_RE | CHN_RD | CHN_TE);
    wait_tx(tx);
    WAIT_FOR((rx_table[rx].ctrl & RXD_EN) == 0, "packet not received");
    check((regs->rxd & 0x3F8U) == (uint32_t) ((n + 1) % 128) << 3,
          "selector advances by one descriptor and wraps after 128");
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
    check((regs->rxd & 0x3F8U) == (uint32_t) ((n + 1) % 4) << 3,
          "selector returns to 0 after a descriptor with WR");
    enable_rx(rx, small_rx[rx], rx == 3 ? RXD_WR : 0);
  }

  setup();
  regs->rxl = SMALL_SIZE;
  enable_rx(5, small_rx[5], 0);
  regs->rxd = (uint32_t) rx_table | (5U << 3);
  set_tx_bytes(0, NODE_ADDRESS, "\xD5", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((rx_table[5].ctrl & RXD_EN) == 0, "packet not received in descriptor 5");
  check(small_rx[5][2] == 0xD5, "reception starts at the selector written");
  check((regs->rxd & 0x3F8U) == 6U << 3, "selector advances from the value written");
}
