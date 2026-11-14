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
 * Common part of the peer-side requirement tests (REQ_GRSPW_XXX.c with
 * "@side peer"): Linux programs that check what the device sends. They run
 * against the guest req/peer_guest.c, which sends the address of a 64-byte
 * memory area, the "target", then waits for the end packet 42 F0 <status>.
 * A test file includes this header and defines test(); main() sends a status of
 * 0 when all checks passed. The guest then prints TEST OK.
 *
 * The guest packet is acknowledged before test() unless PEER_NO_AUTO_ACK is
 * defined before the include.
 */

#ifndef REQ_PEER_H_
#define REQ_PEER_H_

#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grlib_mqgrspw.h"

#define NODE_GUEST        0x42
#define NODE_INITIATOR    0x67
#define KEY_GUEST         0x20
#define KEY_WRONG         0x21
#define PROTOCOL_RMAP     0x01
#define PROTOCOL_USER     0xF0

#define INS_WRITE_ACK_INC     0x6C
#define INS_WRITE_ACK_SINGLE  0x68
#define INS_WRITE_NOACK_INC   0x64
#define INS_WRITE_NOACK_SINGLE 0x60
#define INS_VERIFY_ACK_INC    0x7C
#define INS_VERIFY_NOACK_INC  0x74
#define INS_READ_INC          0x4C
#define INS_READ_SINGLE       0x48
#define INS_RMW               0x5C
#define INS_UNUSED_REPLY      0x58

#define STATUS_OK             0
#define STATUS_GENERAL        1
#define STATUS_UNUSED         2
#define STATUS_KEY            3
#define STATUS_DATA_CRC       4
#define STATUS_EARLY_EOP      5
#define STATUS_TOO_MUCH_DATA  6
#define STATUS_VERIFY_OVERRUN 9
#define STATUS_NOT_AUTHORISED 10
#define STATUS_RMW_LENGTH     11

#define COMMAND_HEADER_SIZE   16
#define READ_REPLY_HEADER     12
#define WRITE_REPLY_HEADER    8

#define UNMAPPED_ADDRESS      0x10000000U
#define REPLY_TIMEOUT_NS      100000000L
#define GUEST_TIMEOUT_S       5
#define MSG_HEADER_SIZE       (C_HEADER_IDX + 1 + C_DATA_SIZE)
#define QUEUE_MSG_COUNT       10
#define NO_DATA               ((const uint8_t *) NULL)

static mqd_t from_guest;
static mqd_t to_guest;
static int core;
static uint32_t target;
static uint16_t next_tid;
static int failures;
static uint8_t first_message[C_MAXQ_SIZE];
static ssize_t first_size;

static void test(void);

__attribute__((unused)) static uint8_t crc8(const uint8_t *data, size_t size)
{
  uint8_t crc = 0;

  while (size-- > 0) {
    int bit;

    crc ^= *data++;
    for (bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (uint8_t) ((crc >> 1) ^ 0xE0) : (uint8_t) (crc >> 1);
    }
  }

  return crc;
}

__attribute__((unused)) static void check(bool condition, const char *name)
{
  if (!condition) {
    printf("CHECK FAILED: %s\n", name);
    fflush(stdout);
    ++failures;
  }
}

__attribute__((unused)) static void send_packet(int to_core, uint8_t word, const uint8_t *packet,
                                                uint32_t size)
{
  static uint8_t message[C_MAXQ_SIZE];

  memset(message, 0, MSG_HEADER_SIZE);
  message[C_SPW_ID_IDX] = (uint8_t) to_core;
  message[C_PWORD_IDX] = word;
  memcpy(&message[C_HEADER_IDX + 1], &size, sizeof(size));
  if (size > 0) {
    memcpy(&message[MSG_HEADER_SIZE], packet, size);
  }

  if (mq_send(to_guest, (const char *) message, MSG_HEADER_SIZE + size, 0) != 0) {
    perror("mq_send");
    exit(EXIT_FAILURE);
  }
}

__attribute__((unused)) static void send_ack(void)
{
  send_packet(core, C_MSGQ_ACK, NULL, 0);
}

__attribute__((unused)) static ssize_t receive_message(uint8_t *message, long seconds, long nanoseconds)
{
  struct timespec deadline;

  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_sec += seconds;
  deadline.tv_nsec += nanoseconds;
  if (deadline.tv_nsec >= 1000000000L) {
    deadline.tv_nsec -= 1000000000L;
    ++deadline.tv_sec;
  }

  return mq_timedreceive(from_guest, (char *) message, C_MAXQ_SIZE, NULL, &deadline);
}

