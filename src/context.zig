const c = @import("c.zig");

pub const Context = struct {

    ptr: *c.context_t,

    const Self = @This();

    pub fn init(in_filename: [:0]const u8, out_filename: [:0]const u8) Self {
        var ptr = c.create_context(in_filename, out_filename, true, true);
        return Self {
            .ptr = ptr,
        };
    }

    pub fn deinit(self: *Self) void {
        c.destroy_context(self.ptr);
    }

    pub fn parse(self: *Self) !void {
        if (!c.parse(self.ptr)) {
            return error.ParseFailed;
        }
    }

};