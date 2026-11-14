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
 * REQ_GRSPW_130: packet composition
 *
 * @tc     TC_GRSPW_032
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void test(void)
{
  static const uint8_t header[] = { 0x05, PEER_ADDRESS, PROTOCOL_ID };
  static const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
  uint8_t expected[16];
  uint32_t size = 0;

  setup();
  enable_rx(0, rx_buffer[0], 0);
  memcpy(&expected[size], header, sizeof(header));
  size += sizeof(header);
  if (IS_RMAP_CORE) {
    expected[size++] = rmap_crc(&header[1], sizeof(header) - 1);
  }
  memcpy(&expected[size], data, sizeof(data));
  size += sizeof(data);
  if (IS_RMAP_CORE) {
    expected[size++] = rmap_crc(data, sizeof(data));
  }
  tx_table[0].header = (uint32_t) header;
  tx_table[0].len = sizeof(data);
  tx_table[0].data = (uint32_t) data;
  RTEMS_COMPILER_MEMORY_BARRIER();
  tx_table[0].ctrl = TXD_EN | TXD_HC | TXD_DC | TXD_NON_CRC(1) | sizeof(header);
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  check_report(0, MSG_DATA, size, expected, size,
               IS_RMAP_CORE ? "header, header CRC without the first byte, data, data CRC"
                            : "header and data only: no CRC without RMAP CRC logic");
}
