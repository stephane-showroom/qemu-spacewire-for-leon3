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
 * REQ_GRSPW_123: TX descriptor HEADERLEN
 *
 * Descriptor 3 has no header: its data is the start of tx_buffer[0].
 *
 * @tc     TC_GRSPW_064
 * @side   guest
 * @cores  0 1 2 3
 * @peer   peer_script wait report ack wait report ack wait report ack wait report ack
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
  static const uint8_t data[] = { 0xD1, 0xD2, 0xD3 };
  static const uint32_t header_sizes[] = { 2, 3, 255, 0 };
  static const uint32_t data_sizes[] = { 2, 0, 3, 3 };
  uint8_t expected[64];
  int i;

  setup();
  tx_buffer[0][0] = PEER_ADDRESS;
  tx_buffer[0][1] = PROTOCOL_ID;
  for (i = 2; i < 255; ++i) {
    tx_buffer[0][i] = (uint8_t) i;
  }
  for (i = 0; i < 4; ++i) {
    enable_rx(i, rx_buffer[i], 0);
    set_descriptor(i, i == 3 ? (const void *) data : tx_buffer[0], header_sizes[i],
                   i == 3 ? (const void *) tx_buffer[0] : data, data_sizes[i], 0);
  }
  write_chn(CHN_RE | CHN_RD | CHN_TE);
  wait_rx(3);
  for (i = 0; i < 4; ++i) {
    uint32_t size = header_sizes[i] + data_sizes[i];
    uint32_t shown = size < 64 ? size : 64;

    if (i == 3) {
      memcpy(expected, tx_buffer[0], 3);
    } else {
      memcpy(expected, tx_buffer[0], header_sizes[i] < shown ? header_sizes[i] : shown);
      if (header_sizes[i] < shown) {
        memcpy(&expected[header_sizes[i]], data, shown - header_sizes[i]);
      }
    }
    check_report(i, MSG_DATA, size, expected, shown,
                 "HEADERLEN bytes of header sent before the data");
    check((tx_table[i].ctrl & 0xFFU) == header_sizes[i], "HEADERLEN kept when the core writes word 0");
  }
}
