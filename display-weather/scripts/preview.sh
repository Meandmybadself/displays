#!/usr/bin/env bash
# Thin wrapper → display-common/scripts/preview.sh. Args after `--` go to the
# preview binary, e.g.:  scripts/preview.sh -- --screen status --cond "Heavy Snow"
exec "$(dirname "$0")/../../display-common/scripts/preview.sh" "$(cd "$(dirname "$0")/.." && pwd)" "$@"
