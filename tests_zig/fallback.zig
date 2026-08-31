const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

threadlocal var tl_u32: u32 = 5;
threadlocal var tl_u64: [4]u64 = .{ 0xc0ffee, 0xc0ffee1, 0xc0ffee2, 0xc0ffee3 };
threadlocal var tl_bss: [8]u64 = @splat(0);

var counter: std.atomic.Value(u32) = .init(0);

fn body() void {
    tl_u32 += 37;
    _ = counter.fetchAdd(1, .monotonic);
}

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    const tp_before = utils.get_thread_pointer();
    utils.assert_eq(tp_before, 0);
    // Since linker was not loaded, this should call `initStatic`
    sd.init(argc, argv);
    utils.assert(utils.get_thread_pointer() != 0);

    utils.assert_eq(sd.got.result.success, 0);
    utils.assert_eq(sd.got.result.@"error", .no_bounce_binary);

    utils.assert_eq(tl_u32, 5);
    utils.assert_eq(tl_u64[0], 0xc0ffee);
    utils.assert_eq(tl_u64[3], 0xc0ffee3);
    for (tl_bss) |v| utils.assert_eq(v, 0);

    tl_u32 = 69;
    tl_u32 += 1;
    utils.assert_eq(tl_u32, 70);

    tl_bss[1] = 0xbeef;
    utils.assert_eq(tl_bss[1], 0xbeef);

    utils.assert(std.os.linux.elf_aux_maybe != null);
    utils.assert_eq(std.os.linux.getauxval(std.elf.AT_PAGESZ), 4096);
    utils.assert(std.math.isPowerOfTwo(std.os.linux.tls.area_desc.alignment));

    // Zig threads should work
    tl_u32 = 5;
    const th = std.Thread.spawn(.{}, body, .{}) catch unreachable;
    th.join();
    utils.assert_eq(counter.load(.monotonic), 1);
    utils.assert_eq(tl_u32, 5);

    return 0;
}
