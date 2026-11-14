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
 * REQ_GRSPW_142: RMAP error priority
 *
 * @tc     TC_GRSPW_039, TC_GRSPW_041
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t data[] = { 1, 2, 3, 4, 5, 6 };
  uint8_t packet[64];
  size_t header;
  size_t size;

  header = build_header(packet, INS_UNUSED_REPLY, KEY_WRONG, NULL, target, 4);
  exchange("unused code (2) before invalid key (3)", packet, header, 0, STATUS_UNUSED, true, NO_DATA, 0);
  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_WRONG, NULL, target, 5);
  exchange("invalid key (3) before verify overrun (9)", packet, add_data(packet, header, data, 5), 0,
           STATUS_KEY, false, NO_DATA, 0);
  header = build_header(packet, INS_READ_SINGLE, KEY_WRONG, NULL, target + 1, 4);
  exchange("invalid key (3) before authorisation failure (10)", packet, header, 0, STATUS_KEY, true, NO_DATA, 0);
  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 1, 6);
  exchange("rmw length error (11) before authorisation failure (10)", packet,
           add_data(packet, header, data, 6), 0, STATUS_RMW_LENGTH, true, NO_DATA, 0);
  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target, 3);
  size = add_data(packet, header, data, 3);
  packet[size - 1] ^= 0xFF;
  exchange("authorisation failure (10) before invalid data CRC (4)", packet, size, 0,
           STATUS_NOT_AUTHORISED, false, NO_DATA, 0);
  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target, 4);
  size = add_data(packet, header, data, 4);
  packet[size - 3] ^= 0xFF;
  exchange("early EOP (5) before invalid data CRC (4)", packet, size - 2, 0, STATUS_EARLY_EOP,
           false, NO_DATA, 0);
  header = build_header(packet, INS_VERIFY_ACK_INC, KEY_GUEST, NULL, target, 4);
  size = add_data(packet, header, data, 4);
  packet[size] = 0;
  exchange("too much data (6) last", packet, size + 1, 0, STATUS_TOO_MUCH_DATA, false, NO_DATA, 0);
}
