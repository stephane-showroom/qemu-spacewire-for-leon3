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
 * Environment simulator of the demonstration: everything at the other end of
 * the SpaceWire link of the onboard computer.
 *
 * It is the only reader of /spwq and the only writer of /devq. Each message from
 * QEMU is routed by core number: CORE_IMU to the inertial measurement unit
 * (imu.c), CORE_GROUND to the ground station (ground.c). Every DATA packet sent
 * by the onboard computer is acknowledged here, on whatever core, since its
 * transmission only ends in the guest with this acknowledge.
 *
 * The main loop also paces the IMU (one measurement per second) and the scenario
 * of the ground station (one step every STEP_PERIOD_S seconds, or on Enter with
 * -i, then a read of the state every MONITOR_PERIOD_S seconds). On SIGUSR1, sent
 * by demo.sh when the demonstration stops, the ground station commands FAIL.
 *
 *   simu [-i] [--imu-fail N]
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <mqueue.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "grlib_mqgrspw.h"
#include "demo_protocol.h"
#include "simu.h"

#define QUEUE_MSG_COUNT   10    /* Same attributes as grlib_mqgrspw_init() */
#define MSG_HEADER_SIZE   (C_HEADER_IDX + 1 + C_DATA_SIZE)
#define IMU_PERIOD_S      1.0
#define STEP_PERIOD_S     5.0
#define MONITOR_PERIOD_S  5.0
#define FIRST_STEP_S      1.0

static mqd_t from_qemu;
static mqd_t to_qemu;
static volatile sig_atomic_t fail_requested;

static void request_fail(int signal_number)
{
  (void) signal_number;
  fail_requested = 1;
}

static void die(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

double simu_now(void)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  return (double) now.tv_sec + now.tv_nsec / 1e9;
}

static void send_message(int core, uint8_t word, const uint8_t *packet, uint32_t size)
{
  uint8_t message[C_MAXQ_SIZE] = { 0 };

  if (size > C_MAXQ_SIZE - MSG_HEADER_SIZE) {
    fprintf(stderr, "simu: packet too large\n");
    return;
  }

  message[C_SPW_ID_IDX] = (uint8_t) core;
  message[C_PWORD_IDX] = word;
  memcpy(&message[C_HEADER_IDX + 1], &size, sizeof(size));
  if (size > 0) {
    memcpy(&message[MSG_HEADER_SIZE], packet, size);
  }

  if (mq_send(to_qemu, (const char *) message, MSG_HEADER_SIZE + size, 0) != 0) {
    die("mq_send");
  }
}

void link_send_packet(int core, const uint8_t *packet, uint32_t size)
{
  send_message(core, C_MSGQ_DATA, packet, size);
}

/*
 * A message is [core][word][header length][header][data length][data]: the
 * packet is the header followed by the data. Returns its size, or -1.
 */
static ssize_t extract_packet(const uint8_t *message, size_t size, uint8_t *packet)
{
  size_t header_size;
  uint32_t data_size;

  if (size < MSG_HEADER_SIZE) {
    return -1;
  }

  header_size = message[C_HEADER_IDX];
  if (size < MSG_HEADER_SIZE + header_size) {
    return -1;
  }

  memcpy(&data_size, &message[C_HEADER_IDX + 1 + header_size], sizeof(data_size));
  if (size != MSG_HEADER_SIZE + header_size + data_size) {
    return -1;
  }

  memcpy(packet, &message[C_HEADER_IDX + 1], header_size);
  memcpy(&packet[header_size], &message[MSG_HEADER_SIZE + header_size], data_size);

  return (ssize_t) (header_size + data_size);
}

static void dispatch(const uint8_t *message, size_t size)
{
  static uint8_t packet[C_MAXQ_SIZE];
  int core = message[C_SPW_ID_IDX];
  uint8_t word = message[C_PWORD_IDX];
  ssize_t packet_size;

  if (word == C_MSGQ_DATA) {
    send_message(core, C_MSGQ_ACK, NULL, 0);
  }

  if (word != C_MSGQ_DATA && word != C_MSGQ_RMAP) {
    return;
  }

  packet_size = extract_packet(message, size, packet);
  if (packet_size < 0) {
    fprintf(stderr, "simu: dropped a malformed message from core %d\n", core);
    return;
  }

  switch (core) {
  case CORE_GROUND:
    ground_receive(word == C_MSGQ_RMAP, packet, (size_t) packet_size);
    break;
  case CORE_IMU:
    /* The IMU only sends: the packet is acknowledged and dropped. */
    printf(COLOR_ERROR "IMU  unexpected packet from the onboard computer" COLOR_RESET "\n");
    break;
  default:
    printf(COLOR_ERROR "simu: packet on core %d, which is not connected" COLOR_RESET "\n", core);
    break;
  }
}

static void receive_all(void)
{
  for (;;) {
    uint8_t message[C_MAXQ_SIZE];
    ssize_t size = mq_receive(from_qemu, (char *) message, sizeof(message), NULL);

    if (size < 0) {
      if (errno == EAGAIN) {
        return;
      }
      die("mq_receive");
    }

    dispatch(message, (size_t) size);
  }
}

