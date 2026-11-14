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
 * Scriptable peer of the GRSPW device (TEST_PLAN.md, section 5). The steps are
 * given as arguments and run in order:
 *
 *   wait          wait for the next message from the device and keep it
 *   ack           acknowledge the kept message (core of the kept message)
 *   report        send the kept message back to the guest as a data packet:
 *                 42 F0 <type> <length, 4 bytes big-endian> <64 first bytes>
 *   send=HEX      send a data packet with these bytes
 *   fill=LEN      send a data packet of LEN bytes: 42 F0 00 01 02 ...
 *   seq=N         send N data packets 42 F0 <i>, i from 0 to N-1
 *   time=HEX      send a time code
 *   sleep=MS      wait MS milliseconds
 *   rmapw=KEY     send an RMAP write command (incrementing, no reply) of the
 *                 4 bytes "OK!!" to the address in bytes 2 to 5 of the kept
 *                 message, with the destination key KEY (hexadecimal)
 *   acks          from now on, acknowledge every data packet
 *
 * After the last step the peer keeps reading the messages of the device without
 * acknowledging them, unless "acks" was given. The core of the messages sent is
 * the one of the last message received, 0 before the first one.
 */

#include <fcntl.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grlib_mqgrspw.h"

#define QUEUE_MSG_COUNT   10
#define MSG_HEADER_SIZE   (C_HEADER_IDX + 1 + C_DATA_SIZE)
#define NODE_GUEST        0x42
#define NODE_PEER         0x67
#define PROTOCOL_USER     0xF0
#define PROTOCOL_RMAP     0x01
#define INS_WRITE_NOACK   0x64
#define REPORT_BYTES      64

static mqd_t from_qemu;
static mqd_t to_qemu;
static uint8_t kept[C_MAXQ_SIZE];
static ssize_t kept_size;
static uint8_t core;

