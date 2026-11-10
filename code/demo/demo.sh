#!/usr/bin/env bash
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

# Builds the demonstration and runs it in tmux: the onboard computer in QEMU on
# the left, the environment simulator (IMU and ground station) on the right.
# The options are passed to the simulator.
#
#   demo.sh [-i] [--imu-fail N]     start the demonstration
#   demo.sh shutdown                the ground commands FAIL, then stop after 5 s
#   demo.sh stop                    stop at once
#
# In the tmux session, Escape runs shutdown and Ctrl-C runs stop. The session has
# its own tmux server, so these keys do not change any other tmux session.
#
# QEMU must contain the GRSPW device: see code/integrate_qemu.sh.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
demo_dir=$(pwd)

session=spw-demo
qemu=${QEMU:-/workspace/qemu/build/qemu-system-sparc}
ram_mb=${RAM_MB:-64}
shutdown_delay_s=5

# Own server: it works from inside another tmux, and its key bindings stay here.
demo_tmux() {
    TMUX='' tmux -L "$session" "$@"
}

remove_queues() {
    rm -f /dev/mqueue/spwq /dev/mqueue/devq
}

# signal_executable SIGNAL EXECUTABLE: signals the processes of this executable only.
signal_executable() {
    local pid

    for pid in $(pgrep -f -- "$2" || true); do
        if [ "$(readlink -f "/proc/$pid/exe" 2>/dev/null)" = "$(readlink -f "$2")" ]; then
            kill "-$1" "$pid" 2>/dev/null || true
        fi
    done
}

stop() {
    # QEMU ignores SIGTERM while it waits for an acknowledge, hence SIGKILL.
    signal_executable KILL "$qemu"
    signal_executable KILL "$demo_dir/simu/build/simu"
    remove_queues
    # Last: when it runs from the Ctrl-C binding, this ends the script too.
    demo_tmux kill-server 2>/dev/null || true
}

# The ground station commands FAIL on SIGUSR1: let the onboard computer show it.
shutdown() {
    signal_executable USR1 "$demo_dir/simu/build/simu"
    demo_tmux display-message -d $((shutdown_delay_s * 1000)) \
        "FAIL commanded by the ground, the demo stops in $shutdown_delay_s s" 2>/dev/null || true
    sleep "$shutdown_delay_s"
    stop
}

case "${1:-}" in
    shutdown)
        shutdown
        exit 0
        ;;
    stop|--stop)
        stop
        exit 0
        ;;
esac

if ! command -v tmux >/dev/null; then
    echo "demo.sh: tmux is not installed. Install it, or run in two terminals:" >&2
    echo "  $demo_dir/simu/build/simu $*" >&2
    echo "  make -C $demo_dir/onboard run" >&2
    exit 1
fi

if demo_tmux has-session -t "$session" 2>/dev/null; then
    echo "demo.sh: the demonstration is already running." >&2
    echo "  attach: tmux -L $session attach    stop: $0 stop" >&2
    exit 1
fi

make -s -C onboard RAM_MB="$ram_mb"
make -s -C simu

# Stale messages from a previous run would be taken for acknowledges.
remove_queues

simu_command="$demo_dir/simu/build/simu"
for arg in "$@"; do
    simu_command+=" $(printf '%q' "$arg")"
done

# The simulator creates the queues and must listen before the onboard computer
# sends the address of its state variable.
onboard_command="sleep 1; $(printf '%q' "$qemu") -M leon3_generic -m $ram_mb"
onboard_command+=" -display none -serial stdio -no-reboot -kernel $demo_dir/onboard/build/onboard.exe"
onboard_command+=" 2>&1 | grep --line-buffered -av '^trace debug'"

demo_script=$(printf '%q' "$demo_dir/demo.sh")

# A pane whose program ends stays open, to keep its output, until Ctrl-C.
demo_tmux -f /dev/null new-session -d -s "$session" -x "$(tput cols 2>/dev/null || echo 200)" \
    -y "$(tput lines 2>/dev/null || echo 50)" -n demo "$onboard_command; read -r"
demo_tmux split-window -h -t "$session:demo" "$simu_command; read -r"
demo_tmux set-option -g mouse on
demo_tmux set-option -g status-right " Esc: FAIL then stop | Ctrl-C: stop now "
demo_tmux set-option -g status-right-length 60
demo_tmux set-option -g pane-border-status top
# Escape alone, without waiting for the rest of an escape sequence.
demo_tmux set-option -s escape-time 10
demo_tmux bind-key -n Escape run-shell -b "$demo_script shutdown"
demo_tmux bind-key -n C-c run-shell "$demo_script stop"
demo_tmux select-pane -t "$session:demo.0" -T "onboard computer (QEMU)"
demo_tmux select-pane -t "$session:demo.1" -T "environment simulator"
# The ground station reads Enter in -i mode: start in its pane.
demo_tmux select-pane -t "$session:demo.1"

demo_tmux attach -t "$session" || true
stop
