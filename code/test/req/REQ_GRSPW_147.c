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
 * REQ_GRSPW_147: read over the transport limit
 *
 * @tc     TC_GRSPW_042
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static uint8_t message[C_MAXQ_SIZE];
  const uint32_t fits = C_MAXQ_SIZE - MSG_HEADER_SIZE - READ_REPLY_HEADER - 1;
  const uint8_t *reply = &message[MSG_HEADER_SIZE];
  uint8_t packet[64];
  size_t header;
  ssize_t received;

  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, target, fits + 1);
  exchange("reply too large for the transport: error 10", packet, header, 0, STATUS_NOT_AUTHORISED,
           true, NO_DATA, 0);
  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, target, fits);
  send_packet(core, C_MSGQ_DATA, packet, (uint32_t) header);
  received = receive_message(message, 0, REPLY_TIMEOUT_NS);
  check(received == (ssize_t) C_MAXQ_SIZE && message[C_PWORD_IDX] == C_MSGQ_RMAP,
        "the largest reply that fits in the transport is sent");
  if (received == (ssize_t) C_MAXQ_SIZE) {
    check(reply[3] == STATUS_OK && crc8(reply, READ_REPLY_HEADER) == 0
          && crc8(&reply[READ_REPLY_HEADER], fits + 1) == 0, "content of the largest reply");
  }
}
