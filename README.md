# Static dynamic

The one header library to allow statically linked binaries get access to the
dynamic loading functionality.

The trick for doing this is to make the application do part of the kernel job
and load the dynamic linker manually. This has additional benefit of making the
application `libc` version independent. The path to the linker is found using
an assumption that on each linux machine there is a dynamically linked
`/bin/sh` binary from which the system's linker path can be obtained. Assuming
this holds on all distributions, this also makes the application linker
independent.

Since the system `libc` is loaded separately, this means the applications can
still use statically linked `libc` such as `musl` without issues.

For doing all of the preparatory work, the library needs some space to store
data required for the dynamic linker to launch properly. Trying to take as
little space as possible and be non intrusive as possible, the library
allocates all of the permanent data on the stack in a tightly packed manner.
This uses ~2K (depends on the number of args and env vars present) of the stack
space which is just a small percentage of the usual stack on linux.

> [!NOTE]
>
> Currently only x86_64 is supported

## Usage

Just include the header to your application and change the `main` function signature to

```c
int main(u64* argc_argv)
```

To obtain `argc`, `argv`, `envp` and so on, just do a simple pointer math:

```c
u64  argc = *argc_argv;
u64* argv = argc_argv + 1;
u64* envp = argv + argc + 1;
```

The access to the `dlopen` and other functions is provided as an additional
argument at the very end of the argument list:

```c
u64* got  = (u64*)*(argc_argv + argc);
```

The `got` is a pointer to the array of 4 function pointers: `dlopen`, `dlsym`,
`dlclose`, `dlerror`

```c
dlopen_fn  dlopen  = (dlopen_fn) got[0];
dlsym_fn   dlsym   = (dlsym_fn)  got[1];
dlclose_fn dlclose = (dlclose_fn)got[2];
dlerror_fn dlerror = (dlerror_fn)got[3];
```

There is an example `static_dynamic_test.c` that shows all of this as well and
builds a simple `raylib` demo with this functionality

## Acknowledgements

- [Detour](https://github.com/graphitemaster/detour) - provided the base idea
  of how this whole machinery should work
