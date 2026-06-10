#!/usr/bin/env bash
# Thin wrapper → display-common/scripts/monitor.sh.
exec "$(dirname "$0")/../../display-common/scripts/monitor.sh" "$@"
