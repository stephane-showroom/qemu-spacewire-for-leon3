/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 10, 2026                                      *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * Ground station of the demonstration, on CORE_GROUND.
 *
 * It waits for the address of the state variable, then commands the onboard
 * computer with RMAP: INIT -> WARMUP -> OPERATIONAL. Each write is a verified
 * write with reply, followed by a read of the variable. Once the scenario is
 * over, each step reads the state again. ground_command_fail() commands FAIL at
 * any time, when the demonstration stops. Every packet is printed in hexadecimal.
 *
 * The RMAP functions come from code/test/rmap_test.c. One command is in
 * progress at a time.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "demo_protocol.h"
#include "simu.h"

#define PROTOCOL_RMAP         0x01
#define INS_VERIFY_ACK_INC    0x7C
#define INS_READ_INC          0x4C
#define COMMAND_HEADER_SIZE   16
#define READ_REPLY_HEADER     12
#define WRITE_REPLY_HEADER    8
#define STATE_SIZE            4
#define REPLY_TIMEOUT_S       1.0
#define READBACK_DELAY_S      0.3   /* Lets the onboard computer react to a write */
#define MAX_OPERATIONS        4

typedef struct {
  bool write;           /* Write value, or read */
  uint32_t value;
} operation;

typedef struct {
  const char *name;
  int write;            /* Value to write, or -1 for a read only */
} step;

static const step scenario[] = {
  { "read the initial state", -1 },
  { "command WARMUP", STATE_WARMUP },
  { "command OPERATIONAL", STATE_OPERATIONAL },
};

#define STEP_COUNT  (sizeof(scenario) / sizeof(scenario[0]))

static struct {
  bool has_address;
  uint32_t target;
  uint16_t next_tid;
  size_t next_step;
  operation queue[MAX_OPERATIONS];
  size_t queued;
  bool waiting;         /* queue[0] was sent and waits for its reply */
  uint16_t tid;
  double deadline;      /* Of the reply when waiting, else when queue[0] may be sent */
  uint32_t last_state;
} ground = { .last_state = UINT32_MAX };

