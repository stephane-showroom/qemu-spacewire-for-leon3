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

# Builds the demonstration: the onboard computer (RTEMS) and the environment
# simulator (Linux). `make run` starts it in tmux with demo.sh.

all:
	$(MAKE) -C onboard
	$(MAKE) -C simu

run:
	./demo.sh $(ARGS)

shutdown:
	./demo.sh shutdown

stop:
	./demo.sh stop

clean:
	$(MAKE) -C onboard clean
	$(MAKE) -C simu clean

.PHONY: all run shutdown stop clean
