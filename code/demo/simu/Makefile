# SPDX-License-Identifier: GPL-2.0-or-later
#################################################################################
#                                                                               #
#     ▀▄   ▄▀                                                                   #
#    ▄█▀███▀█▄    Created on: Nov 10, 2026                                      #
#   █▀███████▀█   Author: Stephane GONZALEZ                                     #
#   █ █▀▀▀▀▀█ █                                                                 #
#      ▀▀ ▀▀                                                                    #
#                                                                               #
#  Copyright (C) 2015-2026 Stephane GONZALEZ                                    #
#  Released under the GNU GPL v2 or later, see LICENSE                          #
#################################################################################

# Builds the environment simulator of the demonstration for the host:
# the inertial measurement unit and the ground station.

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra
CFLAGS  += -I.. -I../../leon3_grspw_v1
LDLIBS  := -lrt -lm

APP  := build/simu
SRCS := simu.c imu.c ground.c
DEPS := simu.h ../demo_protocol.h ../../leon3_grspw_v1/grlib_mqgrspw.h ../../leon3_grspw_v1/grlib_typesgrspw.h

all: $(APP)

$(APP): $(SRCS) $(DEPS) | build
	$(CC) $(CFLAGS) $(SRCS) $(LDLIBS) -o $@

build:
	mkdir -p $@

run: $(APP)
	./$(APP) $(ARGS)

clean:
	rm -rf build

.PHONY: all run clean
