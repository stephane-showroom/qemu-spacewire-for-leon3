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
 * REQ_GRSPW_090: time-code reception
 *
 * @tc     TC_GRSPW_016
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack time=11 send=42f0e1 wait ack time=11 send=42f0e2
 */

#include "req_guest.h"

static void test(void)
{
  int base;

  setup();
  enable_rx(0, rx_buffer[0], 0);
  enable_rx(1, rx_buffer[1], 0);
  regs->tim = 0x10;
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check(regs->tim == 0x10 && (regs->str & STR_TO) == 0, "time code ignored without TR");

  base = irq_count[CORE];
  regs->ctr = CTR_LS | CTR_TR | CTR_TQ | CTR_IE;
  trigger(1);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  rtems_task_wake_after(2);
  check(regs->tim == 0x11, "counter stored with TR");
  check((regs->str & STR_TO) != 0, "TO set");
  check(irq_count[CORE] == base + 1, "tick-out interrupt with TQ and IE");
}
