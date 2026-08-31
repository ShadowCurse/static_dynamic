# Static dynamic

The one header library to allow statically linked binaries to get access to the
dynamic loading functionality.

The trick for doing this is to make the application do part of the kernel job
and load the dynamic linker manually. This has the additional benefit of making
the application `libc` version independent. The path to the linker is found
using an assumption that on each Linux machine there is a dynamically linked
`/bin/sh` binary from which the system's linker path can be obtained. Assuming
this holds on all distributions, this also makes the application linker
independent.

For doing all of the preparatory work, the library needs some space to store
data required for the dynamic linker to launch properly. Trying to take as
little space as possible and be non-intrusive as possible, the library
allocates all of the permanent data on the stack in a tightly packed manner.
This uses ~2K (depends on the number of args and env vars present) of the stack
space which is just a small percentage of the usual stack on linux.

> [!NOTE]
>
> Currently only x86_64 and aarch64 are supported

## Usage

### Bounce file

The bounce file can be overwritten by defining `SD_BOUNCE_BINARY` (default `/bin/sh`).

### C

Just include the header to your application and ensure the `main` function signature is

```c
int main(int argc, char** argv)
```

> [!NOTE]
>
> Even though unlikely, there is still a possibility that loading the dynamic
> linker will fail. To check if there was an error just check `sd_got.success`
> and if there was one, `sd_got.error` will contain the error code. It is
> advised to check for the `success` at the beginning of the `main`.

If the linker was loaded successfully, `sd_got` will contain `dlopen`, `dlsym`,
`dlclose`, `dlerror` function pointers. There are also `SD_RTLD_NOW` and
`SD_RTLD_LAZY` macros already defined to avoid including additional headers.

```c
void* libc = sd_got.dlopen("libc.so.6", SD_RTLD_NOW);
```

#### Compilation flags

The loading of the dynamic linker happens before the `main` is called. For this
reason the library defines its own `_start` symbol from which the program
execution should start. For this to work, build must include compilation flags:
`-nostartfiles -fno-stack-protector`.

#### Usage with statically linked `musl`

It is not advisable to link `musl` in addition to using this library. The
reason for this is that `musl` (like `glibc`) needs to be initialized before it
can be properly used. This is usually done by calling `__libc_start_main`, but
since the program is initialized though the dynamic linker, the `musl`
initialization is never invoked. But that is not the whole issue. Both `musl`
and `glibc` want to set up `TLS` the way they need it, so calling
`__libc_start_main` after dynamic linker setup will break `glibc`, but without
it `musl` is in a broken state.

Fortunately, some `musl` functions (like `printf`) can still work as long as
they do not set `errno` (since `errno` is inside `TLS` block, but `glibc` and
`musl` put it at different offsets), or access uninitialized global variables
(for example `pthread_create` will fail because `libc.can_do_threads` will not
be set by `__init_tls` call).

> [!WARNING]
>
> In general it is better to avoid linking `musl` and just rely on raw syscalls
> or functions obtained from loaded `glibc`.

#### Example

There is an example `static_dynamic_test.c` with `build.sh` that shows all of this and
builds a simple `raylib` demo with this functionality

### Zig

First you need to add `_start` definition to your root module:

```zig
pub const _start = {};
```

This will tell Zig to not generate the `_start` symbol since it is already present in `static_dynamic.h`.

The `main` function should be defined as:

```zig
export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32
```

The first thing you would need to call inside `main` is `sd.init` which will
perform setup of global variables inside Zig `std`. More about this down below.
Additionally don't forget to check `sd.got.result.success` just in case.

```zig
export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    if (sd.got.result.success == 0) return 1;
    ...
}
```

The usage of provided functions is very close to the C version:

```zig
const libc = sd.got.fns.dlopen("libc.so.6", .{ .NOW = true });
```

#### `build.zig`

All you need to do is to add C source file to your `root_module`:

```zig
const sd_c = b.addWriteFiles().add("static_dynamic.c",
    \\#include "static_dynamic.h"
);
root_module.addCSourceFile(.{
    .file = sd_c,
    .flags = &[_][]const u8{ "-fno-stack-protector" },
});
root_module.addIncludePath(b.path("."));
```

#### About `sd.init`

Zig needs to have some initialization performed in order for `std` to work
properly. Usually Zig does it inside its `std.start` code, but since we skip
it, we need to do it ourselves. The `static_dynamic.zig` provides an `init`
function specifically for this purpose.

> [!NOTE]
>
> Even though `sd.init` makes Zig `std` functions work, there is a caveat when
> it comes to the `std.Thread.spawn`: it only creates `TLS` for the Zig usage,
> so using `libc` functions from these threads most likely will crash the
> program. Instead it is better to just load `pthread_create` and use that for
> new thread creation. This way new threads will be able to use both `std` and
> `libc` functions.


#### Usage with statically linked `musl`

Linking with `musl` does not work. Compiling with `link_libc = true` and
`-Dtarget=x86_64-linux-musl` creates a symbol collision since Zig links
`crt1.o` unconditionally which defines its own `_start`. Working around this
issue by compiling Zig code to object files and doing external linking will hit
same issues as the C version.

#### Example

All of the info above is also repeated in `build.zig` and `static_dynamic_test.zig`.

### Tests

```bash
# will compile tests with `bash ./built.sh tests` and `zig build tests` and run them
$ bash ./test.sh
```

## Acknowledgements

- [Detour](https://github.com/graphitemaster/detour) - provided the base idea
  of how this whole machinery should work
