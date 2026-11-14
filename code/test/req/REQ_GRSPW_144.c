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
 * REQ_GRSPW_144: read reply on error
 *
 * @tc     TC_GRSPW_039, TC_GRSPW_040
 * @side   peer
 * @cores  2 3
 */

#include "req_peer.h"

static void test(void)
{
  uint8_t packet[64];
  size_t header;

  header = build_header(packet, INS_READ_INC, KEY_WRONG, NULL, target, 4);
  exchange("no data in a read reply with an error", packet, header, 0, STATUS_KEY, true, NO_DATA, 0);
  header = build_header(packet, INS_READ_SINGLE, KEY_GUEST, NULL, target + 1, 0);
  exchange("authorisation failure reported for a zero length", packet, header, 0,
           STATUS_NOT_AUTHORISED, true, NO_DATA, 0);
}
