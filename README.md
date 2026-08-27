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

Since the system `libc` is loaded separately, this means the applications can
still use statically linked `libc` such as `musl` without issues.

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

### C

Just include the header to your application and ensure the `main` function signature is

```c
int main(int argc, char** argv)
```

> [!NOTE]
>
> Even though unlikely, there is still a possibility that loading dynamic
> linker will fail. To check if there was an error just check the
> `sd_got.success` and if there was one `sd_got.error` will contain the error
> code.

The access to the `dlopen` and other functions just access the `sd_got` global.
There are also a `SD_RTLD_NOW` and `SD_RTLD_LAZY` macros already defined to
avoid including additional headers.

```c
void* libc = sd_got.dlopen("libc.so.6", SD_RTLD_NOW);
```

The `sd_got` itself contains 4 function pointers: `dlopen`, `dlsym`,
`dlclose`, `dlerror`.

#### Compilation flags

The loading of the dynamic linker happens before the `main` is called. For this
reason the library defines its own `_start` symbol from which the program
execution should start. For this to work, build must include compilation flags:
`-nostartfiles -fno-stack-protector`.

#### Usage with statically linked `musl`

If program links `musl` it is advised to define `SD_MUSL` to allow for a
graceful fallback in case of linker loading error. If that happen the `musl`
library will still be initialized and so you would be able to call it's
functions as usual. Otherwise your program will fail on the first `musl` call.

#### Example

There is an example `static_dynamic_test.c` with `build.sh` that shows all of this and
builds a simple `raylib` demo with this functionality

### Zig

First you need to add `_start` definition to your root module:

```zig
pub const _start = {};
```

This will tell Zig to not generate `_start` symbol. It is already present in the `static_dynamic.h`.

The main definition needs to be defined as:

```zig
export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32
```

Then `static_dynamic.zig` file contains bindings for the types and global `sd_got`:

```zig
const libc = sd.got.fns.dlopen("libc.so.6", .{ .NOW = true });
```

#### Build

All you need to do is to add C compilation file to your root module:

```zig
const sd_c = b.addWriteFiles().add("static_dynamic.c",
    \\#include "static_dynamic.h"
);
mod.addCSourceFile(.{ .file = sd_c });
mod.addIncludePath(b.path("."));
```

#### Example

All of the info above is also repeated in `build.zig` and `static_dynamic_test.zig`.


## Acknowledgements

- [Detour](https://github.com/graphitemaster/detour) - provided the base idea
  of how this whole machinery should work
