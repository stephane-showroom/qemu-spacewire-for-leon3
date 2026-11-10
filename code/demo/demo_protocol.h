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
 * What the onboard computer, the inertial measurement unit (IMU) and the ground
 * station agree on. Shared by the RTEMS application and the Linux programs.
 *
 * Core 0, IMU -> onboard, DMA only:
 *   [NODE_ONBOARD][PROTOCOL_IMU][sequence, u32][9 x i32]   IMU_PACKET_SIZE bytes
 *   The values are roll, pitch, yaw in millidegrees, the angular rates p, q, r in
 *   millidegrees per second, the accelerations ax, ay, az in mm/s2 (body axes, z
 *   down: the accelerometer reads about -9810 on z in level flight).
 *
 * Core 2, onboard <-> ground:
 *   at startup, onboard -> ground in DMA:
 *   [NODE_GROUND][PROTOCOL_ADDRESS][address of the state variable, u32]
 *   then the ground reads and writes the state variable (u32) with RMAP
 *   commands to NODE_ONBOARD, with the key RMAP_KEY.
 *
 * Every multi-byte field is big-endian, the byte order of the LEON3.
 */

#ifndef DEMO_PROTOCOL_H_
#define DEMO_PROTOCOL_H_

#include <stdint.h>

#define CORE_IMU              0
#define CORE_GROUND           2

#define NODE_ONBOARD          0x42
#define NODE_GROUND           0x67
#define RMAP_KEY              0x20

#define PROTOCOL_ADDRESS      0xF0
#define PROTOCOL_IMU          0xF1

#define ADDRESS_PACKET_SIZE   6

#define IMU_SEQUENCE_OFFSET   2
#define IMU_VALUES_OFFSET     6
#define IMU_VALUE_COUNT       9
#define IMU_PACKET_SIZE       (IMU_VALUES_OFFSET + 4 * IMU_VALUE_COUNT)

enum imu_value {
  IMU_ROLL, IMU_PITCH, IMU_YAW,   /* millidegrees */
  IMU_P, IMU_Q, IMU_R,            /* millidegrees per second */
  IMU_AX, IMU_AY, IMU_AZ,         /* mm/s2 */
};

/* Values of the state variable. Anything else is invalid. */
enum onboard_state {
  STATE_INIT = 0,
  STATE_WARMUP = 1,
  STATE_OPERATIONAL = 2,
  STATE_FAIL = 3,
  STATE_COUNT
};

static inline const char *state_name(uint32_t state)
{
  static const char *const names[STATE_COUNT] = { "INIT", "WARMUP", "OPERATIONAL", "FAIL" };

  return state < STATE_COUNT ? names[state] : "INVALID";
}

/* INIT -> WARMUP -> OPERATIONAL, and FAIL from any state. */
static inline int transition_is_valid(uint32_t from, uint32_t to)
{
  if (from >= STATE_COUNT || to >= STATE_COUNT || from == STATE_FAIL) {
    return 0;
  }

  return to == STATE_FAIL || to == from + 1;
}

static inline uint32_t get_be32(const uint8_t *bytes)
{
  return (uint32_t) bytes[0] << 24 | (uint32_t) bytes[1] << 16
       | (uint32_t) bytes[2] << 8 | bytes[3];
}

static inline void put_be32(uint8_t *bytes, uint32_t value)
{
  bytes[0] = (uint8_t) (value >> 24);
  bytes[1] = (uint8_t) (value >> 16);
  bytes[2] = (uint8_t) (value >> 8);
  bytes[3] = (uint8_t) value;
}

#endif /* DEMO_PROTOCOL_H_ */
