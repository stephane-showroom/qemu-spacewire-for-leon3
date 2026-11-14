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
 * REQ_GRSPW_152: one interrupt line per core
 *
 * @tc     TC_GRSPW_049
 * @side   guest
 * @cores  0
 * @peer   device_simu
 */

#include "req_guest.h"

static void test(void)
{
  static rx_descriptor rx_tables[CORE_COUNT][128] __attribute__((aligned(1024)));
  static tx_descriptor tx_tables[CORE_COUNT][64] __attribute__((aligned(1024)));
  static uint8_t data[CORE_COUNT][16] __attribute__((aligned(4)));
  static uint8_t received[CORE_COUNT][16] __attribute__((aligned(4)));
  int core;

  for (core = 1; core < CORE_COUNT; ++core) {
    if (rtems_interrupt_handler_install(CORE_IRQ(core), "grspw", RTEMS_INTERRUPT_UNIQUE,
                                        count_irq, (void *) (intptr_t) core) != RTEMS_SUCCESSFUL) {
      fail("rtems_interrupt_handler_install");
    }
  }
  for (core = 0; core < CORE_COUNT; ++core) {
    registers *const r = (registers *) CORE_ADDRESS(core);
    int before[CORE_COUNT];
    int other;

    for (other = 0; other < CORE_COUNT; ++other) {
      before[other] = irq_count[other];
    }
    r->ctr = CTR_RS;
    r->ctr = CTR_LS;
    r->ndr = NODE_ADDRESS;
    r->rxl = sizeof(received[core]);
    r->rxd = (uint32_t) rx_tables[core];
    r->txd = (uint32_t) tx_tables[core];
    rx_tables[core][0].addr = (uint32_t) received[core];
    rx_tables[core][0].ctrl = RXD_EN | RXD_IE;
    data[core][0] = NODE_ADDRESS;
    data[core][1] = PROTOCOL_ID;
    data[core][2] = (uint8_t) (0x80 | core);
    tx_tables[core][0].header = 0;
    tx_tables[core][0].len = 3;
    tx_tables[core][0].data = (uint32_t) data[core];
    tx_tables[core][0].ctrl = TXD_EN;
    RTEMS_COMPILER_MEMORY_BARRIER();
    r->chn = CHN_RE | CHN_RD | CHN_RI | CHN_TE;
    WAIT_FOR((rx_tables[core][0].ctrl & RXD_EN) == 0, "packet not received");
    rtems_task_wake_after(2);
    for (other = 0; other < CORE_COUNT; ++other) {
      check(irq_count[other] == before[other] + (other == core),
            "only the interrupt line of the core is raised");
    }
  }
}
