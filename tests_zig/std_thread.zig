const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

threadlocal var tl_u32: u32 = 5;
threadlocal var tl_arr: [4]u64 = .{ 0xc0ffee, 0xc0ffee1, 0xc0ffee2, 0xc0ffee3 };
threadlocal var tl_bss: [8]u64 = @splat(0);

var main_tp: usize = 0;
var counter: std.atomic.Value(u32) = .init(0);

fn bump() void {
    _ = counter.fetchAdd(1, .monotonic);
}

fn body() void {
    const tp = utils.get_thread_pointer();
    utils.assert(tp != 0);
    utils.assert(tp != main_tp);

    utils.assert_eq(tl_u32, 5);
    utils.assert_eq(tl_arr[0], 0xc0ffee);
    utils.assert_eq(tl_arr[3], 0xc0ffee3);
    tl_u32 = 100;

    const page = std.heap.page_allocator;
    const a = page.alloc(u8, 8192) catch unreachable;
    a[8191] = 1;
    utils.assert_eq(a[8191], 1);
    page.free(a);

    const smp = std.heap.smp_allocator;
    const b = smp.alloc(u8, 5000) catch unreachable;
    b[4999] = 2;
    utils.assert_eq(b[4999], 2);
    smp.free(b);

    const th = std.Thread.spawn(.{}, bump, .{}) catch unreachable;
    th.join();
    utils.assert_eq(counter.load(.monotonic), 1);

    utils.assert_eq(std.Thread.getCurrentId(), @as(u32, @intCast(std.os.linux.gettid())));
}

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    utils.assert(sd.got.result.success != 0);

    main_tp = utils.get_thread_pointer();
    tl_u32 = 7;

    const th = std.Thread.spawn(.{}, body, .{}) catch unreachable;
    th.join();
    // The thread wrote 100 into its own copy.
    utils.assert_eq(tl_u32, 7);

    // Several at once, to check the areas are not shared by accident.
    counter.store(0, .monotonic);
    var threads: [4]std.Thread = undefined;
    for (&threads) |*th2| th2.* = std.Thread.spawn(.{}, bump, .{}) catch unreachable;
    for (threads) |th2| th2.join();
    utils.assert_eq(counter.load(.monotonic), 4);

    return 0;
}
