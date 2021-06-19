#ifndef __PARSE_H
#define __PARSE_H

#include "common.h"
#include "context.h"
#include "match.h"
#include "node.h"

node_t *parse_primary(context_t *ctx, node_t *rule);
node_t *parse_term(context_t *ctx, node_t *rule);
node_t *parse_sequence(context_t *ctx, node_t *rule);
node_t *parse_expression(context_t *ctx, node_t *rule);
node_t *parse_rule(context_t *ctx);
bool_t parse_directive_include_(context_t *ctx, const char *name, char_array_t *output1, char_array_t *output2);
bool_t parse_directive_string_(context_t *ctx, const char *name, char **output, string_flag_t mode);
bool_t parse(context_t *ctx);

#endif
