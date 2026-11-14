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
 * REQ_GRSPW_145: RMAP replies before the DMA packets
 *
 * Partly an analysis (TEST_PLAN.md TC_GRSPW_043): the test shows that replies
 * are sent while a DMA transmission waits for its acknowledge. CTRL.RD has no
 * effect.
 *
 * @tc     TC_GRSPW_043
 * @side   peer
 * @cores  2 3
 */

#define PEER_NO_AUTO_ACK
#include "req_peer.h"

static void test(void)
{
  static const uint8_t data[] = { 0xA1, 0xA2, 0xA3, 0xA4 };

  /* The packet of the guest is not acknowledged yet: its transmission waits. */
  write_memory("write reply while a DMA packet waits", INS_WRITE_ACK_INC, KEY_GUEST, target,
               data, sizeof(data), STATUS_OK);
  read_memory("read reply while a DMA packet waits", INS_READ_INC, target, data, sizeof(data), STATUS_OK);
  send_ack();
}
