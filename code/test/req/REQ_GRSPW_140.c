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
 * REQ_GRSPW_140: RMAP command table
 *
 * @tc     TC_GRSPW_039, TC_GRSPW_040
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t reference[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
  static const uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  static const uint8_t five[] = { 1, 2, 3, 4, 5 };
  static const uint8_t no_reply_codes[] = { 0x40, 0x44, 0x50, 0x54 };
  uint8_t packet[64];
  size_t header;
  size_t i;

  write_memory("setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 40, reference, sizeof(reference), STATUS_OK);
  for (i = 0; i < sizeof(no_reply_codes); ++i) {
    header = build_header(packet, no_reply_codes[i], KEY_GUEST, NULL, target + 40, 4);
    expect_no_reply("unused command code without acknowledge: no reply", packet, header);
  }
  header = build_header(packet, INS_UNUSED_REPLY, KEY_GUEST, NULL, target, 4);
  exchange("verify, acknowledge, single address read: error 2", packet, header, 0, STATUS_UNUSED,
           true, NO_DATA, 0);
  header = build_header(packet, INS_READ_SINGLE, KEY_GUEST, NULL, target + 1, 4);
  exchange("single address read, unaligned: error 10", packet, header, 0, STATUS_NOT_AUTHORISED,
           true, NO_DATA, 0);
  header = build_header(packet, INS_VERIFY_NOACK_INC, KEY_GUEST, NULL, target + 40, sizeof(five));
  expect_no_reply("verified write of 5 bytes without reply", packet, add_data(packet, header, five, sizeof(five)));
  header = build_header(packet, INS_WRITE_NOACK_SINGLE, KEY_GUEST, NULL, target + 41, 4);
  expect_no_reply("single address write without reply, unaligned", packet, add_data(packet, header, data, 4));
  read_memory("nothing written by the refused commands", INS_READ_INC, target + 40, reference,
              sizeof(reference), STATUS_OK);
  write_memory("single address write, unaligned: error 10", INS_WRITE_ACK_SINGLE, KEY_GUEST,
               target + 41, data, 4, STATUS_NOT_AUTHORISED);
  write_memory("verified write of 5 bytes: error 9", INS_VERIFY_ACK_INC, KEY_GUEST, target,
               five, sizeof(five), STATUS_VERIFY_OVERRUN);
  write_memory("single address write: the same word again and again", INS_WRITE_ACK_SINGLE,
               KEY_GUEST, target + 16, data, sizeof(data), STATUS_OK);
  read_memory("single address write readback", INS_READ_INC, target + 16, &data[4], 4, STATUS_OK);
}
