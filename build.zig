const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const mod = b.createModule(.{
        .root_source_file = b.path("static_dynamic_test.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{},
    });

    const sd_c = b.addWriteFiles().add("static_dynamic.c",
        \\#include "static_dynamic.h"
    );
    mod.addCSourceFile(.{ .file = sd_c });
    mod.addIncludePath(b.path("."));

    const exe = b.addExecutable(.{ .name = "zig_test", .root_module = mod });
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
}
