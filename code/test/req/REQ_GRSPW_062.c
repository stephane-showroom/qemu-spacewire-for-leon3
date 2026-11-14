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
 * REQ_GRSPW_062: DMACTRL.RA
 *
 * @tc     TC_GRSPW_061
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  check((regs->chn & CHN_RA) == 0, "power-on value");
  setup();
  write_chn(CHN_RA);
  check((regs->chn & CHN_RA) == 0, "a write of 1 does not set it");
  enable_rx(0, (void *) UNMAPPED_ADDRESS, 0);
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  WAIT_FOR((regs->chn & CHN_RA) != 0, "RA not set by a memory error on reception");
  write_chn(0);
  check((regs->chn & CHN_RA) != 0, "unchanged by a write of 0");
  write_chn(CHN_RA);
  check((regs->chn & CHN_RA) == 0, "cleared by a write of 1");
}