static void die(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

static uint8_t crc8(const uint8_t *data, size_t size)
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

static void send_message(uint8_t word, const uint8_t *packet, uint32_t size)
{
  static uint8_t message[C_MAXQ_SIZE];

  if (MSG_HEADER_SIZE + size > C_MAXQ_SIZE) {
    fprintf(stderr, "peer_script: packet of %u bytes too large\n", size);
    exit(EXIT_FAILURE);
  }
  memset(message, 0, MSG_HEADER_SIZE);
  message[C_SPW_ID_IDX] = core;
  message[C_PWORD_IDX] = word;
  memcpy(&message[C_HEADER_IDX + 1], &size, sizeof(size));
  if (size > 0) {
    memcpy(&message[MSG_HEADER_SIZE], packet, size);
  }
  if (mq_send(to_qemu, (const char *) message, MSG_HEADER_SIZE + size, 0) != 0) {
    die("mq_send");
  }
}

static size_t parse_hex(const char *text, uint8_t *bytes, size_t max)
{
  size_t size = 0;

  while (text[0] != '\0' && text[1] != '\0') {
    unsigned int value;

    if (size == max || sscanf(text, "%2x", &value) != 1) {
      fprintf(stderr, "peer_script: bad hexadecimal string\n");
      exit(EXIT_FAILURE);
    }
    bytes[size++] = (uint8_t) value;
    text += 2;
  }

  return size;
}

/* The packet of a message is the concatenation of its header and its data. */
static uint32_t message_packet(const uint8_t *message, ssize_t size, uint8_t *packet)
{
  uint32_t header_size = message[C_HEADER_IDX];
  uint32_t data_size;

  if (size < (ssize_t) (MSG_HEADER_SIZE + header_size)) {
    return 0;
  }
  memcpy(packet, &message[C_HEADER_IDX + 1], header_size);
  memcpy(&data_size, &message[C_HEADER_IDX + 1 + header_size], sizeof(data_size));
  memcpy(&packet[header_size], &message[MSG_HEADER_SIZE + header_size], data_size);

  return header_size + data_size;
}

static void receive(void)
{
  kept_size = mq_receive(from_qemu, (char *) kept, sizeof(kept), NULL);
  if (kept_size < 0) {
    die("mq_receive");
  }
  core = kept[C_SPW_ID_IDX];
  printf("peer_script: received type %u from core %u, %zd bytes\n", kept[C_PWORD_IDX], core,
         kept_size);
  fflush(stdout);
}

static void report(void)
{
  static uint8_t packet[C_MAXQ_SIZE];
  uint8_t out[3 + 4 + REPORT_BYTES] = { NODE_GUEST, PROTOCOL_USER };
  uint32_t size = message_packet(kept, kept_size, packet);
  uint32_t shown = size < REPORT_BYTES ? size : REPORT_BYTES;

  out[2] = kept[C_PWORD_IDX];
  out[3] = (uint8_t) (size >> 24);
  out[4] = (uint8_t) (size >> 16);
  out[5] = (uint8_t) (size >> 8);
  out[6] = (uint8_t) size;
  memcpy(&out[7], packet, shown);
  send_message(C_MSGQ_DATA, out, 7 + shown);
}

static void rmap_write(uint8_t key)
{
  static const uint8_t data[] = { 'O', 'K', '!', '!' };
  static uint8_t packet[C_MAXQ_SIZE];
  uint8_t command[16 + sizeof(data) + 1];
  uint32_t size = message_packet(kept, kept_size, packet);

  if (size < 6) {
    fprintf(stderr, "peer_script: rmapw needs an address in the kept message\n");
    exit(EXIT_FAILURE);
  }
  command[0] = NODE_GUEST;
  command[1] = PROTOCOL_RMAP;
  command[2] = INS_WRITE_NOACK;
  command[3] = key;
  command[4] = NODE_PEER;
  command[5] = 0;
  command[6] = 1;
  command[7] = 0;
  memcpy(&command[8], &packet[2], 4);
  command[12] = 0;
  command[13] = 0;
  command[14] = sizeof(data);
  command[15] = crc8(command, 15);
  memcpy(&command[16], data, sizeof(data));
  command[16 + sizeof(data)] = crc8(data, sizeof(data));
  send_message(C_MSGQ_DATA, command, sizeof(command));
}

int main(int argc, char **argv)
{
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  static uint8_t packet[C_MAXQ_SIZE];
  int ack_all = 0;
  int i;

  from_qemu = mq_open(C_SPWQ, O_CREAT | O_RDONLY, 0644, &attr);
  to_qemu = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  if (from_qemu == (mqd_t) -1 || to_qemu == (mqd_t) -1) {
    die("mq_open");
  }

  for (i = 1; i < argc; ++i) {
    const char *step = argv[i];
    unsigned long value;

    printf("peer_script: %s\n", step);
    fflush(stdout);
    if (strcmp(step, "wait") == 0) {
      receive();
    } else if (strcmp(step, "ack") == 0) {
      send_message(C_MSGQ_ACK, NULL, 0);
    } else if (strcmp(step, "report") == 0) {
      report();
    } else if (strcmp(step, "acks") == 0) {
      ack_all = 1;
    } else if (strncmp(step, "send=", 5) == 0) {
      send_message(C_MSGQ_DATA, packet, (uint32_t) parse_hex(step + 5, packet, sizeof(packet)));
    } else if (strncmp(step, "time=", 5) == 0) {
      uint8_t code;

      parse_hex(step + 5, &code, 1);
      send_message(C_MSGQ_TIME, &code, 1);
    } else if (sscanf(step, "fill=%lu", &value) == 1) {
      uint32_t n;

      for (n = 0; n < value && n < sizeof(packet); ++n) {
        packet[n] = (uint8_t) (n - 2);
      }
      packet[0] = NODE_GUEST;
      packet[1] = PROTOCOL_USER;
      send_message(C_MSGQ_DATA, packet, (uint32_t) value);
    } else if (sscanf(step, "seq=%lu", &value) == 1) {
      unsigned long n;

      for (n = 0; n < value; ++n) {
        uint8_t small[] = { NODE_GUEST, PROTOCOL_USER, (uint8_t) n };

        send_message(C_MSGQ_DATA, small, sizeof(small));
      }
    } else if (sscanf(step, "sleep=%lu", &value) == 1) {
      struct timespec delay = { (time_t) (value / 1000), (long) (value % 1000) * 1000000L };

      nanosleep(&delay, NULL);
    } else if (sscanf(step, "rmapw=%lx", &value) == 1) {
      rmap_write((uint8_t) value);
    } else {
      fprintf(stderr, "peer_script: unknown step %s\n", step);
      return EXIT_FAILURE;
    }
  }

  for (;;) {
    receive();
    if (ack_all && kept[C_PWORD_IDX] == C_MSGQ_DATA) {
      send_message(C_MSGQ_ACK, NULL, 0);
    }
  }
}
