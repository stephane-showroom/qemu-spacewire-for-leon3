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
 * REQ_GRSPW_159: soft reset of the packet driver
 *
 * @tc     TC_GRSPW_051
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait ack rmapw=20
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  regs->ctr = CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0);
  regs->key = DESTINATION_KEY;
  write_chn(CHN_LE | CHN_NS | CHN_RE);
  regs->tim = 0x15;
  /* The register writes of grspw_hw_stop() and grspw_hw_softreset() */
  regs->chn = (regs->chn & (CHN_LE | CHN_NS | 0xE000U)) | CHN_AT;
  regs->chn = regs->chn & (CHN_LE | 0x2000U);
  regs->ctr = regs->ctr & (CTR_LD | CTR_LS | CTR_AS | CTR_RE | CTR_RD | CTR_NP | CTR_PS);
  regs->rxl = 1024;
  regs->txd = 0;
  regs->rxd = 0;
  regs->str = 0xFFFFFFFFU;
  regs->tim = 0;
  check(STR_LINK(regs->str) == LINK_RUN, "link still in run");
  check((regs->ctr & (CTR_LS | CTR_RE)) == (CTR_LS | (IS_RMAP_CORE ? CTR_RE : 0)), "LS and RE kept");
  check(regs->chn == CHN_LE, "DMACTRL keeps LE only");
  check(regs->rxl == 1024 && regs->txd == 0 && regs->rxd == 0, "DMA registers");
  check((regs->str & (STR_ERRORS | STR_TO)) == 0 && regs->tim == 0, "STS and TIME cleared");
  if (IS_RMAP_CORE) {
    regs->txd = (uint32_t) tx_table;
    set_tx_area_address(0);
    write_chn(CHN_LE | CHN_TE);
    WAIT_FOR(area_written(), "RMAP command not executed after the soft reset");
  }
}
