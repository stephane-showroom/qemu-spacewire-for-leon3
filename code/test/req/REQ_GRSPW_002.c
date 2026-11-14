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
 * REQ_GRSPW_002: plug and play record
 *
 * @tc     TC_GRSPW_002
 * @side   guest
 * @cores  0
 * @peer   none
 */

#include "req_guest.h"

#include <grlib/ambapp.h>

typedef struct {
  int count;
  uint32_t start[8];
  uint32_t mask[8];
  int irq[8];
} pnp_list;

static int pnp_found(struct ambapp_dev *dev, int index, void *arg)
{
  pnp_list *list = arg;
  struct ambapp_apb_info *apb = DEV_TO_APB(dev);

  (void) index;
  if (list->count < 8) {
    list->start[list->count] = apb->start;
    list->mask[list->count] = apb->mask;
    list->irq[list->count] = apb->common.irq;
  }
  ++list->count;
  return 0;
}

__attribute__((unused)) static int pnp_index(const pnp_list *list, uint32_t start)
{
  int i;

  for (i = 0; i < list->count && i < 8; ++i) {
    if (list->start[i] == start) {
      return i;
    }
  }
  return -1;
}

static void test(void)
{
  pnp_list list = { 0 };
  int i;

  ambapp_for_each(ambapp_plb(), OPTIONS_ALL | OPTIONS_APB_SLVS, VENDOR_GAISLER, GAISLER_SPW,
                  pnp_found, &list);
  check(list.count == CORE_COUNT, "four records with vendor 0x01 and device 0x1F");
  for (i = 0; i < list.count && i < 8; ++i) {
    check(list.mask[i] == 0x100, "APB bar of 256 bytes");
  }
}
