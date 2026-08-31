const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    utils.assert(sd.got.result.success != 0);
    utils.assert(0 < std.posix.getSelfPhdrs().len);
    @panic("panic on purpose, should pet `SIGABRT`, the trace has to name this file");
}
