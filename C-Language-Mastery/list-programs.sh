#!/usr/bin/env bash
# List every standalone single-file C program in the curriculum.
#
# "Standalone" means: it defines main(), and neither it nor any ancestor
# directory has a Makefile (those are self-managed multi-file projects that
# need their own -I flags and companion translation units).
#
#   ./list-programs.sh [scope-dir]
#
# Used by the top-level Makefile. It lives in its own file because make's
# $(shell ...) counts parentheses, and a grep pattern containing '\(' makes
# that miserable.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1
SCOPE="${1:-.}"

in_project() {
    local d
    d=$(cd "$(dirname "$1")" && pwd)
    while [ "$d" != "/" ] && [ "$d" != "$ROOT" ]; do
        [ -f "$d/Makefile" ] && return 0
        d=$(dirname "$d")
    done
    return 1
}

find "$SCOPE" -name '*.c' -not -path './build/*' -not -path '*/broken/*' 2>/dev/null |
sort |
while IFS= read -r f; do
    in_project "$f" && continue
    grep -qE '\bmain[[:space:]]*\(' "$f" && printf '%s\n' "$f"
done