static void print_hex(const char *direction, const uint8_t *packet, size_t size)
{
  size_t i;

  printf(COLOR_GROUND "GND  %s %2zu bytes:", direction, size);
  for (i = 0; i < size; ++i) {
    printf(" %02X", packet[i]);
  }
  printf(COLOR_RESET "\n");
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

/* Builds a command header and returns its size. */
static size_t build_header(uint8_t *packet, uint8_t instruction, uint8_t key,
                           const uint8_t *reply_address, uint32_t address, uint32_t length)
{
  size_t reply_size = (size_t) (instruction & 3) * 4;
  size_t tail = reply_size;
  uint16_t tid = ground.next_tid++;

  packet[0] = NODE_ONBOARD;
  packet[1] = PROTOCOL_RMAP;
  packet[2] = instruction;
  packet[3] = key;
  if (reply_size > 0) {
    memcpy(&packet[4], reply_address, reply_size);
  }
  packet[tail + 4] = NODE_GROUND;
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

static size_t add_data(uint8_t *packet, size_t header_size, const uint8_t *data, size_t size)
{
  memcpy(&packet[header_size], data, size);
  packet[header_size + size] = crc8(data, size);

  return header_size + size + 1;
}

/* Sends queue[0] if nothing is in progress. */
static void send_next(void)
{
  uint8_t packet[COMMAND_HEADER_SIZE + STATE_SIZE + 1];
  const operation *op = &ground.queue[0];
  size_t size;

  if (ground.waiting || ground.queued == 0 || simu_now() < ground.deadline) {
    return;
  }

  ground.tid = ground.next_tid;
  if (op->write) {
    uint8_t value[STATE_SIZE];

    put_be32(value, op->value);
    size = build_header(packet, INS_VERIFY_ACK_INC, RMAP_KEY, NULL, ground.target, STATE_SIZE);
    size = add_data(packet, size, value, sizeof(value));
    printf(COLOR_GROUND "GND  RMAP write %s (%u) at 0x%08X" COLOR_RESET "\n",
           state_name(op->value), (unsigned) op->value, (unsigned) ground.target);
  } else {
    size = build_header(packet, INS_READ_INC, RMAP_KEY, NULL, ground.target, STATE_SIZE);
    printf(COLOR_GROUND "GND  RMAP read at 0x%08X" COLOR_RESET "\n", (unsigned) ground.target);
  }

  print_hex("->", packet, size);
  link_send_packet(CORE_GROUND, packet, (uint32_t) size);

  ground.waiting = true;
  ground.deadline = simu_now() + REPLY_TIMEOUT_S;
}

static void push(bool write, uint32_t value)
{
  if (ground.queued < MAX_OPERATIONS) {
    ground.queue[ground.queued].write = write;
    ground.queue[ground.queued].value = value;
    ++ground.queued;
  }
  send_next();
}

/* Removes queue[0] and sends the next command, a readback after a delay. */
static void pop(void)
{
  bool was_write = ground.queued > 0 && ground.queue[0].write;

  ground.waiting = false;
  ground.deadline = was_write ? simu_now() + READBACK_DELAY_S : 0.0;
  if (ground.queued > 0) {
    memmove(&ground.queue[0], &ground.queue[1], --ground.queued * sizeof(ground.queue[0]));
  }
  send_next();
}

static void handle_reply(const uint8_t *reply, size_t size)
{
  const operation *op = &ground.queue[0];
  size_t header_size = op->write ? WRITE_REPLY_HEADER : READ_REPLY_HEADER;
  uint16_t tid;

  print_hex("<-", reply, size);

  if (!ground.waiting) {
    printf(COLOR_ERROR "GND  unexpected RMAP reply" COLOR_RESET "\n");
    return;
  }

  tid = (uint16_t) (size >= 7 ? reply[5] << 8 | reply[6] : 0);
  if (size < header_size || crc8(reply, header_size) != 0 || tid != ground.tid) {
    printf(COLOR_ERROR "GND  invalid RMAP reply" COLOR_RESET "\n");
  } else if (reply[3] != 0) {
    printf(COLOR_ERROR "GND  RMAP error, status %u" COLOR_RESET "\n", reply[3]);
  } else if (op->write) {
    printf(COLOR_GROUND "GND  write acknowledged" COLOR_RESET "\n");
  } else if (size < READ_REPLY_HEADER + STATE_SIZE + 1
             || crc8(&reply[READ_REPLY_HEADER], STATE_SIZE + 1) != 0) {
    printf(COLOR_ERROR "GND  invalid data in the read reply" COLOR_RESET "\n");
  } else {
    uint32_t state = get_be32(&reply[READ_REPLY_HEADER]);

    printf(COLOR_GROUND "GND  onboard state: %s%s" COLOR_RESET "\n", state_name(state),
           ground.last_state != UINT32_MAX && state != ground.last_state ? "  (changed)" : "");
    ground.last_state = state;
  }

  pop();
}

void ground_init(void)
{
  ground.next_step = 0;
}

void ground_receive(int is_rmap_reply, const uint8_t *packet, size_t size)
{
  if (is_rmap_reply) {
    handle_reply(packet, size);
    return;
  }

  print_hex("<-", packet, size);
  if (size == ADDRESS_PACKET_SIZE && packet[0] == NODE_GROUND && packet[1] == PROTOCOL_ADDRESS) {
    ground.target = get_be32(&packet[2]);
    ground.has_address = true;
    printf(COLOR_GROUND "GND  state variable of the onboard computer at 0x%08X"
           COLOR_RESET "\n", (unsigned) ground.target);
  } else {
    printf(COLOR_ERROR "GND  unexpected packet" COLOR_RESET "\n");
  }
}

void ground_poll(void)
{
  if (ground.waiting) {
    if (simu_now() >= ground.deadline) {
      printf(COLOR_ERROR "GND  no RMAP reply" COLOR_RESET "\n");
      pop();
    }
  } else {
    send_next();
  }
}

int ground_has_address(void)
{
  return ground.has_address;
}

int ground_is_busy(void)
{
  return ground.queued > 0;
}

double ground_deadline(void)
{
  return ground.queued > 0 ? ground.deadline : 0.0;
}

const char *ground_next_step(void)
{
  return ground.next_step < STEP_COUNT ? scenario[ground.next_step].name : "read the state";
}

void ground_command_fail(void)
{
  if (!ground.has_address) {
    printf(COLOR_ERROR "GND  cannot command FAIL: address of the state unknown" COLOR_RESET "\n");
    return;
  }

  /* Commands not sent yet are dropped, and the scenario ends. */
  ground.queued = ground.waiting ? 1 : 0;
  ground.next_step = STEP_COUNT;

  printf(COLOR_ERROR "GND  --- stop of the demonstration: command FAIL ---" COLOR_RESET "\n");
  push(true, STATE_FAIL);
  push(false, 0);
}

int ground_step(void)
{
  const step *current = ground.next_step < STEP_COUNT ? &scenario[ground.next_step] : NULL;

  printf(COLOR_GROUND "GND  --- %s ---" COLOR_RESET "\n", ground_next_step());
  if (current != NULL && current->write >= 0) {
    push(true, (uint32_t) current->write);
  }
  push(false, 0);

  if (ground.next_step < STEP_COUNT) {
    ++ground.next_step;
  }

  return ground.next_step < STEP_COUNT;
}
