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
 * Environment simulator of the demonstration: what simu.c, which owns the
 * message queues, offers to its modules, and what the modules offer to the
 * main loop.
 */

#ifndef SIMU_H_
#define SIMU_H_

#include <stddef.h>
#include <stdint.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_IMU     "\033[36m"
#define COLOR_GROUND  "\033[1;32m"
#define COLOR_ERROR   "\033[1;31m"

/* simu.c: sends one SpaceWire packet (DATA) to the onboard computer. */
void link_send_packet(int core, const uint8_t *packet, uint32_t size);

/* imu.c: inertial measurement unit on CORE_IMU. */
void imu_init(int fail_after_s);
/* Sends the next measurement. Returns 0 once the IMU has stopped (--imu-fail). */
int imu_send(void);

/* ground.c: ground station on CORE_GROUND. */
void ground_init(void);
/* A DATA packet or an RMAP reply received on CORE_GROUND. */
void ground_receive(int is_rmap_reply, const uint8_t *packet, size_t size);
/* Called at ground_deadline(): reply timeout, or delayed command to send. */
void ground_poll(void);
int ground_has_address(void);
/* An RMAP command is waiting for its reply, or is about to be sent. */
int ground_is_busy(void);
/* Next time ground_poll() has something to do, in seconds of simu_now(), or 0. */
double ground_deadline(void);
/* Name of the step ground_step() will run. */
const char *ground_next_step(void);
/* Runs the next step of the scenario. Returns 0 when it was the last one. */
int ground_step(void);
/* Commands FAIL and ends the scenario. */
void ground_command_fail(void);

/* simu.c: monotonic time in seconds. */
double simu_now(void);

#endif /* SIMU_H_ */
