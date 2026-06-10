#!/usr/bin/env bash
# Block until an Adafruit Feather USB serial port appears.
# Echoes the discovered port to stdout. Exits non-zero on timeout.
# Usage:  PORT=$(scripts/_wait_for_board.sh)
# Env:    BOARD_PORT_GLOB (default /dev/cu.usbmodem*), BOARD_TIMEOUT_S (default 120)
set -euo pipefail

PORT_GLOB="${BOARD_PORT_GLOB:-/dev/cu.usbmodem*}"
TIMEOUT_S="${BOARD_TIMEOUT_S:-120}"

waited=0
echo "Waiting for board on ${PORT_GLOB}..." >&2
until compgen -G "$PORT_GLOB" >/dev/null; do
    if (( waited >= TIMEOUT_S )); then
        echo "Timed out after ${TIMEOUT_S}s waiting for ${PORT_GLOB}" >&2
        exit 1
    fi
    sleep 1
    waited=$((waited + 1))
done

compgen -G "$PORT_GLOB" | head -n1
