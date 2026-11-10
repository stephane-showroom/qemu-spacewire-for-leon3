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

# Builds the onboard computer of the demonstration for the RTEMS sparc/leon3 BSP
# and runs it in QEMU. Start the simulator first: see ../demo.sh.
#
# QEMU must contain the GRSPW device: see code/integrate_qemu.sh.

RTEMS_PREFIX ?= /opt/rtems/6
TARGET       ?= sparc-rtems6
BSP          ?= leon3
QEMU         ?= /workspace/qemu/build/qemu-system-sparc
RAM_MB       ?= 64

PKG := PKG_CONFIG_PATH=$(RTEMS_PREFIX)/lib/pkgconfig pkg-config $(TARGET)-$(BSP)
CC  := $(RTEMS_PREFIX)/bin/$(TARGET)-gcc

CFLAGS  := $(shell $(PKG) --cflags) -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections -I..
ASFLAGS := $(shell $(PKG) --cflags) -g -DRAM_END='(0x40000000 + $(RAM_MB) * 1024 * 1024)'
# Start from qemu_boot.S instead of the BSP's `start`: see the comment in that file.
LDFLAGS := $(shell $(PKG) --libs) -Wl,-e,qemu_boot

BOOT_SRC := ../../rtems_hello/qemu_boot.S
APP      := build/onboard.exe

all: $(APP)

$(APP): build/init.o build/qemu_boot.o
	$(CC) $^ $(LDFLAGS) -o $@

build/init.o: init.c ../demo_protocol.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/qemu_boot.o: $(BOOT_SRC) Makefile | build
	$(CC) $(ASFLAGS) -c $< -o $@

build:
	mkdir -p $@

run: all
	$(QEMU) -M leon3_generic -m $(RAM_MB) -display none -serial stdio -no-reboot \
	    -kernel $(APP) 2>&1 | grep --line-buffered -av '^trace debug'

clean:
	rm -rf build

.PHONY: all run clean
