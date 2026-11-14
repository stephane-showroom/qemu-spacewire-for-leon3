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
 * REQ_GRSPW_006: big-endian byte order
 *
 * @tc     TC_GRSPW_017
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t bytes[] = { 0x81, 0x92 };
  const volatile uint32_t *word = (const volatile uint32_t *) rx_buffer[0];

  setup();
  enable_rx(0, rx_buffer[0], 0);
  set_tx_bytes(0, NODE_ADDRESS, bytes, sizeof(bytes), 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(*word == 0x42F08192U, "first byte of the packet is the most significant byte of the word");
}
