const std = @import("std");

pub const dlopen_fn = *const fn (path: [*c]const u8, flags: std.c.RTLD) callconv(.c) ?*anyopaque;
pub const dlsym_fn = *const fn (noalias handle: ?*anyopaque, noalias symbol: [*c]const u8) callconv(.c) ?*anyopaque;
pub const dlclose_fn = *const fn (handle: ?*anyopaque) callconv(.c) i32;
pub const dlerror_fn = *const fn () callconv(.c) [*c]u8;
pub const @"error" = enum(u64) {
    none = 0,
    no_bounce_binary = 1,
    cannot_read_bounce_binary = 2,
    no_linker_path = 3,
    no_linker = 4,
    cannot_load_linker = 5,
};
pub const got_t = extern union {
    fns: extern struct {
        dlopen: dlopen_fn,
        dlsym: dlsym_fn,
        dlclose: dlclose_fn,
        dlerror: dlerror_fn,
    },
    result: extern struct {
        success: u64 = 0,
        @"error": @"error" = .none,
        _padding1: u64 = 0,
        _padding2: u64 = 0,
    },
};

pub const got = @extern(*const got_t, .{ .name = "sd_got" });

/// Configures globals for the `std` to work properly. Should be called right at the start of `main`.
///
/// Note: treads started with `std.Thread` should not touch functions loaded
/// through `got` since in such threads `libc` will not have it's thread
/// locals. It is better to use `pthread_create` instead.
pub fn init(argc: u64, argv: [*]const [*:0]const u8) void {
    const auxv = get_auxv(argc, argv);
    std.os.linux.elf_aux_maybe = auxv;

    const phdrs = phdrs_from_auxv(auxv);

    if (got.result.success == 0) {
        // If the dynamic linker was never loaded, no TLS setup was ever
        // performed, so we need to perform full TLS setup.
        std.os.linux.tls.initStatic(phdrs);
    } else {
        // Otherwise preserve the TLS and only write area description.
        std.os.linux.tls.area_desc = compute_tls_area_desc(phdrs);
    }
}

fn get_auxv(argc: u64, argv: [*]const [*:0]const u8) [*]std.elf.Auxv {
    const envp: [*:null]const ?[*:0]const u8 = @ptrCast(@alignCast(argv + argc + 1));
    var n_env: usize = 0;
    while (envp[n_env]) |_| : (n_env += 1) {}
    return @ptrCast(@alignCast(@constCast(envp + n_env + 1)));
}

fn phdrs_from_auxv(auxv: [*]const std.elf.Auxv) []std.elf.Phdr {
    var at_phdr: usize = 0;
    var at_phnum: usize = 0;
    var i: usize = 0;
    while (auxv[i].a_type != std.elf.AT_NULL) : (i += 1) {
        switch (auxv[i].a_type) {
            std.elf.AT_PHDR => at_phdr = auxv[i].a_un.a_val,
            std.elf.AT_PHNUM => at_phnum = auxv[i].a_un.a_val,
            else => {},
        }
    }
    return @as([*]std.elf.Phdr, @ptrFromInt(at_phdr))[0..at_phnum];
}

/// Based on the `std.os.linux.tls.computeAreaDesc`
fn compute_tls_area_desc(phdrs: []const std.elf.Phdr) @TypeOf(std.os.linux.tls.area_desc) {
    const word = @sizeOf(usize);
    // Sizes of private types: `AbiTcb`, `ZigTcb` and `Dtv` from `std/os/linux/tls.zig`
    const abi_tcb_size: usize = switch (@import("builtin").cpu.arch) {
        .x86_64 => word, // variant II: { self: *AbiTcb }
        .aarch64, .aarch64_be => 2 * word, // variant I:  { dtv: usize, _reserved: ?*anyopaque }
        else => @compileError("unsupported architecture"),
    };
    const zig_tcb_size: usize = word; // { dummy: usize }
    const dtv_size: usize = 2 * word; // { len: usize, tls_block: [*]u8 }
    const dtv_align: usize = word;

    var tls_phdr: ?*const std.elf.Phdr = null;
    var img_base: usize = 0;
    for (phdrs) |*phdr| {
        switch (phdr.p_type) {
            std.elf.PT_PHDR => img_base = @intFromPtr(phdrs.ptr) - phdr.p_vaddr,
            std.elf.PT_TLS => tls_phdr = phdr,
            else => {},
        }
    }

    var align_factor: usize = undefined;
    var block_init: []const u8 = undefined;
    var block_size: usize = undefined;
    if (tls_phdr) |phdr| {
        align_factor = phdr.p_align;

        // The effective size in memory is represented by `p_memsz`; the length of the data stored
        // in the `PT_TLS` segment is `p_filesz` and may be less than the former.
        block_init = @as([*]u8, @ptrFromInt(img_base + phdr.p_vaddr))[0..phdr.p_filesz];
        block_size = phdr.p_memsz;
    } else {
        align_factor = @alignOf(usize);

        block_init = &[_]u8{};
        block_size = 0;
    }

    // Offsets into the TLS area.
    var dtv_offset: usize = undefined;
    var abi_tcb_offset: usize = undefined;
    var block_offset: usize = undefined;
    var size: usize = 0;

    switch (@import("builtin").cpu.arch) {
        // Variant II: | TLS blocks | ABI TCB | Zig TCB | DTV |
        //                          ^ the thread pointer points here
        .x86_64 => {
            block_offset = size;
            size += std.mem.alignForward(usize, block_size, align_factor);
            // The TP is aligned to `align_factor`.
            abi_tcb_offset = size;
            size += abi_tcb_size;
            // The `ZigTcb` structure is right after the `AbiTcb` with no padding in between so it
            // can be easily found.
            size += zig_tcb_size;
            // It doesn't really matter where we put the DTV, so give it natural alignment.
            size = std.mem.alignForward(usize, size, dtv_align);
            dtv_offset = size;
            size += dtv_size;
        },
        // Variant I: | DTV | Zig TCB | ABI TCB | TLS blocks |
        //                             ^ the thread pointer points here
        .aarch64, .aarch64_be => {
            dtv_offset = size;
            size += dtv_size;
            // Add some padding here so that the TP (`abi_tcb_offset`) is aligned to `align_factor`
            // and the `ZigTcb` structure can be found by simply subtracting `@sizeOf(ZigTcb)` from
            // the TP.
            const delta = (size + zig_tcb_size) & (align_factor - 1);
            if (delta > 0) size += align_factor - delta;
            size += zig_tcb_size;
            abi_tcb_offset = size;
            size += std.mem.alignForward(usize, abi_tcb_size, align_factor);
            block_offset = size;
            size += block_size;
        },
        else => @compileError("unsupported architecture"),
    }

    return .{
        .size = size,
        .alignment = align_factor,
        .dtv = .{ .offset = dtv_offset },
        .abi_tcb = .{ .offset = abi_tcb_offset },
        .block = .{ .init = block_init, .offset = block_offset, .size = block_size },
        .gdt_entry_number = @bitCast(@as(isize, -1)),
    };
}
