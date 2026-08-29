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
        dlsym:  dlsym_fn,
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
