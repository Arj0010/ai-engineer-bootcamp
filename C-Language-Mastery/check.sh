#!/usr/bin/env bash
# Compile-check every program in the curriculum.
# Usage: ./check.sh [module-dir]
set -uo pipefail

CC=${CC:-gcc}
CFLAGS="-std=c17 -Wall -Wextra -Wpedantic -O2"
LDLIBS="-lm -lpthread"
ROOT="$(cd "$(dirname "$0")" && pwd)"
SCOPE="${1:-.}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cd "$ROOT" || exit 1
ok=0; fail=0; failed=()

# --- standalone single-file programs -----------------------------------------
while IFS= read -r f; do
    d=$(dirname "$f")
    [ -f "$d/Makefile" ] && continue          # handled by its own build
    grep -qE '\bmain[[:space:]]*\(' "$f" || continue
    if out=$($CC $CFLAGS "$f" -o "$TMP/a.out" $LDLIBS 2>&1); then
        ok=$((ok+1))
        [ -n "$out" ] && { echo "WARN $f"; echo "$out" | sed 's/^/     /'; }
    else
        fail=$((fail+1)); failed+=("$f")
        echo "FAIL $f"; echo "$out" | sed 's/^/     /'
    fi
done < <(find "$SCOPE" -name '*.c' -not -path './build/*' -not -path '*/broken/*' | sort)

# --- multi-file projects with their own Makefile ------------------------------
while IFS= read -r mk; do
    d=$(dirname "$mk")
    [ "$d" = "." ] && continue
    if out=$(make -C "$d" 2>&1); then
        ok=$((ok+1))
        make -C "$d" clean >/dev/null 2>&1
    else
        fail=$((fail+1)); failed+=("$d (project)")
        echo "FAIL $d"; echo "$out" | sed 's/^/     /'
    fi
done < <(find "$SCOPE" -name Makefile -not -path './build/*' | sort)

echo
echo "-------------------------------------------"
echo "  compiled clean : $ok"
echo "  failed         : $fail"
for f in "${failed[@]:-}"; do [ -n "$f" ] && echo "    - $f"; done
echo "-------------------------------------------"
[ "$fail" -eq 0 ]
