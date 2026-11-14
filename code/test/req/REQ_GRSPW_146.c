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
 * REQ_GRSPW_146: read of invalid memory
 *
 * @tc     TC_GRSPW_039
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t data[] = { 1, 2, 3, 4 };
  uint8_t packet[64];
  size_t header;

  write_memory("write to unmapped memory: error 1", INS_WRITE_ACK_INC, KEY_GUEST, UNMAPPED_ADDRESS,
               data, sizeof(data), STATUS_GENERAL);
  header = build_header(packet, INS_READ_INC, KEY_GUEST, NULL, UNMAPPED_ADDRESS, 4);
  exchange("read of unmapped memory: correct header, truncated reply", packet, header, 0, STATUS_OK,
           true, NO_DATA, 0);
}
