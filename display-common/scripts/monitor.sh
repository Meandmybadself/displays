#!/usr/bin/env bash
# Wait for the Feather, then open the serial monitor for the project in the CURRENT
# directory (see flash.sh for why we don't cd).
# Usage: scripts/monitor.sh [extra pio args...]
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

PORT=$("$HERE/_wait_for_board.sh")
echo "Connecting to ${PORT} @115200..."
exec pio device monitor -p "$PORT" -b 115200 "$@"
