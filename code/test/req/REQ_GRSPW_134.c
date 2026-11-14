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
 * REQ_GRSPW_134: a packet is sent when the peer acknowledges it
 *
 * @tc     TC_GRSPW_035
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait sleep=3000 ack
 */

#include "req_guest.h"

static void test(void)
{
  int i;

  setup();
  trigger(0);
  write_chn(CHN_TE);
  for (i = 0; i < 20; ++i) {
    rtems_task_wake_after(1);
    check((tx_table[0].ctrl & TXD_EN) != 0 && (regs->chn & CHN_TE) != 0,
          "EN and TE stay set until the acknowledge");
  }
  wait_tx(0);
  check((tx_table[0].ctrl & TXD_LE) == 0, "completed without error by the acknowledge");
}
