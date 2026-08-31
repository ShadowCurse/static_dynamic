const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);

    utils.assert(sd.got.result.success != 0);
    utils.assert(@as(u64, @intFromPtr(sd.got.fns.dlopen))  != 0);
    utils.assert(@as(u64, @intFromPtr(sd.got.fns.dlsym))   != 0);
    utils.assert(@as(u64, @intFromPtr(sd.got.fns.dlclose)) != 0);
    utils.assert(@as(u64, @intFromPtr(sd.got.fns.dlerror)) != 0);

    const libc = sd.got.fns.dlopen("libc.so.6", .{ .NOW = true });
    utils.assert(libc != null);
    utils.assert(sd.got.fns.dlsym(libc, "printf") != null);

    utils.assert(sd.got.fns.dlsym(libc, "blah") == null);
    const err = sd.got.fns.dlerror();
    utils.assert(err != null and err[0] != 0);
    utils.assert(sd.got.fns.dlerror() == null);

    utils.assert(sd.got.fns.dlopen("blah.so", .{ .NOW = true }) == null);
    utils.assert(sd.got.fns.dlerror() != null);

    return 0;
}
