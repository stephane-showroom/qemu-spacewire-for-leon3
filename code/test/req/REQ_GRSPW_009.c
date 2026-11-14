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
 * REQ_GRSPW_009: byte and halfword accesses
 *
 * The result of the byte and halfword accesses is undefined: only the guest
 * reaching TEST OK and the later 32-bit accesses are checked.
 *
 * @tc     TC_GRSPW_005
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  volatile uint8_t *const byte = (volatile uint8_t *) CORE_BASE;
  volatile uint16_t *const half = (volatile uint16_t *) CORE_BASE;
  uint32_t sink = 0;

  setup();
  regs->tim = 0x2A;
  sink += byte[0x17];
  sink += half[0x16 / 2];
  sink += byte[0x00];
  byte[0x17] = 0x15;
  half[0x16 / 2] = 0x0015;
  byte[0x08] = 0x33;
  (void) sink;
  regs->tim = 0x2A;
  check(regs->tim == 0x2A, "32-bit accesses still work after byte and halfword accesses");
  regs->ndr = NODE_ADDRESS;
  check(regs->ndr == NODE_ADDRESS, "32-bit accesses to another register still work");
}
