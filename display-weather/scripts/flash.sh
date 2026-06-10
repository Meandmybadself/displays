#!/usr/bin/env bash
# Thin wrapper → display-common/scripts/flash.sh (runs pio in this app dir).
exec "$(dirname "$0")/../../display-common/scripts/flash.sh" "$@"
