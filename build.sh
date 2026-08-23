# Using musl libc is possible
#
musl-gcc static_dynamic_test.c -static -nostartfiles -fno-stack-protector -o static_dynamic -g -O3

# Otherwise can compile with normal gcc command (for the demo need to remove the `printf` usage)
#
# gcc static_dynamic_test.c -static -nostartfiles -nodefaultlibs -nostdlib -fno-stack-protector -o static_dynamic -g -O3
