#!/usr/bin/env bash
# compile_stages.sh — run each of the four compilation stages separately and
# show what changed. Run this once; it makes the whole toolchain click.
#
#   bash compile_stages.sh
set -euo pipefail

CC=${CC:-gcc}
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/hello.c"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

banner() { printf '\n\033[1m=== %s ===\033[0m\n' "$1"; }

banner "STAGE 0 — the source you wrote"
printf '%s: %s lines, %s bytes\n' "hello.c" "$(wc -l < "$SRC")" "$(wc -c < "$SRC")"

banner "STAGE 1 — PREPROCESSOR   (gcc -E)   #include / #define / #ifdef"
$CC -E "$SRC" -o "$OUT/hello.i"
printf 'hello.i: %s lines (was %s) — stdio.h was pasted in wholesale\n' \
    "$(wc -l < "$OUT/hello.i")" "$(wc -l < "$SRC")"
echo '--- the printf declaration the compiler will actually see: ---'
grep -m1 'extern int printf' "$OUT/hello.i" || grep -m1 'printf' "$OUT/hello.i"

banner "STAGE 2 — COMPILER       (gcc -S)   C -> assembly"
$CC -S -O0 "$OUT/hello.i" -o "$OUT/hello.s"
printf 'hello.s: %s lines of %s assembly\n' "$(wc -l < "$OUT/hello.s")" "$(uname -m)"
echo '--- the body of main: ---'
sed -n '/^main:/,/^\s*ret/p' "$OUT/hello.s" | head -25

banner "STAGE 3 — ASSEMBLER      (gcc -c)   assembly -> object code"
$CC -c "$OUT/hello.s" -o "$OUT/hello.o"
printf 'hello.o: %s bytes of machine code + a symbol table\n' "$(wc -c < "$OUT/hello.o")"
echo '--- symbols (T = defined here, U = UNDEFINED, must come from elsewhere): ---'
nm "$OUT/hello.o" 2>/dev/null || echo '(nm not available)'
echo
echo 'Note printf is marked U. The object file knows it is called but has no'
echo 'idea where it lives. Resolving that is the linker'"'"'s entire job.'

banner "STAGE 4 — LINKER         (gcc)      resolve symbols, attach libc"
$CC "$OUT/hello.o" -o "$OUT/hello"
printf 'hello: %s bytes executable\n' "$(wc -c < "$OUT/hello")"
echo '--- shared libraries it will load at run time: ---'
ldd "$OUT/hello" 2>/dev/null || otool -L "$OUT/hello" 2>/dev/null || echo '(static or unavailable)'

banner "RUN"
"$OUT/hello"
echo "exit status = $?"

banner "THE POINT"
cat <<'EOF'
  hello.c   ->  hello.i   ->  hello.s   ->  hello.o   ->  hello
             cpp         cc1          as           ld

  Errors tell you which stage failed:
    "No such file or directory"      -> stage 1, preprocessor, bad -I path
    "expected ';' before"            -> stage 2, compiler, syntax
    "implicit declaration of"        -> stage 2, missing #include
    "undefined reference to 'sqrt'"  -> stage 4, LINKER, missing -lm
EOF
