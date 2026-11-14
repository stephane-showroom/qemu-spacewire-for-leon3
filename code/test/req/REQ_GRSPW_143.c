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
 * REQ_GRSPW_143: accesses of the verified writes and read-modify-writes
 *
 * @tc     TC_GRSPW_039, TC_GRSPW_040
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t reference[] = { 0x10, 0x20, 0x30, 0x40 };
  static const uint8_t one_byte[] = { 0xAB, 0xFF };
  static const uint8_t two_bytes[] = { 0x12, 0x34, 0xFF, 0xFF };
  static const uint8_t old_one[] = { 0x20 };
  static const uint8_t old_two[] = { 0x30, 0x40 };
  static const uint8_t after[] = { 0x10, 0xAB, 0x12, 0x34 };
  static const uint8_t half[] = { 0xCA, 0xFE };
  uint8_t packet[64];
  size_t header;

  write_memory("setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 40, reference, sizeof(reference), STATUS_OK);
  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 41, sizeof(one_byte));
  exchange("rmw of 1 byte at an odd address", packet, add_data(packet, header, one_byte, sizeof(one_byte)),
           0, STATUS_OK, true, old_one, sizeof(old_one));
  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 43, sizeof(two_bytes));
  exchange("rmw of 2 bytes, unaligned: error 10", packet, add_data(packet, header, two_bytes, sizeof(two_bytes)),
           0, STATUS_NOT_AUTHORISED, true, NO_DATA, 0);
  header = build_header(packet, INS_RMW, KEY_GUEST, NULL, target + 42, sizeof(two_bytes));
  exchange("rmw of 2 bytes, halfword aligned", packet, add_data(packet, header, two_bytes, sizeof(two_bytes)),
           0, STATUS_OK, true, old_two, sizeof(old_two));
  read_memory("rmw results", INS_READ_INC, target + 40, after, sizeof(after), STATUS_OK);
  write_memory("verified write of 3 bytes: error 10", INS_VERIFY_ACK_INC, KEY_GUEST, target,
               reference, 3, STATUS_NOT_AUTHORISED);
  write_memory("verified write of 2 bytes, unaligned: error 10", INS_VERIFY_ACK_INC, KEY_GUEST,
               target + 45, half, sizeof(half), STATUS_NOT_AUTHORISED);
  write_memory("verified write of 2 bytes, aligned", INS_VERIFY_ACK_INC, KEY_GUEST, target + 46,
               half, sizeof(half), STATUS_OK);
  read_memory("verified write readback", INS_READ_INC, target + 46, half, sizeof(half), STATUS_OK);
}
