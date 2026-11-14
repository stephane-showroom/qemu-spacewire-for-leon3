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
 * REQ_GRSPW_157: reset by CTRL.RS
 *
 * @tc     TC_GRSPW_004, TC_GRSPW_050
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait send=42f0aa time=01 sleep=500 ack time=01
 */

#include "req_guest.h"

static void test(void)
{
  uint32_t interrupts = CHN_AI | CHN_RI | CHN_TI;

  setup();
  regs->rxl = 64;
  regs->txd = (uint32_t) tx_table | 0x10;
  regs->ctr = CTR_LI | CTR_TQ | CTR_LS | CTR_PM;
  regs->ndr = 0x11;
  write_chn(interrupts);
  regs->ctr = CTR_RS | CTR_LI | CTR_TQ;
  check((regs->ctr & ~(CTR_CAPABILITIES | CTR_RE | CTR_AS)) == (CTR_LI | CTR_TQ),
        "CTRL: LI and TQ kept, the other fields reset");
  check(regs->ndr == 254, "NODEADDR reset");
  check(regs->chn == interrupts, "DMACTRL: only AI, RI and TI kept");
  check(regs->rxl == 64, "DMAMAXLEN kept");
  check(regs->txd == (uint32_t) tx_table, "table base kept, selector reset");

  setup();
  regs->tim = 0;
  regs->ctr = CTR_LS | CTR_TR;
  trigger(0);
  write_chn(CHN_RE | CHN_NS | CHN_TE);
  WAIT_FOR((regs->str & STR_TO) != 0, "time code of the peer not received");
  regs->ctr = CTR_RS;
  regs->ctr = CTR_LS | CTR_TR;
  regs->ndr = NODE_ADDRESS;
  enable_rx(0, rx_buffer[0], 0);
  write_chn(CHN_RE | CHN_RD);
  WAIT_FOR((regs->str & STR_TO) != 0, "second time code of the peer not received");
  check((tx_table[0].ctrl & TXD_EN) != 0, "the transmission stopped: the late acknowledge completes nothing");
  check((regs->chn & (CHN_TE | CHN_PS)) == 0, "TE and PS cleared by the reset");
  check((rx_table[0].ctrl & RXD_EN) != 0, "the kept packet is forgotten");
}
