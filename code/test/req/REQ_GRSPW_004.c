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
 * REQ_GRSPW_004: one port and one DMA channel
 *
 * @tc     TC_GRSPW_002
 * @side   guest
 * @cores  0 1 2 3
 * @peer   none
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  check((regs->ctr & (CTR_PO | CTR_NCH)) == 0, "PO = 0 and CTRL[28:27] = 0");
  regs->ctr = CTR_LS | CTR_PS | CTR_NP;
  check((regs->ctr & (CTR_PS | CTR_NP)) == 0, "PS and NP read 0 with one port");
  check((regs->str & (1U << 9)) == 0, "STS.AP reads 0 with one port");
}
