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
 * REQ_GRSPW_141: destination key
 *
 * @tc     TC_GRSPW_039
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  static const uint8_t reference[] = { 0x55, 0x66, 0x77, 0x88 };
  static const uint8_t other[] = { 0x11, 0x22, 0x33, 0x44 };

  write_memory("setup", INS_WRITE_ACK_INC, KEY_GUEST, target + 28, reference, sizeof(reference), STATUS_OK);
  write_memory("invalid key: error 3", INS_WRITE_ACK_INC, KEY_WRONG, target + 28, other, sizeof(other), STATUS_KEY);
  read_memory("command with an invalid key not executed", INS_READ_INC, target + 28, reference,
              sizeof(reference), STATUS_OK);
}
