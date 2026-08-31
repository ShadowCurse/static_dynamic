const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // tests cannot access `static_dynamic.zig` from the root, so create a module for them
    const sd_mod = b.createModule(.{
        .root_source_file = b.path("static_dynamic.zig"),
        .target = target,
        .optimize = optimize,
    });

    const exe = create_exe(b, target, optimize, sd_mod, "zig_test", "static_dynamic_test.zig", null);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    const tests_step = b.step("tests", "Build the Zig test programs");
    // Tests that run with the linker loaded normally.
    for ([_][]const u8{
        "linker",
        "no_fallback_thread_crash",
        "pthread",
        "stack_trace_crash",
        "std",
        "std_thread",
        "std_thread_libc_crash",
    }) |name| {
        const t_exe = create_exe(
            b,
            target,
            optimize,
            sd_mod,
            name,
            b.fmt("tests_zig/{s}.zig", .{name}),
            null,
        );
        tests_step.dependOn(&b.addInstallArtifact(t_exe, .{}).step);
    }

    // Tests where dynamic linker loading fails because bounce binary does not exist
    for ([_][]const u8{
        "fallback",
    }) |name| {
        const t_exe = create_exe(
            b,
            target,
            optimize,
            sd_mod,
            name,
            b.fmt("tests_zig/{s}.zig", .{name}),
            "blah",
        );
        tests_step.dependOn(&b.addInstallArtifact(t_exe, .{}).step);
    }
}

fn create_exe(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    sd_mod: *std.Build.Module,
    name: []const u8,
    root: []const u8,
    bounce_binary: ?[]const u8,
) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .root_source_file = b.path(root),
        .target = target,
        .optimize = optimize,
        .imports = &.{.{ .name = "static_dynamic", .module = sd_mod }},
        // When trying to link with musl, zig seems to always link crt1.o as well which also defines
        // `_start` symbol and this causes the symbol collision
        .link_libc = false,
    });

    const sd_c = b.addWriteFiles().add("static_dynamic.c",
        \\#include "static_dynamic.h"
    );
    var flags: std.ArrayList([]const u8) = .empty;
    flags.append(b.allocator, "-fno-stack-protector") catch @panic("OOM");
    if (bounce_binary) |path| {
        flags.append(b.allocator, b.fmt("-DSD_BOUNCE_BINARY=\"{s}\"", .{path})) catch @panic("OOM");
    }
    mod.addCSourceFile(.{
        .file = sd_c,
        .flags = flags.items,
    });
    mod.addIncludePath(b.path("."));

    return b.addExecutable(.{ .name = name, .root_module = mod });
}
