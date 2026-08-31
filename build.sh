#!/bin/sh
# Usage:
# ./build.sh         => builds example
# ./build.sh "tests" => bulds test binaries
set -e

CC=${CC:-gcc}
# CC=clang
CFLAGS="-static -nostartfiles -nodefaultlibs -nostdlib -fno-stack-protector -g"

if [ "$1" != "tests" ]; then
  $CC static_dynamic_test.c $CFLAGS -O3 -o static_dynamic
  exit 0
fi

mkdir -p tests_c/build

# Tests that run with the linker loaded normally.
for t in linker tls pthread; do
  $CC "tests_c/$t.c" $CFLAGS -O3 -o "tests_c/build/$t"
  echo "built tests_c/build/$t"
done

# Tests where dynamic linker loading fails because bounce binary does not exist
for t in fallback_tls fallback_crash; do
  $CC "tests_c/$t.c" $CFLAGS -O3 -DSD_BOUNCE_BINARY='"/sd_no_such_bounce_binary"' \
      -o "tests_c/build/$t"
  echo "built tests_c/build/$t"
done
