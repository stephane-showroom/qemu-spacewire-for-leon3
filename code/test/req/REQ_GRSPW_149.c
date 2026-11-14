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
 * REQ_GRSPW_149: message format
 *
 * @tc     TC_GRSPW_044
 * @side   peer
 * @cores  0 1 2 3
 */

#include "req_peer.h"

static void test(void)
{
  uint32_t header_size = first_message[C_HEADER_IDX];
  uint32_t data_size;

  memcpy(&data_size, &first_message[C_HEADER_IDX + 1 + header_size], sizeof(data_size));
  check(first_message[C_PWORD_IDX] == C_MSGQ_DATA, "type DATA");
  check((ssize_t) (MSG_HEADER_SIZE + header_size + data_size) == first_size,
        "[core][type][header length][header][data length, u32][data]");
  check(header_size + data_size == 6, "the packet of the guest: 2 bytes and an address");
}
