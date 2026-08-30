const std = @import("std");
const builtin = @import("builtin");

const sd = @import("static_dynamic.zig");

// Disable default `_start` zig want to generate since `static_dynamic.h` already defines it.
pub const _start = {};

threadlocal var tl_a: u32 = 5;

const rlColor = extern struct {
    r: u8 = 0,
    g: u8 = 0,
    b: u8 = 0,
    a: u8 = 0,
};
const RL_RED: rlColor = .{ .r = 230, .g = 41, .b = 55, .a = 255 };
const RL_BLACK: rlColor = .{ .a = 255 };

const rlVector2 = extern struct {
    x: f32 = 0.0,
    y: f32 = 0.0,
};

export fn main(argc: u64, argv: [*]const [*:0]const u8) callconv(.c) i32 {
    sd.init(argc, argv);

    if (sd.got.result.success == 0) {
        std.log.err("error during linker loading: {t}", .{sd.got.result.@"error"});
        return 1;
    }

    std.log.info("dlopen:  {*}", .{sd.got.fns.dlopen});
    std.log.info("dlsym:   {*}", .{sd.got.fns.dlsym});
    std.log.info("dlclose: {*}", .{sd.got.fns.dlclose});
    std.log.info("dlerror: {*}", .{sd.got.fns.dlerror});

    const libc = sd.got.fns.dlopen("libc.so.6", .{ .NOW = true });
    const libm = sd.got.fns.dlopen("libm.so.6", .{ .NOW = true });
    const raylib = sd.got.fns.dlopen("libraylib.so", .{ .NOW = true });

    const printf2: *const fn (fmt: [*:0]const u8, ...) callconv(.c) void =
        @ptrCast(sd.got.fns.dlsym(libc, "printf"));

    printf2("dynamic libc argc %d\n", argc);
    for (0..argc) |i| {
        printf2("argv[%d] %s\n", i, argv[i]);
    }
    printf2("thread local a %d\n", tl_a);

    printf2("libc.so is %p\n", libc);
    printf2("libm.so is %p\n", libm);
    printf2("raylib.so is %p\n", raylib);

    _ = sd.got.fns.dlopen("libfoo.so", .{ .NOW = true });
    printf2("libfoo: %s\n", sd.got.fns.dlerror());

    if (libm != null and raylib != null) {
        printf2("Demo time\n");

        const sinf: *const fn (f32) callconv(.c) f32 = @ptrCast(sd.got.fns.dlsym(libm, "sinf"));
        const rlInitWindow: *const fn (i32, i32, [*:0]const u8) callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "InitWindow"));
        const rlSetTargetFPS: *const fn (i32) callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "SetTargetFPS"));
        const rlWindowShouldClose: *const fn () callconv(.c) bool =
            @ptrCast(sd.got.fns.dlsym(raylib, "WindowShouldClose"));
        const rlGetFrameTime: *const fn () callconv(.c) f32 =
            @ptrCast(sd.got.fns.dlsym(raylib, "GetFrameTime"));
        const rlBeginDrawing: *const fn () callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "BeginDrawing"));
        const rlClearBackground: *const fn (rlColor) callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "ClearBackground"));
        const rlDrawCircleV: *const fn (rlVector2, f32, rlColor) callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "DrawCircleV"));
        const rlDrawText: *const fn ([*:0]const u8, i32, i32, i32, rlColor) callconv(.c) void =
            @ptrCast(sd.got.fns.dlsym(raylib, "DrawText"));
        const rlEndDrawing: *const fn () callconv(.c) void = @ptrCast(sd.got.fns.dlsym(raylib, "EndDrawing"));
        const rlCloseWindow: *const fn () callconv(.c) void = @ptrCast(sd.got.fns.dlsym(raylib, "CloseWindow"));

        rlInitWindow(1280, 720, "test");
        rlSetTargetFPS(60);

        var t: f32 = 0.0;
        var d: f32 = 1.0;
        const left_border = 10.0;
        const right_border = 1260.0;
        var p: rlVector2 = .{ .x = left_border, .y = 200.0 };

        while (!rlWindowShouldClose()) {
            const dt = rlGetFrameTime();

            t += dt;
            p.x += d * 200.0 * dt;
            if (right_border < p.x) {
                d *= -1.0;
                p.x = right_border;
            }
            if (p.x < left_border) {
                d *= -1.0;
                p.x = left_border;
            }
            p.y = sinf(20.0 * t) + 700.0;

            rlBeginDrawing();
            rlClearBackground(RL_BLACK);
            rlDrawCircleV(p, 10.0, RL_RED);
            rlDrawText("static_dynamic demo", 120.0, 250.0, 100.0, RL_RED);
            rlEndDrawing();
        }
        rlCloseWindow();
        printf2("End of the demo\n");
        _ = sd.got.fns.dlclose(raylib);
    } else {
        printf2("Cannot load raylib, so no demo\n");
    }
    return 0;
}
