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
 * REQ_GRSPW_153: link event interrupts
 *
 * The link error part (CTRL.LI) is blocked with REQ_GRSPW_086.
 *
 * @tc     TC_GRSPW_014, TC_GRSPW_016
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack time=01 send=42f0e1 wait ack time=02 send=42f0e2 wait ack time=03 send=42f0e3
 */

#include "req_guest.h"

static void test(void)
{
  static const uint32_t ctr[] = { CTR_TQ | CTR_IE, CTR_TQ, CTR_IE };
  static const int expected[] = { 1, 0, 0 };
  int i;

  setup();
  regs->tim = 0;
  for (i = 0; i < 3; ++i) {
    int base = irq_count[CORE];

    regs->ctr = CTR_LS | CTR_TR | ctr[i];
    enable_rx(i, rx_buffer[i], 0);
    trigger(i);
    write_chn(CHN_RE | CHN_RD | CHN_TE);
    wait_rx(i);
    rtems_task_wake_after(2);
    check((regs->str & STR_TO) != 0, "time code received");
    regs->str = STR_TO;
    check(irq_count[CORE] == base + expected[i],
          i == 0 ? "interrupt with TQ and IE" : "no interrupt without both TQ and IE");
  }
}
