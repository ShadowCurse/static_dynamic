#!/bin/sh
# Usage:
# ./test.sh              build everything and run it
# ./test.sh --no-build   run what is already built
set -u

if [ "${1:-}" != "--no-build" ]; then
  sh build.sh tests >/dev/null || exit 1
  zig build tests || exit 1
fi

out=$(mktemp)
trap 'rm -f "$out"' EXIT

# run <binary> <expected exit code> [pattern string the output must contain] [binary args...]
run() {
  name=$(basename "$1")
  bin=$1
  expected=$2
  pattern=${3:-}
  shift 2
  [ $# -eq 0 ] || shift

  if [ ! -x "$bin" ]; then
    echo "FAIL $name: not built"
    return
  fi

  # Some errors can result in infinitely hunging tests, so limit runtime
  timeout 30 "$bin" "$@" >"$out" 2>&1
  got=$?

  error_str=""
  if [ "$got" -eq 124 ]; then
    error_str="timed out"
  elif [ "$got" -ne "$expected" ]; then
    error_str="expected exit $expected, got $got"
  elif [ -n "$pattern" ] && ! grep -q "$pattern" "$out"; then
    error_str="output is missing \"$pattern\""
  fi

  if [ -z "$error_str" ]; then
    echo "ok   $name"
  else
    echo "FAIL $name: $error_str"
    sed 's/^/       | /' "$out"
  fi
  # echo "out:" $(cat $out)
}

echo "############# C tests #############"
# Linker is fine
run tests_c/build/linker              0
run tests_c/build/tls                 0
run tests_c/build/pthread             0
# No linker
run tests_c/build/fallback_tls        0
run tests_c/build/fallback_crash    139 # SIGSEGV

echo "############ Zig tests ############"
# Linker is fine
run zig-out/bin/linker                     0
run zig-out/bin/no_fallback_thread_crash 134 "reached unreachable code"
run zig-out/bin/pthread                    0
run zig-out/bin/stack_trace_crash        134 "panic on purpose"
run zig-out/bin/std                        0
run zig-out/bin/std_thread                 0
run zig-out/bin/std_thread_libc_crash    139 # SIGSEGV
# No linker
run zig-out/bin/fallback                   0
