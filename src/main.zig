const std = @import("std");

// extern fn c_main(argc: c_int, argv: ?[*]?[*]u8) c_int;
extern fn c_print_version() void;

pub fn main() void {
    c_print_version();
}