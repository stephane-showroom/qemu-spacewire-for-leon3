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
 * REQ_GRSPW_128: TX descriptor word 2 reserved bits
 *
 * @tc     TC_GRSPW_064
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack
 */

#include "req_guest.h"

static void set_descriptor(int index, const void *header, uint32_t header_size,
                           const void *data, uint32_t data_size, uint32_t flags)
{
  tx_table[index].header = (uint32_t) header;
  tx_table[index].len = data_size;
  tx_table[index].data = (uint32_t) data;
  RTEMS_COMPILER_MEMORY_BARRIER();
  tx_table[index].ctrl = TXD_EN | flags | header_size;
}

/* The packet expected from a descriptor; the CRCs only on the RMAP cores. */
__attribute__((unused)) static uint32_t expected_packet(uint8_t *out, const uint8_t *header,
                                                        uint32_t header_size, uint32_t non_crc,
                                                        int hc, const uint8_t *data,
                                                        uint32_t data_size, int dc)
{
  uint32_t size = 0;

  memcpy(&out[size], header, header_size);
  size += header_size;
  if (hc && IS_RMAP_CORE) {
    out[size++] = rmap_crc(header + non_crc, header_size - non_crc);
  }
  memcpy(&out[size], data, data_size);
  size += data_size;
  if (dc && IS_RMAP_CORE) {
    out[size++] = rmap_crc(data, data_size);
  }
  return size;
}

static void test(void)
{
  static const uint8_t header[] = { PEER_ADDRESS, PROTOCOL_ID };
  static const uint8_t data[] = { 1, 2, 3, 4 };
  uint8_t expected[8];
  uint32_t size;

  setup();
  enable_rx(0, rx_buffer[0], 0);
  set_descriptor(0, header, sizeof(header), data, sizeof(data), 0);
  tx_table[0].len = sizeof(data) | 0xFF000000U;
  RTEMS_COMPILER_MEMORY_BARRIER();
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(0);
  size = expected_packet(expected, header, sizeof(header), 0, 0, data, sizeof(data), 0);
  check_report(0, MSG_DATA, size, expected, size, "DATALEN is bits 23:0 only");
}
