const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

var libc: utils.Libc = undefined;

fn body() void {
    var buf: [64]u8 = undefined;
    // On `std.Thread` thread this should cause `SIGSEGV`
    _ = libc.snprintf(&buf, buf.len, "%.3f", @as(f64, 2.5));
}

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    utils.assert(sd.got.result.success != 0);
    libc = utils.Libc.load(sd.got);

    // On main thread this should work
    var buf: [64]u8 = undefined;
    _ = libc.snprintf(&buf, buf.len, "%.3f", @as(f64, 2.5));
    utils.assert(std.mem.eql(u8, std.mem.sliceTo(&buf, 0), "2.500"));

    const th = std.Thread.spawn(.{}, body, .{}) catch unreachable;
    th.join();

    return 1;
}
