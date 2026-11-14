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
 * REQ_GRSPW_114: memory error on reception
 *
 * @tc     TC_GRSPW_030
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  int base = irq_count[CORE];

  setup();
  enable_rx(0, (void *) UNMAPPED_ADDRESS, 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_AI | CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((regs->chn & CHN_RA) != 0, "RA not set by a memory error on reception");
  check((regs->chn & CHN_RE) == 0, "RE cleared");
  rtems_task_wake_after(2);
  check(irq_count[CORE] == base + 1, "AHB error interrupt with AI");
}
