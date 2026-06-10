#!/usr/bin/env bash
# Wait for the Feather, then build + flash the project in the CURRENT directory.
# Apps call this through a thin scripts/flash.sh wrapper, so it must run pio in the
# caller's cwd (the app project), not in display-common.
# Usage: scripts/flash.sh [extra pio args...]
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

PORT=$("$HERE/_wait_for_board.sh")
echo "Found ${PORT}. Building & uploading..."
exec pio run -t upload --upload-port "$PORT" "$@"
