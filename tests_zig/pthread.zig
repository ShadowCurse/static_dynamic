const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

threadlocal var tl_u32: u32 = 5;
threadlocal var tl_arr: [4]u64 = .{ 0xc0ffee, 0xc0ffee1, 0xc0ffee2, 0xc0ffee3 };

var libc: utils.Libc = undefined;
var main_tp: usize = 0;
var area_size: usize = 0;
var counter: std.atomic.Value(u32) = .init(0);

fn bump() void {
    _ = counter.fetchAdd(1, .monotonic);
}

fn body(_: ?*anyopaque) callconv(.c) ?*anyopaque {
    const tp = utils.get_thread_pointer();
    utils.assert(tp != 0);
    utils.assert(tp != main_tp);

    utils.assert(std.os.linux.elf_aux_maybe != null);
    utils.assert_eq(std.os.linux.getauxval(std.elf.AT_PAGESZ), 4096);
    utils.assert_eq(std.os.linux.tls.area_desc.size, area_size);

    utils.assert_eq(tl_u32, 5);
    utils.assert_eq(tl_arr[0], 0xc0ffee);
    utils.assert_eq(tl_arr[3], 0xc0ffee3);
    tl_u32 = 100;

    const e = libc.errno_location();
    utils.assert(@as(u64, @intFromPtr(e)) != 0);
    _ = libc.open("blah", 0);
    utils.assert_eq(e.*, 2);

    const p = libc.malloc(4096);
    utils.assert(p != null);
    @as([*]u8, @ptrCast(p.?))[4095] = 1;
    utils.assert_eq(@as([*]u8, @ptrCast(p.?))[4095], 1);
    libc.free(p);

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

    var dbg: std.heap.DebugAllocator(.{}) = .init;
    const c = dbg.allocator().alloc(u32, 64) catch unreachable;
    dbg.allocator().free(c);

    const th = std.Thread.spawn(.{}, bump, .{}) catch unreachable;
    th.join();
    utils.assert_eq(counter.load(.monotonic), 1);

    utils.assert_eq(std.Thread.getCurrentId(), @as(u32, @intCast(std.os.linux.gettid())));

    return null;
}

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    utils.assert(sd.got.result.success != 0);
    libc = utils.Libc.load(sd.got);

    main_tp = utils.get_thread_pointer();
    area_size = std.os.linux.tls.area_desc.size;
    tl_u32 = 7;

    var tid: usize = 0;
    utils.assert_eq(libc.pthread_create(&tid, null, body, null), 0);
    utils.assert_eq(libc.pthread_join(tid, null), 0);

    utils.assert_eq(tl_u32, 7);

    return 0;
}
