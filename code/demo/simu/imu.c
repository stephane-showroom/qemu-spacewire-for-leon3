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
 * Inertial measurement unit (IMU) of the demonstration, on CORE_IMU.
 *
 * The aircraft flies at 70 m/s, banks slowly from one side to the other in
 * coordinated turns and pitches gently around a slight climb. The angular rates
 * are derived from the Euler angles, and the accelerometers measure the specific
 * force in body axes (z down), with a small noise. The main loop calls
 * imu_send() once per second.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "demo_protocol.h"
#include "simu.h"

#define GRAVITY        9.80665    /* m/s2 */
#define AIRSPEED       70.0       /* m/s */
#define ROLL_MAX       25.0       /* deg */
#define ROLL_PERIOD    60.0       /* s */
#define PITCH_TRIM     3.0        /* deg */
#define PITCH_MAX      2.0        /* deg */
#define PITCH_PERIOD   45.0       /* s */
#define INITIAL_YAW    90.0       /* deg */
#define RATE_NOISE     0.05       /* deg/s */
#define ACCEL_NOISE    0.03       /* m/s2 */

#define DEG            (M_PI / 180.0)

static struct {
  uint32_t sequence;
  int fail_after;       /* Measurements before the failure, -1 for never */
  double yaw;           /* rad */
} imu;

/* Uniform noise in [-amplitude, amplitude]: enough for a demonstration. */
static double noise(double amplitude)
{
  return amplitude * (2.0 * rand() / RAND_MAX - 1.0);
}

static int32_t milli(double value)
{
  return (int32_t) lround(value * 1000.0);
}

void imu_init(int fail_after_s)
{
  imu.sequence = 0;
  imu.fail_after = fail_after_s;
  imu.yaw = INITIAL_YAW * DEG;
  srand(1);
}

int imu_send(void)
{
  double t = imu.sequence;
  double w_roll = 2.0 * M_PI / ROLL_PERIOD;
  double w_pitch = 2.0 * M_PI / PITCH_PERIOD;
  double roll = ROLL_MAX * DEG * sin(w_roll * t);
  double pitch = (PITCH_TRIM + PITCH_MAX * sin(w_pitch * t)) * DEG;
  double roll_dot = ROLL_MAX * DEG * w_roll * cos(w_roll * t);
  double pitch_dot = PITCH_MAX * DEG * w_pitch * cos(w_pitch * t);
  double yaw_dot = GRAVITY * tan(roll) / AIRSPEED;   /* Coordinated turn */
  double value[IMU_VALUE_COUNT];
  uint8_t packet[IMU_PACKET_SIZE];
  int i;

  if (imu.fail_after >= 0 && imu.sequence >= (uint32_t) imu.fail_after) {
    if (imu.sequence == (uint32_t) imu.fail_after) {
      printf(COLOR_ERROR "IMU  failure: no more data after %d s" COLOR_RESET "\n",
             imu.fail_after);
      ++imu.sequence;
    }
    return 0;
  }

  /* Body rates from the Euler angle rates. */
  value[IMU_ROLL] = roll / DEG;
  value[IMU_PITCH] = pitch / DEG;
  value[IMU_YAW] = fmod(imu.yaw / DEG + 360.0, 360.0);
  value[IMU_P] = (roll_dot - yaw_dot * sin(pitch)) / DEG + noise(RATE_NOISE);
  value[IMU_Q] = (pitch_dot * cos(roll) + yaw_dot * sin(roll) * cos(pitch)) / DEG
               + noise(RATE_NOISE);
  value[IMU_R] = (-pitch_dot * sin(roll) + yaw_dot * cos(roll) * cos(pitch)) / DEG
               + noise(RATE_NOISE);

  /* Specific force in a coordinated turn: the load factor is 1 / cos(roll). */
  value[IMU_AX] = GRAVITY * sin(pitch) + noise(ACCEL_NOISE);
  value[IMU_AY] = noise(ACCEL_NOISE);
  value[IMU_AZ] = -GRAVITY * cos(pitch) / cos(roll) + noise(ACCEL_NOISE);

  packet[0] = NODE_ONBOARD;
  packet[1] = PROTOCOL_IMU;
  put_be32(&packet[IMU_SEQUENCE_OFFSET], imu.sequence);
  for (i = 0; i < IMU_VALUE_COUNT; ++i) {
    put_be32(&packet[IMU_VALUES_OFFSET + 4 * i], (uint32_t) milli(value[i]));
  }

  link_send_packet(CORE_IMU, packet, sizeof(packet));

  printf(COLOR_IMU "IMU  #%-4u att %6.1f %5.1f %5.1f  rate %5.2f %5.2f %5.2f  acc %5.2f %5.2f %6.2f"
         COLOR_RESET "\n", (unsigned) imu.sequence,
         value[IMU_ROLL], value[IMU_PITCH], value[IMU_YAW],
         value[IMU_P], value[IMU_Q], value[IMU_R],
         value[IMU_AX], value[IMU_AY], value[IMU_AZ]);

  imu.yaw += yaw_dot;    /* One second to the next measurement */
  ++imu.sequence;

  return 1;
}
