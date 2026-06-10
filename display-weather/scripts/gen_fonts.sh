#!/usr/bin/env bash
# Thin wrapper → display-common/scripts/gen_fonts.sh, passing this app's dir.
# Generates the shared CORE faces into display-common and this app's faces
# (see scripts/app_fonts.sh) into src/fonts/.
exec "$(dirname "$0")/../../display-common/scripts/gen_fonts.sh" "$(cd "$(dirname "$0")/.." && pwd)"
