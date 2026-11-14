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
 * REQ_GRSPW_096: RX descriptor HC
 *
 * The peer sends a valid read reply, the same with a bad header CRC, the same
 * with a bad data CRC, and a non-RMAP packet ended by its CRC.
 *
 * @tc     TC_GRSPW_063
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack send=42010c00670001000000045c11223344ca send=42010c0067000100000004a311223344ca send=42010c00670001000000045c1122334435 send=42f01020304050607080b3
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t expected[] = { 0U, RXD_HC, 0U, 0U };
  int i;

  setup();
  regs->ctr = CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0);
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], RXD_HC | RXD_DC);
  }
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(3);
  for (i = 0; i < 4; ++i) {
    check((rx_table[i].ctrl & RXD_HC) == (IS_RMAP_CORE ? expected[i] : 0U),
          IS_RMAP_CORE ? "HC written from the CRC check, whatever the guest wrote"
                       : "HC always 0 without RMAP CRC, whatever the guest wrote");
  }
}
