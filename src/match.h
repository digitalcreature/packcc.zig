#ifndef __MATCH_H
#define __MATCH_H

#include "common.h"
#include "context.h"

bool_t match_eof(context_t *ctx);
bool_t match_eol(context_t *ctx);
bool_t match_character(context_t *ctx, char ch);
bool_t match_character_range(context_t *ctx, char min, char max);
bool_t match_character_set(context_t *ctx, const char *chs);
bool_t match_character_any(context_t *ctx);
bool_t match_string(context_t *ctx, const char *str);
bool_t match_blank(context_t *ctx);
bool_t match_section_line_(context_t *ctx, const char *head);
bool_t match_section_line_continuable_(context_t *ctx, const char *head);
bool_t match_section_block_(context_t *ctx, const char *left, const char *right, const char *name);
bool_t match_quotation_(context_t *ctx, const char *left, const char *right, const char *name);
bool_t match_directive_c(context_t *ctx);
bool_t match_comment(context_t *ctx);
bool_t match_comment_c(context_t *ctx);
bool_t match_comment_cxx(context_t *ctx);
bool_t match_quotation_single(context_t *ctx);
bool_t match_quotation_double(context_t *ctx);
bool_t match_character_class(context_t *ctx);
bool_t match_spaces(context_t *ctx);
bool_t match_number(context_t *ctx);
bool_t match_identifier(context_t *ctx);
bool_t match_code_block(context_t *ctx);
bool_t match_footer_start(context_t *ctx);

#endif
