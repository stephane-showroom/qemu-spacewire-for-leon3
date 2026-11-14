#!/usr/bin/env bash
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

# Runs every test and prints one result per line. Logs are in build/logs.
#
#   run_tests.sh QEMU RAM_MB
#
# ONLY=<pattern> runs only the tests whose name matches the shell pattern, for
# example ONLY='REQ_GRSPW_03*'.
#
# PASS   the test succeeded
# FAIL   the test failed
# XFAIL  the test failed because of a known bug
# XPASS  a known bug seems fixed: remove its "known" mark below
#
# Exits with 0 when there is neither FAIL nor XPASS.
set -uo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

qemu=$1 ram_mb=$2
timeout_seconds=10
logs=build/logs
failures=0

device_simu=../device_simu/build/device_simu
spw_echo=../rtems_spw_echo/build/spw_echo.exe

mkdir -p "$logs"

remove_queues() {
    rm -f /dev/mqueue/spwq /dev/mqueue/devq
}

report() {
    local name=$1 status=$2 expectation=$3 detail=$4 result

    case "$status/$expectation" in
        passed/expected) result=PASS ;;
        failed/known)    result=XFAIL ;;
        passed/known)    result=XPASS; failures=$((failures + 1)) ;;
        *)               result=FAIL; failures=$((failures + 1)) ;;
    esac

    printf '%-6s %-28s %s\n' "$result" "$name" "$detail"
}

# run_guest NAME EXECUTABLE PEER CONSOLE_PATTERN expected|known [DESCRIPTION]
run_guest() {
    local name=$1 app=$2 peer=$3 pattern=$4 expectation=$5 detail=${6:-}
    local log="$logs/$name.log" peer_pid="" code status=failed

    # shellcheck disable=SC2053
    if [ -n "${ONLY:-}" ] && [[ $name != $ONLY ]]; then
        return
    fi

    remove_queues
    if [ -n "$peer" ]; then
        # The peer is a command with its arguments, hence no quotes.
        $peer >"$logs/$name.peer.log" 2>&1 &
        peer_pid=$!
    fi

    # Without an acknowledge QEMU blocks and ignores SIGTERM, hence the -k.
    (
        timeout -k 1 "$timeout_seconds" "$qemu" -M leon3_generic -m "$ram_mb" \
            -display none -serial stdio -no-reboot -kernel "$app"
        echo $? >"$log.code"
    ) 2>&1 | grep -av '^trace debug' >"$log"
    code=$(<"$log.code")

    if [ -n "$peer_pid" ]; then
        kill "$peer_pid" 2>/dev/null
        wait "$peer_pid" 2>/dev/null
    fi
    remove_queues

    if grep -aq "$pattern" "$log"; then
        status=passed
    elif [ "$code" -ne 0 ]; then
        detail="$detail [QEMU exit code $code]"
    fi

    report "$name" "$status" "$expectation" "$detail"
}

run_queue_test() {
    local name=device_simu_queues simu_pid status=failed

    # shellcheck disable=SC2053
    if [ -n "${ONLY:-}" ] && [[ $name != $ONLY ]]; then
        return
    fi

    remove_queues
    "$device_simu" >"$logs/$name.peer.log" 2>&1 &
    simu_pid=$!

    if build/queue_test >"$logs/$name.log" 2>&1; then
        status=passed
    fi

    kill "$simu_pid" 2>/dev/null
    wait "$simu_pid" 2>/dev/null
    remove_queues

    report "$name" "$status" expected ""
}

ok='^TEST OK'

run_guest hello ../rtems_hello/build/hello.exe "" '^Hello World' expected
run_guest echo_pkt_no_peer "$spw_echo" "" '^SPW ECHO OK' known \
    "no peer: the packet is never acknowledged, by design"
run_queue_test
run_guest all_cores build/raw_all_cores.0.exe "$device_simu" "$ok" expected

for core in 0 1 2 3; do
    run_guest "echo_pkt.$core" "build/echo.$core.exe" "$device_simu" '^SPW ECHO OK' expected
    run_guest "pkt_irq.$core" "build/pkt_irq.$core.exe" "$device_simu" "$ok" expected

    for scenario in irq_rx_blocking many_packets stop_start clear_ctrl_bit max_packet; do
        run_guest "$scenario.$core" "build/legacy_$scenario.$core.exe" "$device_simu" "$ok" expected
    done
    run_guest "data_before_ack.$core" "build/legacy_data_before_ack.$core.exe" \
        build/peer_data_before_ack "$ok" expected

    for scenario in reset_values rx_ri_only tx_burst rx_no_spill invalid_address header; do
        run_guest "$scenario.$core" "build/raw_$scenario.$core.exe" "$device_simu" "$ok" expected
    done
    run_guest "time_codes.$core" "build/raw_time_codes.$core.exe" build/peer_time_codes "$ok" expected

    if [ "$core" -ge 2 ]; then
        run_guest "tx_crc.$core" "build/raw_tx_crc.$core.exe" "$device_simu" "$ok" expected
        run_guest "rmap_disabled.$core" "build/raw_rmap_disabled.$core.exe" "$device_simu" "$ok" expected
        run_guest "rmap.$core" "build/raw_rmap.$core.exe" build/rmap_test "$ok" expected
    else
        run_guest "no_rmap.$core" "build/raw_no_rmap.$core.exe" "$device_simu" "$ok" expected
    fi
done

# Prints the value of the "@<field>" lines of the header comment of a req/ file.
req_field() {
    sed -n "s/^ \* @$2 *//p" "$1"
}

# One test per requirement, on each core of its "@cores" line. A "@known
# <cores>: <reason>" line marks a known deviation of the model from the
# specification on these cores ("all" for every core).
run_requirements() {
    local file name side core peer expectation detail known_cores line

    for file in req/REQ_GRSPW_*.c; do
        name=$(basename "$file" .c)
        side=$(req_field "$file" side)
        for core in $(req_field "$file" cores); do
            expectation=expected
            detail=""
            while IFS= read -r line; do
                known_cores=${line%%:*}
                if [ "$known_cores" = all ] || [[ " $known_cores " == *" $core "* ]]; then
                    expectation=known
                    detail=${line#*: }
                fi
            done < <(req_field "$file" known)

            if [ "$side" = peer ]; then
                run_guest "$name.$core" "build/req/peer_guest.$core.exe" "build/req/$name.peer" \
                    "$ok" "$expectation" "$detail"
                continue
            fi

            peer=$(req_field "$file" peer)
            case $peer in
                none)          peer="" ;;
                device_simu)   peer=$device_simu ;;
                peer_script*)  peer="build/peer_script ${peer#peer_script}" ;;
            esac
            run_guest "$name.$core" "build/req/$name.$core.exe" "$peer" "$ok" "$expectation" "$detail"
        done
    done
}

run_requirements

if [ "$failures" -ne 0 ]; then
    echo "$failures unexpected result(s), see $logs" >&2
    exit 1
fi
