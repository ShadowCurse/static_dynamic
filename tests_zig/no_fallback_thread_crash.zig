const std = @import("std");
const utils = @import("util.zig");

pub const _start = {};

fn body() void {}

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    _ = argc;
    _ = argv;
    // Skip `sd.init` which should cause `std.Thread` to crash
    utils.assert(std.os.linux.elf_aux_maybe == null);

    const th = std.Thread.spawn(.{}, body, .{}) catch return 2;
    th.join();

    return 1;
}
