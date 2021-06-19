const std = @import("std");
const c = @import("c.zig");

const Context = c.context_t;

pub fn main() void {
    std.log.info("{d}", .{@sizeOf(Context)});
}