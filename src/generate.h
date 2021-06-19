#ifndef __GENERATE_H
#define __GENERATE_H

#include "common.h"
#include "context.h"

typedef struct generate_tag {
    FILE *stream;
    const node_t *rule;
    int label;
    bool_t ascii;
} generate_t;

typedef enum code_reach_tag {
    CODE_REACH__BOTH = 0,
    CODE_REACH__ALWAYS_SUCCEED = 1,
    CODE_REACH__ALWAYS_FAIL = -1
} code_reach_t;

code_reach_t generate_matching_string_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare);
code_reach_t generate_matching_charclass_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare);
code_reach_t generate_matching_utf8_charclass_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare);
code_reach_t generate_quantifying_code(generate_t *gen, const node_t *expr, int min, int max, int onfail, size_t indent, bool_t bare);
code_reach_t generate_predicating_code(generate_t *gen, const node_t *expr, bool_t neg, int onfail, size_t indent, bool_t bare);
code_reach_t generate_sequential_code(generate_t *gen, const node_array_t *nodes, int onfail, size_t indent, bool_t bare);
code_reach_t generate_alternative_code(generate_t *gen, const node_array_t *nodes, int onfail, size_t indent, bool_t bare);
code_reach_t generate_capturing_code(generate_t *gen, const node_t *expr, size_t index, int onfail, size_t indent, bool_t bare);
code_reach_t generate_expanding_code(generate_t *gen, size_t index, int onfail, size_t indent, bool_t bare);

code_reach_t generate_thunking_action_code(
    generate_t *gen, size_t index, const node_const_array_t *vars, const node_const_array_t *capts, bool_t error, int onfail, size_t indent, bool_t bare
);
code_reach_t generate_thunking_error_code(
    generate_t *gen, const node_t *expr, size_t index, const node_const_array_t *vars, const node_const_array_t *capts, int onfail, size_t indent, bool_t bare
);

code_reach_t generate_code(generate_t *gen, const node_t *node, int onfail, size_t indent, bool_t bare);
bool_t generate(context_t *ctx);

#endif