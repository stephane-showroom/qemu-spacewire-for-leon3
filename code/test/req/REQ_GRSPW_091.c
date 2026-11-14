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
 * REQ_GRSPW_091: valid time codes
 *
 * The peer sends 0xD1 (counter 0x11 after 0x10, flags 3), 0x45 (counter 5
 * after 0x11, flags 1) and 0x86 (counter 6 after 5, flags 2).
 *
 * @tc     TC_GRSPW_016
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack time=d1 send=42f0e1 wait ack time=45 send=42f0e2 wait ack time=86 send=42f0e3
 */

#include "req_guest.h"

static void test(void)
{
  int base;

  setup();
  for (base = 0; base < 3; ++base) {
    enable_rx(base, rx_buffer[base], 0);
  }
  regs->tim = 0x10;
  regs->ctr = CTR_LS | CTR_TR | CTR_TQ | CTR_IE;

  base = irq_count[CORE];
  trigger(0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  rtems_task_wake_after(2);
  check(regs->tim == 0xD1, "valid (counter + 1): counter and control flags stored");
  check((regs->str & STR_TO) != 0, "valid: TO set");
  check(irq_count[CORE] == base + 1, "valid: tick-out interrupt with TQ and IE");

  regs->str = STR_TO;
  base = irq_count[CORE];
  trigger(1);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(1);
  rtems_task_wake_after(2);
  check(regs->tim == 0xC5, "not valid: counter stored, control flags kept");
  check((regs->str & STR_TO) == 0, "not valid: TO not set");
  check(irq_count[CORE] == base, "not valid: no interrupt");

  base = irq_count[CORE];
  trigger(2);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(2);
  rtems_task_wake_after(2);
  check(regs->tim == 0x86, "valid again from the stored counter: counter and flags stored");
  check((regs->str & STR_TO) != 0, "valid again: TO set");
  check(irq_count[CORE] == base + 1, "valid again: interrupt");
}
