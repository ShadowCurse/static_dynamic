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

# run <binary> <expected exit code> [pattern string the output must contain]
run() {
  name=$(basename "$1")

  if [ ! -x "$1" ]; then
    echo "FAIL $name: not built"
    return
  fi

  # Some errors can result in infinitely hunging tests, so limit runtime
  timeout 30 "$1" >"$out" 2>&1
  got=$?

  error_str=""
  if [ "$got" -eq 124 ]; then
    error_str="timed out"
  elif [ "$got" -ne "$2" ]; then
    error_str="expected exit $2, got $got"
  elif [ -n "${3:-}" ] && ! grep -q "$3" "$out"; then
    error_str="output is missing \"$3\""
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
