const Builder = @import("std").build.Builder;
const std = @import("std");
const fs = std.fs;

pub fn build(b: *Builder) !void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const packcc_exe = b.addExecutable("packcc", null);

    packcc_exe.setTarget(target);
    packcc_exe.setBuildMode(mode);
    
    const c_flags = &[_][]const u8{"-std=gnu89"};

    const c_dir = "src/c";
    const c_src_dir = c_dir ++ "/src";
    const c_inc_dir = c_dir ++ "/inc";

    packcc_exe.addIncludeDir(c_inc_dir);

    const Dir = fs.Dir;
    var dir = try fs.cwd().openDir(c_src_dir, Dir.OpenDirOptions{ .iterate = true });
    defer dir.close();
    var iterator = dir.iterate();
    while (try iterator.next()) |entry| {
        if (std.mem.endsWith(u8, entry.name, ".c")) {
            const full_path = try std.fmt.allocPrint(b.allocator, "{s}/{s}", .{c_src_dir, entry.name});
            packcc_exe.addCSourceFile(full_path, c_flags);
        }
    }

    packcc_exe.linkLibC();
    packcc_exe.install();


    const run_cmd = packcc_exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}

