# SPDX-License-Identifier: GPL-2.0-or-later
#################################################################################
#                                                                               #
#     ▀▄   ▄▀                                                                   #
#    ▄█▀███▀█▄    Created on: Nov 6, 2026                                       #
#   █▀███████▀█   Author: Stephane GONZALEZ                                     #
#   █ █▀▀▀▀▀█ █                                                                 #
#      ▀▀ ▀▀                                                                    #
#                                                                               #
#  Copyright (C) 2015-2026 Stephane GONZALEZ                                    #
#  Released under the GNU GPL v2 or later, see LICENSE                          #
#################################################################################

# Builds and runs the tests of the RTEMS applications, device_simu and the GRSPW
# device. QEMU must contain the device: see code/integrate_qemu.sh.

RTEMS_PREFIX ?= /opt/rtems/6
TARGET       ?= sparc-rtems6
BSP          ?= leon3
QEMU         ?= /workspace/qemu/build/qemu-system-sparc
RAM_MB       ?= 64

PKG      := PKG_CONFIG_PATH=$(RTEMS_PREFIX)/lib/pkgconfig pkg-config $(TARGET)-$(BSP)
RTEMS_CC := $(RTEMS_PREFIX)/bin/$(TARGET)-gcc
HOST_CC  ?= gcc

CFLAGS      := $(shell $(PKG) --cflags) -O2 -g -std=gnu11 -Wall -Wextra -ffunction-sections -fdata-sections
ASFLAGS     := $(shell $(PKG) --cflags) -g -DRAM_END='(0x40000000 + $(RAM_MB) * 1024 * 1024)'
LDFLAGS     := $(shell $(PKG) --libs) -Wl,-e,qemu_boot
HOST_CFLAGS := -O2 -g -Wall -Wextra -I../leon3_grspw_v1

BOOT_SRC   := ../rtems_hello/qemu_boot.S
ECHO_DIR   := ../rtems_spw_echo
CORES      := 0 1 2 3
RMAP_CORES := 2 3
NO_RMAP_CORES := 0 1

# Executables are named <prefix>_<scenario>.<core>.exe
LEGACY_SCENARIOS := irq_rx_blocking many_packets stop_start clear_ctrl_bit max_packet data_before_ack
RAW_SCENARIOS := reset_values rx_ri_only tx_burst rx_no_spill invalid_address header time_codes
RMAP_SCENARIOS := tx_crc rmap rmap_disabled
NO_RMAP_SCENARIOS := no_rmap

for_cores = $(foreach core,$(2),$(foreach scenario,$(3),build/$(1)_$(scenario).$(core).exe))

LEGACY_EXE := $(call for_cores,legacy,$(CORES),$(LEGACY_SCENARIOS))
RAW_EXE    := $(call for_cores,raw,$(CORES),$(RAW_SCENARIOS)) \
              $(call for_cores,raw,$(RMAP_CORES),$(RMAP_SCENARIOS)) \
              $(call for_cores,raw,$(NO_RMAP_CORES),$(NO_RMAP_SCENARIOS)) \
              build/raw_all_cores.0.exe
PKT_EXE    := $(CORES:%=build/pkt_irq.%.exe)
ECHO_EXE   := $(CORES:%=build/echo.%.exe)

# One test per requirement in req/, described by the "@" lines of its header
# comment: "@side guest" files run in the guest on each core of "@cores",
# "@side peer" files are Linux peers run against the guest req/peer_guest.c.
REQ_SOURCES   := $(sort $(wildcard req/REQ_GRSPW_*.c))
req_field      = $(strip $(shell sed -n 's/^ \* @$(2) *//p' $(1)))
REQ_GUEST     := $(foreach f,$(REQ_SOURCES),$(if $(filter guest,$(call req_field,$(f),side)),$(f)))
REQ_PEER      := $(filter-out $(REQ_GUEST),$(REQ_SOURCES))
REQ_GUEST_EXE := $(foreach f,$(REQ_GUEST),$(foreach c,$(call req_field,$(f),cores),build/req/$(basename $(notdir $(f))).$(c).exe))
REQ_PEER_EXE  := $(REQ_PEER:req/%.c=build/req/%.peer) $(CORES:%=build/req/peer_guest.%.exe)

scenario = $(basename $*)
core     = $(subst .,,$(suffix $*))

all: $(LEGACY_EXE) $(RAW_EXE) $(PKT_EXE) $(ECHO_EXE) build/queue_test build/rmap_test \
     build/peer_data_before_ack build/peer_time_codes build/peer_script \
     $(REQ_GUEST_EXE) $(REQ_PEER_EXE)
	$(MAKE) -C ../rtems_hello
	$(MAKE) -C $(ECHO_DIR)
	$(MAKE) -C ../device_simu

build/qemu_boot.o: $(BOOT_SRC) Makefile | build
	$(RTEMS_CC) $(ASFLAGS) -c $< -o $@

build/legacy_%.exe: grspw_legacy.c build/qemu_boot.o | build
	$(RTEMS_CC) $(CFLAGS) -DSCENARIO_$(scenario) -DCORE=$(core) -c $< -o build/legacy_$*.o
	$(RTEMS_CC) build/legacy_$*.o build/qemu_boot.o $(LDFLAGS) -o $@

build/raw_%.exe: grspw_raw.c build/qemu_boot.o | build
	$(RTEMS_CC) $(CFLAGS) -DSCENARIO_$(scenario) -DCORE=$(core) -c $< -o build/raw_$*.o
	$(RTEMS_CC) build/raw_$*.o build/qemu_boot.o $(LDFLAGS) -o $@

build/pkt_irq.%.exe: grspw_pkt_irq.c build/qemu_boot.o | build
	$(RTEMS_CC) $(CFLAGS) -DCORE=$* -c $< -o build/pkt_irq.$*.o
	$(RTEMS_CC) build/pkt_irq.$*.o build/qemu_boot.o $(LDFLAGS) -o $@

build/echo.%.exe: $(ECHO_DIR)/init.c build/qemu_boot.o | build
	$(RTEMS_CC) $(CFLAGS) -DSPW_CORE=$* -c $< -o build/echo.$*.o
	$(RTEMS_CC) build/echo.$*.o build/qemu_boot.o $(LDFLAGS) -o $@

# build/req/<file>.<core>.exe from req/<file>.c
.SECONDEXPANSION:
build/req/%.exe: req/$$(basename $$*).c req/req_guest.h build/qemu_boot.o | build/req
	$(RTEMS_CC) $(CFLAGS) -Ireq -DCORE=$(core) -c $< -o build/req/$*.o
	$(RTEMS_CC) build/req/$*.o build/qemu_boot.o $(LDFLAGS) -o $@

build/req/%.peer: req/%.c req/req_peer.h | build/req
	$(HOST_CC) $(HOST_CFLAGS) -Ireq $< -lrt -o $@

build/queue_test: queue_test.c | build
	$(HOST_CC) $(HOST_CFLAGS) $< -lrt -o $@

build/rmap_test: rmap_test.c | build
	$(HOST_CC) $(HOST_CFLAGS) $< -lrt -o $@

build/peer_%: peer_%.c | build
	$(HOST_CC) $(HOST_CFLAGS) $< -lrt -o $@

build build/req:
	mkdir -p $@

test: all
	./run_tests.sh $(QEMU) $(RAM_MB)

clean:
	rm -rf build
	$(MAKE) -C ../rtems_hello clean
	$(MAKE) -C $(ECHO_DIR) clean
	$(MAKE) -C ../device_simu clean

.PHONY: all test clean
