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
 * REQ_GRSPW_085: link error caused by LD during a transmission
 *
 * @tc     TC_GRSPW_012
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  trigger(0);
  trigger(1);
  write_chn(CHN_TE);
  settle();
  regs->ctr = CTR_LD;
  WAIT_FOR((tx_table[0].ctrl & TXD_EN) == 0, "the pending descriptor is not completed by the link error");
  check((tx_table[0].ctrl & TXD_LE) != 0, "LE set in the descriptor");
  check((regs->chn & CHN_PS) != 0, "PS set");
}
