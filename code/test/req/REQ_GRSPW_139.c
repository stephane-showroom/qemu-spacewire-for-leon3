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
 * REQ_GRSPW_139: RMAP write, read and read-modify-write
 *
 * @tc     TC_GRSPW_039
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  static const uint8_t initial[] = { 0x12, 0x34, 0x56, 0x78 };
  static const uint8_t command[] = { 0xFF, 0x00, 0xAA, 0x0F, 0xF0, 0x0F, 0xFF, 0x00 };
  static const uint8_t expected[] = { 0xF2, 0x30, 0xAA, 0x78 };
  uint8_t packet[64];
  size_t header;

  write_memory("write with reply", INS_WRITE_ACK_INC, KEY_GUEST, target, data, sizeof(data), STATUS_OK);
  read_memory("read", INS_READ_INC, target, data, sizeof(data), STATUS_OK);
  write_memory("rmw setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 24, initial, sizeof(initial), STATUS_OK);
  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 24, sizeof(command));
  exchange("rmw replies the old data", packet, add_data(packet, header, command, sizeof(command)),
           0, STATUS_OK, true, initial, sizeof(initial));
  read_memory("rmw writes (data & mask) | (old & ~mask)", INS_READ_INC, target + 24, expected,
              sizeof(expected), STATUS_OK);
  header = build_header(packet, INS_WRITE_NOACK_INC, KEY_GUEST, NULL, target + 8, 4);
  expect_no_reply("write without reply", packet, add_data(packet, header, data, 4));
  read_memory("write without reply executed", INS_READ_INC, target + 8, data, 4, STATUS_OK);
}
