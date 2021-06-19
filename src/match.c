#include "match.h"
#include "util.h"

bool_t match_eof(context_t *ctx) {
    return (refill_buffer(ctx, 1) < 1) ? TRUE : FALSE;
}

bool_t match_eol(context_t *ctx) {
    if (refill_buffer(ctx, 1) >= 1) {
        switch (ctx->buffer.buf[ctx->bufcur]) {
        case '\n':
            ctx->bufcur++;
            ctx->linenum++;
            ctx->linepos = ctx->bufpos + ctx->bufcur;
            return TRUE;
        case '\r':
            ctx->bufcur++;
            if (refill_buffer(ctx, 1) >= 1) {
                if (ctx->buffer.buf[ctx->bufcur] == '\n') ctx->bufcur++;
            }
            ctx->linenum++;
            ctx->linepos = ctx->bufpos + ctx->bufcur;
            return TRUE;
        }
    }
    return FALSE;
}

bool_t match_character(context_t *ctx, char ch) {
    if (refill_buffer(ctx, 1) >= 1) {
        if (ctx->buffer.buf[ctx->bufcur] == ch) {
            ctx->bufcur++;
            return TRUE;
        }
    }
    return FALSE;
}

bool_t match_character_range(context_t *ctx, char min, char max) {
    if (refill_buffer(ctx, 1) >= 1) {
        const char c = ctx->buffer.buf[ctx->bufcur];
        if (c >= min && c <= max) {
            ctx->bufcur++;
            return TRUE;
        }
    }
    return FALSE;
}

bool_t match_character_set(context_t *ctx, const char *chs) {
    if (refill_buffer(ctx, 1) >= 1) {
        const char c = ctx->buffer.buf[ctx->bufcur];
        size_t i;
        for (i = 0; chs[i]; i++) {
            if (c == chs[i]) {
                ctx->bufcur++;
                return TRUE;
            }
        }
    }
    return FALSE;
}

bool_t match_character_any(context_t *ctx) {
    if (refill_buffer(ctx, 1) >= 1) {
        ctx->bufcur++;
        return TRUE;
    }
    return FALSE;
}

bool_t match_string(context_t *ctx, const char *str) {
    const size_t n = strlen(str);
    if (refill_buffer(ctx, n) >= n) {
        if (strncmp(ctx->buffer.buf + ctx->bufcur, str, n) == 0) {
            ctx->bufcur += n;
            return TRUE;
        }
    }
    return FALSE;
}

bool_t match_blank(context_t *ctx) {
    return match_character_set(ctx, " \t\v\f");
}

bool_t match_section_line_(context_t *ctx, const char *head) {
    if (match_string(ctx, head)) {
        while (!match_eol(ctx) && !match_eof(ctx)) match_character_any(ctx);
        return TRUE;
    }
    return FALSE;
}

bool_t match_section_line_continuable_(context_t *ctx, const char *head) {
    if (match_string(ctx, head)) {
        while (!match_eof(ctx)) {
            const size_t p = ctx->bufcur;
            if (match_eol(ctx)) {
                if (ctx->buffer.buf[p - 1] != '\\') break;
            }
            else {
                match_character_any(ctx);
            }
        }
        return TRUE;
    }
    return FALSE;
}

bool_t match_section_block_(context_t *ctx, const char *left, const char *right, const char *name) {
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    if (match_string(ctx, left)) {
        while (!match_string(ctx, right)) {
            if (match_eof(ctx)) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Premature EOF in %s\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
                ctx->errnum++;
                break;
            }
            if (!match_eol(ctx)) match_character_any(ctx);
        }
        return TRUE;
    }
    return FALSE;
}

bool_t match_quotation_(context_t *ctx, const char *left, const char *right, const char *name) {
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    if (match_string(ctx, left)) {
        while (!match_string(ctx, right)) {
            if (match_eof(ctx)) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Premature EOF in %s\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
                ctx->errnum++;
                break;
            }
            if (match_character(ctx, '\\')) {
                if (!match_eol(ctx)) match_character_any(ctx);
            }
            else {
                if (match_eol(ctx)) {
                    print_error("%s:" FMT_LU ":" FMT_LU ": Premature EOL in %s\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
                    ctx->errnum++;
                    break;
                }
                match_character_any(ctx);
            }
        }
        return TRUE;
    }
    return FALSE;
}

bool_t match_directive_c(context_t *ctx) {
    return match_section_line_continuable_(ctx, "#");
}

bool_t match_comment(context_t *ctx) {
    return match_section_line_(ctx, "#");
}

bool_t match_comment_c(context_t *ctx) {
    return match_section_block_(ctx, "/*", "*/", "C comment");
}

bool_t match_comment_cxx(context_t *ctx) {
    return match_section_line_(ctx, "//");
}

bool_t match_quotation_single(context_t *ctx) {
    return match_quotation_(ctx, "\'", "\'", "single quotation");
}

bool_t match_quotation_double(context_t *ctx) {
    return match_quotation_(ctx, "\"", "\"", "double quotation");
}

bool_t match_character_class(context_t *ctx) {
    return match_quotation_(ctx, "[", "]", "character class");
}

bool_t match_spaces(context_t *ctx) {
    size_t n = 0;
    while (match_blank(ctx) || match_eol(ctx) || match_comment(ctx)) n++;
    return (n > 0) ? TRUE : FALSE;
}

bool_t match_number(context_t *ctx) {
    if (match_character_range(ctx, '0', '9')) {
        while (match_character_range(ctx, '0', '9'));
        return TRUE;
    }
    return FALSE;
}

bool_t match_identifier(context_t *ctx) {
    if (
        match_character_range(ctx, 'a', 'z') ||
        match_character_range(ctx, 'A', 'Z') ||
        match_character(ctx, '_')
    ) {
        while (
            match_character_range(ctx, 'a', 'z') ||
            match_character_range(ctx, 'A', 'Z') ||
            match_character_range(ctx, '0', '9') ||
            match_character(ctx, '_')
        );
        return TRUE;
    }
    return FALSE;
}

bool_t match_code_block(context_t *ctx) {
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    if (match_character(ctx, '{')) {
        int d = 1;
        for (;;) {
            if (match_eof(ctx)) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Premature EOF in code block\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
                ctx->errnum++;
                break;
            }
            if (
                match_directive_c(ctx) ||
                match_comment_c(ctx) ||
                match_comment_cxx(ctx) ||
                match_quotation_single(ctx) ||
                match_quotation_double(ctx)
            ) continue;
            if (match_character(ctx, '{')) {
                d++;
            }
            else if (match_character(ctx, '}')) {
                d--;
                if (d == 0) break;
            }
            else {
                if (!match_eol(ctx)) {
                    if (match_character(ctx, '$')) {
                        ctx->buffer.buf[ctx->bufcur - 1] = '_';
                    }
                    else {
                        match_character_any(ctx);
                    }
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

bool_t match_footer_start(context_t *ctx) {
    return match_string(ctx, "%%");
}