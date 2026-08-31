const std = @import("std");

pub fn assert(cond: bool) void {
    if (!cond) @panic("assertion failed");
}

pub fn assert_eq(actual: anytype, expected: @TypeOf(actual)) void {
    if (actual != expected) std.debug.panic("expected {any}, got {any}", .{ expected, actual });
}

pub fn get_thread_pointer() usize {
    switch (@import("builtin").cpu.arch) {
        .x86_64 => {
            var tp: usize = 0;
            _ = std.os.linux.syscall2(.arch_prctl, std.os.linux.ARCH.GET_FS, @intFromPtr(&tp));
            return tp;
        },
        .aarch64, .aarch64_be => return asm ("mrs %[tp], tpidr_el0"
            : [tp] "=r" (-> usize),
        ),
        else => @compileError("unsupported architecture"),
    }
}

pub const Libc = struct {
    handle: ?*anyopaque,

    snprintf: *const fn ([*]u8, usize, [*:0]const u8, ...) callconv(.c) c_int,
    malloc: *const fn (usize) callconv(.c) ?*anyopaque,
    free: *const fn (?*anyopaque) callconv(.c) void,
    open: *const fn ([*:0]const u8, i32) callconv(.c) i32,
    errno_location: *const fn () callconv(.c) *i32,
    pthread_create: *const fn (
        *usize,
        ?*anyopaque,
        *const fn (?*anyopaque) callconv(.c) ?*anyopaque,
        ?*anyopaque,
    ) callconv(.c) i32,
    pthread_join: *const fn (usize, ?*?*anyopaque) callconv(.c) i32,

    pub fn load(got: anytype) Libc {
        const libc = got.fns.dlopen("libc.so.6", .{ .NOW = true });
        assert(libc != null);

        const snprintf = got.fns.dlsym(libc, "snprintf") orelse @panic("snprintf");
        const malloc = got.fns.dlsym(libc, "malloc") orelse @panic("malloc");
        const free = got.fns.dlsym(libc, "free") orelse @panic("free");
        const open = got.fns.dlsym(libc, "open") orelse @panic("open");
        const errno_location = got.fns.dlsym(libc, "__errno_location") orelse @panic("__errno_location");
        const pthread_create = got.fns.dlsym(libc, "pthread_create") orelse @panic("pthread_create");
        const pthread_join = got.fns.dlsym(libc, "pthread_join") orelse @panic("pthread_join");

        return .{
            .handle = libc,
            .snprintf = @ptrCast(snprintf),
            .malloc = @ptrCast(malloc),
            .free = @ptrCast(free),
            .open = @ptrCast(open),
            .errno_location = @ptrCast(errno_location),
            .pthread_create = @ptrCast(pthread_create),
            .pthread_join = @ptrCast(pthread_join),
        };
    }
};
