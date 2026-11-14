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
 * REQ_GRSPW_105: packets for the RMAP target
 *
 * @tc     TC_GRSPW_019, TC_GRSPW_020
 * @side   guest
 * @cores  2 3
 * @peer   peer_script wait ack send=420120ab send=4200000155 send=420180aa rmapw=20 send=42f0e1
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t reply[] = { 0x42, 0x01, 0x20, 0xAB };
  static const uint8_t extended[] = { 0x42, 0x00, 0x00, 0x01, 0x55 };
  static const uint8_t reserved[] = { 0x42, 0x01, 0x80, 0xAA };
  static const uint8_t marker[] = { NODE_ADDRESS, PROTOCOL_ID, 0xE1 };
  int i;

  setup();
  regs->ctr = CTR_LS | CTR_RE | CTR_PM;
  regs->key = DESTINATION_KEY;
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0);
  }
  set_tx_area_address(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(3);
  check(rx_is(0, reply, sizeof(reply)), "RMAP reply stored in the DMA channel");
  check(rx_is(1, extended, sizeof(extended)), "extended protocol identifier stored");
  check(rx_is(2, reserved, sizeof(reserved)), "packet type 10b stored");
  check(rx_is(3, marker, sizeof(marker)), "RMAP command not stored");
  check(area_written(), "RMAP command executed, also in promiscuous mode");
}
