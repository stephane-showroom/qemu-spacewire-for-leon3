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
 * REQ_GRSPW_135: link error during a transmission and DMACTRL.LE
 *
 * @tc     TC_GRSPW_012
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait wait ack wait
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  trigger(0);
  trigger(1);
  trigger(2);
  write_chn(CHN_TE);
  settle();
  regs->ctr = CTR_LD;
  WAIT_FOR((tx_table[0].ctrl & TXD_EN) == 0, "the pending descriptor is not completed by the link error");
  check((tx_table[0].ctrl & TXD_LE) != 0, "LE set, rest of the packet discarded");
  regs->ctr = CTR_LS;
  wait_tx(1);
  check((tx_table[1].ctrl & TXD_LE) == 0, "with DMACTRL.LE = 0, next descriptor sent once the link is back");

  write_chn(CHN_LE | CHN_TE);
  settle();
  regs->ctr = CTR_LD;
  WAIT_FOR((tx_table[2].ctrl & TXD_EN) == 0, "the pending descriptor is not completed (DMACTRL.LE)");
  check((regs->chn & CHN_TE) == 0, "with DMACTRL.LE = 1, TE cleared");
  trigger(3);
  regs->ctr = CTR_LS;
  settle();
  check((tx_table[3].ctrl & TXD_EN) != 0, "with DMACTRL.LE = 1, nothing sent after the stop");
}