static void usage(const char *name)
{
  fprintf(stderr, "usage: %s [-i] [--imu-fail N]\n"
          "  -i, --interactive  wait for Enter before each step of the ground scenario\n"
          "  --imu-fail N       the IMU stops sending after N seconds\n", name);
  exit(EXIT_FAILURE);
}

static double earliest(double a, double b)
{
  if (a <= 0.0) {
    return b;
  }
  return b <= 0.0 || a < b ? a : b;
}

int main(int argc, char **argv)
{
  static const struct option options[] = {
    { "interactive", no_argument, NULL, 'i' },
    { "imu-fail", required_argument, NULL, 'f' },
    { NULL, 0, NULL, 0 },
  };
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MSG_COUNT,
    .mq_msgsize = C_MAXQ_SIZE,
  };
  bool interactive = false;
  bool scenario_done = false;
  bool prompted = false;
  bool enter = false;
  int imu_fail = -1;
  double next_imu;
  double next_step = 0.0;
  struct sigaction action = { .sa_handler = request_fail };
  int option;

  while ((option = getopt_long(argc, argv, "i", options, NULL)) != -1) {
    switch (option) {
    case 'i':
      interactive = true;
      break;
    case 'f':
      imu_fail = atoi(optarg);
      if (imu_fail < 0) {
        usage(argv[0]);
      }
      break;
    default:
      usage(argv[0]);
    }
  }

  setvbuf(stdout, NULL, _IOLBF, 0);

  /* Without SA_RESTART, so that the signal interrupts poll(). */
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGUSR1, &action, NULL) != 0) {
    die("sigaction");
  }

  from_qemu = mq_open(C_SPWQ, O_CREAT | O_RDONLY | O_NONBLOCK, 0644, &attr);
  if (from_qemu == (mqd_t) -1) {
    die("mq_open " C_SPWQ);
  }

  to_qemu = mq_open(C_DEVQ, O_CREAT | O_WRONLY, 0644, &attr);
  if (to_qemu == (mqd_t) -1) {
    die("mq_open " C_DEVQ);
  }

  imu_init(imu_fail);
  ground_init();
  printf("ENVIRONMENT SIMULATOR: IMU on core %d, ground on core %d%s\n",
         CORE_IMU, CORE_GROUND, interactive ? ", interactive" : "");
  if (imu_fail >= 0) {
    printf("the IMU will stop after %d s\n", imu_fail);
  }
  printf("GND  waiting for the address of the state variable...\n");

  next_imu = simu_now() + IMU_PERIOD_S;

  for (;;) {
    /* On Linux, a message queue descriptor is a file descriptor. */
    struct pollfd fds[2] = {
      { .fd = (int) from_qemu, .events = POLLIN },
      { .fd = STDIN_FILENO, .events = POLLIN },
    };
    bool waits_enter = interactive && !scenario_done;
    double deadline = earliest(earliest(next_imu, waits_enter ? 0.0 : next_step),
                               ground_deadline());
    double now = simu_now();
    int timeout_ms = deadline > now ? (int) ceil((deadline - now) * 1000.0) : 0;

    if (poll(fds, interactive ? 2 : 1, timeout_ms) < 0) {
      if (errno != EINTR) {
        die("poll");
      }
      fds[0].revents = 0;
      fds[1].revents = 0;
    }

    if (fail_requested) {
      fail_requested = 0;
      ground_command_fail();
      scenario_done = true;
      next_step = simu_now() + MONITOR_PERIOD_S;
    }

    if (fds[0].revents & POLLIN) {
      receive_all();
    }

    if (interactive && (fds[1].revents & (POLLIN | POLLHUP))) {
      char line[128];

      if (fgets(line, sizeof(line), stdin) == NULL) {
        interactive = false;    /* End of input: go on without waiting */
      }
      enter = true;
    }

    now = simu_now();

    if (next_imu > 0.0 && now >= next_imu) {
      next_imu = imu_send() ? next_imu + IMU_PERIOD_S : 0.0;
    }

    if (ground_deadline() > 0.0 && now >= ground_deadline()) {
      ground_poll();
    }

    /* Scenario of the ground station, once the address is known. */
    if (!ground_has_address() || ground_is_busy()) {
      continue;
    }

    if (next_step == 0.0) {
      next_step = now + FIRST_STEP_S;
    }

    if (interactive && !scenario_done) {
      if (!prompted) {
        printf(COLOR_GROUND "GND  press Enter to %s" COLOR_RESET "\n", ground_next_step());
        prompted = true;
        enter = false;
      } else if (enter) {
        scenario_done = !ground_step();
        prompted = false;
        next_step = now + MONITOR_PERIOD_S;
      }
    } else if (now >= next_step) {
      bool more = ground_step();

      next_step = now + (more ? STEP_PERIOD_S : MONITOR_PERIOD_S);
      scenario_done = scenario_done || !more;
    }
  }
}
