const c = @import("c.zig");

pub const NodeType = enum(c_int) {
    Rule = NODE_RULE,
    Reference =NODE_REFERENCE,
    String = NODE_STRING,
    CharClass = NODE_CHARCLASS,
    Quantity = NODE_QUANTITY,
    Predicate = NODE_PREDICATE,
    Sequence = NODE_SEQUENCE,
    Alternate = NODE_ALTERNATE,
    Capture = NODE_CAPTURE,
    Expand = NODE_EXPAND,
    Action = NODE_ACTION,
    Error = NODE_ERROR,
};

pub const Node = struct {
    
    ptr: *c.node_t,

    const Self = @This();

    pub fn nodeType(self: Self) NodeType {
        return @intToEnum(NodeType, self.ptr.@"type");
    }

};

pub const NodeArray = struct {

    ptr: *c.node_array_t,

    const Self = @This();

    pub fn items(self: *Self) []const *c.node_t {
        return self.ptr.buf[0..self.ptr.len];
    }
    
};