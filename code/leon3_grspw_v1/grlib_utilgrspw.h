/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Jul 5, 2015                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

#ifndef GRLIB_UTILGRSPW_H_
#define GRLIB_UTILGRSPW_H_

#include "exec/hwaddr.h"

#include "grlib_typesgrspw.h"

void endianness_swap(uint8 dest[], const uint8 src[], const uint32 len);
boolean read_leon3_ram(const hwaddr addr, uint8 buff[], const uint32 len, const boolean sawp_endian);
boolean write_leon3_ram(const hwaddr addr, const uint8 buff[], const uint32 len, const boolean sawp_endian);

boolean checkbit(uint32 reg, uint32 mask);
uint32  setbit(uint32 reg, uint32 value);
uint32  clearbit(uint32 reg, uint32 value);
uint32  deposit(uint32 reg, uint32 value, uint32 mask);

#endif /* GRLIB_UTILGRSPW_H_ */