/* Builds a command header and returns its size. */
__attribute__((unused)) static size_t build_header(uint8_t *packet, uint8_t instruction, uint8_t key,
                                                   const uint8_t *reply_address, uint32_t address,
                                                   uint32_t length)
{
  size_t tail = (size_t) (instruction & 3) * 4;
  uint16_t tid = next_tid++;

  packet[0] = NODE_GUEST;
  packet[1] = PROTOCOL_RMAP;
  packet[2] = instruction;
  packet[3] = key;
  if (tail > 0) {
    memcpy(&packet[4], reply_address, tail);
  }
  packet[tail + 4] = NODE_INITIATOR;
  packet[tail + 5] = (uint8_t) (tid >> 8);
  packet[tail + 6] = (uint8_t) tid;
  packet[tail + 7] = 0;
  packet[tail + 8] = (uint8_t) (address >> 24);
  packet[tail + 9] = (uint8_t) (address >> 16);
  packet[tail + 10] = (uint8_t) (address >> 8);
  packet[tail + 11] = (uint8_t) address;
  packet[tail + 12] = (uint8_t) (length >> 16);
  packet[tail + 13] = (uint8_t) (length >> 8);
  packet[tail + 14] = (uint8_t) length;
  packet[tail + 15] = crc8(packet, tail + 15);

  return tail + COMMAND_HEADER_SIZE;
}

__attribute__((unused)) static size_t add_data(uint8_t *packet, size_t header_size,
                                               const uint8_t *data, size_t size)
{
  memcpy(&packet[header_size], data, size);
  packet[header_size + size] = crc8(data, size);

  return header_size + size + 1;
}

/*
 * Sends a command and checks the reply. expected_data is the data of a read or
 * RMW reply, NO_DATA for a write reply or when the reply has no data field.
 */
__attribute__((unused)) static void exchange(const char *name, const uint8_t *packet, size_t size,
                                             int prefix, uint8_t status, bool read_reply,
                                             const uint8_t *expected_data, size_t data_size)
{
  static uint8_t message[C_MAXQ_SIZE];
  const uint8_t *reply = &message[MSG_HEADER_SIZE + prefix];
  size_t header_size = read_reply ? READ_REPLY_HEADER : WRITE_REPLY_HEADER;
  size_t expected = (size_t) prefix + header_size;
  ssize_t received;

  send_packet(core, C_MSGQ_DATA, packet, (uint32_t) size);

  received = receive_message(message, 0, REPLY_TIMEOUT_NS);
  if (received < 0 || message[C_PWORD_IDX] != C_MSGQ_RMAP) {
    check(false, name);
    return;
  }

  if (expected_data != NO_DATA) {
    expected += data_size + 1;
  }

  check((size_t) received == MSG_HEADER_SIZE + expected, name);
  check(reply[0] == NODE_INITIATOR && reply[1] == PROTOCOL_RMAP, name);
  check((reply[2] & 0xC0) == 0 && reply[3] == status && reply[4] == NODE_GUEST, name);
  check(crc8(reply, header_size) == 0, name);

  if (expected_data != NO_DATA) {
    check(memcmp(&reply[READ_REPLY_HEADER], expected_data, data_size) == 0, name);
    check(crc8(&reply[READ_REPLY_HEADER], data_size + 1) == 0, name);
  }
}

__attribute__((unused)) static void expect_no_reply(const char *name, const uint8_t *packet, size_t size)
{
  static uint8_t message[C_MAXQ_SIZE];

  send_packet(core, C_MSGQ_DATA, packet, (uint32_t) size);
  check(receive_message(message, 0, REPLY_TIMEOUT_NS) < 0, name);
}

__attribute__((unused)) static void write_memory(const char *name, uint8_t instruction, uint8_t key,
                                                 uint32_t address, const uint8_t *data, size_t size,
                                                 uint8_t status)
{
  uint8_t packet[256];
  size_t header = build_header(packet, instruction, key, NULL, address, (uint32_t) size);

  exchange(name, packet, add_data(packet, header, data, size), 0, status, false, NO_DATA, 0);
}

__attribute__((unused)) static void read_memory(const char *name, uint8_t instruction, uint32_t address,
                                                const uint8_t *expected, size_t size, uint8_t status)
{
  uint8_t packet[64];
  size_t header = build_header(packet, instruction, KEY_GUEST, NULL, address, (uint32_t) size);

  exchange(name, packet, header, 0, status, true, expected, size);
}

int main(void)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  const uint8_t *packet = &first_message[MSG_HEADER_SIZE];
  uint8_t end[] = { NODE_GUEST, PROTOCOL_USER, 0 };

  from_guest = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  to_guest = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  if (from_guest == (mqd_t) -1 || to_guest == (mqd_t) -1) {
    perror("mq_open");
    return EXIT_FAILURE;
  }

  first_size = receive_message(first_message, GUEST_TIMEOUT_S, 0);
  if (first_size < (ssize_t) (MSG_HEADER_SIZE + 6)) {
    printf("CHECK FAILED: no packet from the guest\n");
    return EXIT_FAILURE;
  }
  core = first_message[C_SPW_ID_IDX];
  target = (uint32_t) packet[2] << 24 | (uint32_t) packet[3] << 16
         | (uint32_t) packet[4] << 8 | packet[5];
#ifndef PEER_NO_AUTO_ACK
  send_ack();
#endif

  test();

  end[2] = failures == 0 ? 0 : 0xEE;
  send_packet(core, C_MSGQ_DATA, end, sizeof(end));
  printf(failures == 0 ? "PEER CHECKS OK\n" : "PEER CHECKS FAILED: %d\n", failures);
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif /* REQ_PEER_H_ */
