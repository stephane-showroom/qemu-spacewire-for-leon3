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
 * REQ_GRSPW_148: message queues
 *
 * @tc     TC_GRSPW_044
 * @side   peer
 * @cores  0
 */

#include "req_peer.h"

static void test(void)
{
  struct mq_attr attr;

  check(mq_getattr(from_guest, &attr) == 0 && attr.mq_maxmsg == 10 && attr.mq_msgsize == 8192,
        "/spwq: 10 messages of 8192 bytes");
  check(mq_getattr(to_guest, &attr) == 0 && attr.mq_maxmsg == 10 && attr.mq_msgsize == 8192,
        "/devq: 10 messages of 8192 bytes");
  check(first_message[C_SPW_ID_IDX] == core, "packet of the guest received on /spwq");
}
