const std = @import("std");
const sd = @import("static_dynamic");
const utils = @import("util.zig");

pub const _start = {};

threadlocal var tl: u32 = 5;

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);
    utils.assert(sd.got.result.success != 0);

    utils.assert(std.os.linux.elf_aux_maybe != null);
    utils.assert_eq(std.os.linux.getauxval(std.elf.AT_PAGESZ), 4096);
    utils.assert(std.os.linux.getauxval(std.elf.AT_PHDR) != 0);
    utils.assert_eq(std.heap.pageSize(), 4096);

    var has_tls = false;
    var has_load = false;
    for (std.posix.getSelfPhdrs()) |p| {
        if (p.type == .TLS) has_tls = true;
        if (p.type == .LOAD) has_load = true;
    }
    utils.assert(has_load and has_tls);

    const d = std.os.linux.tls.area_desc;
    utils.assert(std.math.isPowerOfTwo(d.alignment));
    utils.assert(d.block.size <= d.size);
    utils.assert(d.block.init.len <= d.block.size);
    utils.assert(@intFromPtr(d.block.init.ptr) != 0);

    utils.assert(utils.get_thread_pointer() != 0);
    utils.assert_eq(tl, 5);

    const page = std.heap.page_allocator;
    const a = page.alloc(u8, 8192) catch unreachable;
    a[8191] = 1;
    utils.assert_eq(a[8191], 1);
    page.free(a);

    const smp = std.heap.smp_allocator;
    const b = smp.alloc(u8, 12345) catch unreachable;
    b[12344] = 2;
    utils.assert_eq(b[12344], 2);
    smp.free(b);

    var dbg: std.heap.DebugAllocator(.{}) = .init;
    const c = dbg.allocator().alloc(u32, 64) catch unreachable;
    dbg.allocator().free(c);

    const rc = std.os.linux.openat(std.os.linux.AT.FDCWD, "blah", .{}, 0);
    utils.assert(std.os.linux.errno(rc) == .NOENT);

    utils.assert_eq(std.Thread.getCurrentId(), @as(u32, @intCast(std.os.linux.gettid())));
    utils.assert(0 < (std.Thread.getCpuCount() catch 0));

    return 0;
}
