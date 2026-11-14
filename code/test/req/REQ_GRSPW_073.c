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
 * REQ_GRSPW_073: DMACTRL read-modify-write
 *
 * @tc     TC_GRSPW_009
 * @side   guest
 * @cores  0 1 2 3
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  uint32_t before;

  setup();
  write_chn(CHN_TE);
  check((regs->chn & CHN_TE) == 0, "TE written with a disabled descriptor only clears TE again");
  write_chn(CHN_RE | CHN_RD);
  write_chn(CHN_RE | CHN_RD);
  check((regs->chn & (CHN_RE | CHN_RD)) == (CHN_RE | CHN_RD), "RE and RD written twice");
  enable_rx(0, rx_buffer[0], 0);
  before = regs->chn;
  set_tx_bytes(0, NODE_ADDRESS, "\x81", 1, 0);
  write_chn(before | CHN_TE);
  wait_rx(0);
  before = regs->chn & ~CHN_PR;
  check((regs->chn & CHN_PR) != 0, "PR set by the reception");
  write_chn(before);
  check((regs->chn & CHN_PR) != 0, "a status bit not written with 1 stays set");
}
