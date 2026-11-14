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
 * REQ_GRSPW_136: abort
 *
 * @tc     TC_GRSPW_036
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait
 */

#include "req_guest.h"

static void test(void)
{
  setup();
  regs->ctr = CTR_LD;
  trigger(0);
  write_chn(CHN_TE);
  check((regs->chn & CHN_TE) != 0, "TE stays set while the link is down");
  write_chn(CHN_AT);
  check((regs->chn & (CHN_TE | CHN_AT)) == 0, "AT with no transmission only clears TE");
  check((tx_table[0].ctrl & TXD_EN) != 0, "descriptor untouched by AT without transmission");
  regs->ctr = CTR_LS;
  settle();
  check((tx_table[0].ctrl & TXD_EN) != 0, "nothing sent after AT");

  write_chn(CHN_TE);
  settle();
  write_chn(CHN_AT);
  WAIT_FOR((tx_table[0].ctrl & TXD_EN) == 0, "the pending descriptor is not completed by the abort");
  check((tx_table[0].ctrl & TXD_LE) != 0, "LE set by the abort");
  check((regs->chn & CHN_TE) == 0, "TE cleared by the abort");
}
