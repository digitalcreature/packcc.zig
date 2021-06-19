#include "generate.h"
#include "match.h"
#include "util.h"

code_reach_t generate_matching_string_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare) {
    const size_t n = (value != NULL) ? strlen(value) : 0;
    if (n > 0) {
        char s[5];
        if (n > 1) {
            size_t i;
            write_characters(gen->stream, ' ', indent);
            fputs_e("if (\n", gen->stream);
            write_characters(gen->stream, ' ', indent + 4);
            fprintf_e(gen->stream, "pcc_refill_buffer(ctx, " FMT_LU ") < " FMT_LU " ||\n", (ulong_t)n, (ulong_t)n);
            for (i = 0; i < n - 1; i++) {
                write_characters(gen->stream, ' ', indent + 4);
                fprintf_e(gen->stream, "(ctx->buffer.buf + ctx->cur)[" FMT_LU "] != '%s' ||\n", (ulong_t)i, escape_character(value[i], &s));
            }
            write_characters(gen->stream, ' ', indent + 4);
            fprintf_e(gen->stream, "(ctx->buffer.buf + ctx->cur)[" FMT_LU "] != '%s'\n", (ulong_t)i, escape_character(value[i], &s));
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, ") goto L%04d;\n", onfail);
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "ctx->cur += " FMT_LU ";\n", (ulong_t)n);
            return CODE_REACH__BOTH;
        }
        else {
            write_characters(gen->stream, ' ', indent);
            fputs_e("if (\n", gen->stream);
            write_characters(gen->stream, ' ', indent + 4);
            fputs_e("pcc_refill_buffer(ctx, 1) < 1 ||\n", gen->stream);
            write_characters(gen->stream, ' ', indent + 4);
            fprintf_e(gen->stream, "ctx->buffer.buf[ctx->cur] != '%s'\n", escape_character(value[0], &s));
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, ") goto L%04d;\n", onfail);
            write_characters(gen->stream, ' ', indent);
            fputs_e("ctx->cur++;\n", gen->stream);
            return CODE_REACH__BOTH;
        }
    }
    else {
        /* no code to generate */
        return CODE_REACH__ALWAYS_SUCCEED;
    }
}

code_reach_t generate_matching_charclass_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare) {
    assert(gen->ascii);
    if (value != NULL) {
        const size_t n = strlen(value);
        if (n > 0) {
            char s[5], t[5];
            if (n > 1) {
                const bool_t a = (value[0] == '^') ? TRUE : FALSE;
                size_t i = a ? 1 : 0;
                if (i + 1 == n) { /* fulfilled only if a == TRUE */
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("if (\n", gen->stream);
                    write_characters(gen->stream, ' ', indent + 4);
                    fputs_e("pcc_refill_buffer(ctx, 1) < 1 ||\n", gen->stream);
                    write_characters(gen->stream, ' ', indent + 4);
                    fprintf_e(gen->stream, "ctx->buffer.buf[ctx->cur] == '%s'\n", escape_character(value[i], &s));
                    write_characters(gen->stream, ' ', indent);
                    fprintf_e(gen->stream, ") goto L%04d;\n", onfail);
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("ctx->cur++;\n", gen->stream);
                    return CODE_REACH__BOTH;
                }
                else {
                    if (!bare) {
                        write_characters(gen->stream, ' ', indent);
                        fputs_e("{\n", gen->stream);
                        indent += 4;
                    }
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("char c;\n", gen->stream);
                    write_characters(gen->stream, ' ', indent);
                    fprintf_e(gen->stream, "if (pcc_refill_buffer(ctx, 1) < 1) goto L%04d;\n", onfail);
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("c = ctx->buffer.buf[ctx->cur];\n", gen->stream);
                    if (i + 3 == n && value[i] != '\\' && value[i + 1] == '-') {
                        write_characters(gen->stream, ' ', indent);
                        fprintf_e(gen->stream,
                            a ? "if (c >= '%s' && c <= '%s') goto L%04d;\n"
                              : "if (!(c >= '%s' && c <= '%s')) goto L%04d;\n",
                            escape_character(value[i], &s), escape_character(value[i + 2], &t), onfail);
                    }
                    else {
                        write_characters(gen->stream, ' ', indent);
                        fputs_e(a ? "if (\n" : "if (!(\n", gen->stream);
                        for (; i < n; i++) {
                            write_characters(gen->stream, ' ', indent + 4);
                            if (value[i] == '\\' && i + 1 < n) i++;
                            if (i + 2 < n && value[i + 1] == '-') {
                                fprintf_e(gen->stream, "(c >= '%s' && c <= '%s')%s\n",
                                    escape_character(value[i], &s), escape_character(value[i + 2], &t), (i + 3 == n) ? "" : " ||");
                                i += 2;
                            }
                            else {
                                fprintf_e(gen->stream, "c == '%s'%s\n",
                                    escape_character(value[i], &s), (i + 1 == n) ? "" : " ||");
                            }
                        }
                        write_characters(gen->stream, ' ', indent);
                        fprintf_e(gen->stream, a ? ") goto L%04d;\n" : ")) goto L%04d;\n", onfail);
                    }
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("ctx->cur++;\n", gen->stream);
                    if (!bare) {
                        indent -= 4;
                        write_characters(gen->stream, ' ', indent);
                        fputs_e("}\n", gen->stream);
                    }
                    return CODE_REACH__BOTH;
                }
            }
            else {
                write_characters(gen->stream, ' ', indent);
                fputs_e("if (\n", gen->stream);
                write_characters(gen->stream, ' ', indent + 4);
                fputs_e("pcc_refill_buffer(ctx, 1) < 1 ||\n", gen->stream);
                write_characters(gen->stream, ' ', indent + 4);
                fprintf_e(gen->stream, "ctx->buffer.buf[ctx->cur] != '%s'\n", escape_character(value[0], &s));
                write_characters(gen->stream, ' ', indent);
                fprintf_e(gen->stream, ") goto L%04d;\n", onfail);
                write_characters(gen->stream, ' ', indent);
                fputs_e("ctx->cur++;\n", gen->stream);
                return CODE_REACH__BOTH;
            }
        }
        else {
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", onfail);
            return CODE_REACH__ALWAYS_FAIL;
        }
    }
    else {
        write_characters(gen->stream, ' ', indent);
        fprintf_e(gen->stream, "if (pcc_refill_buffer(ctx, 1) < 1) goto L%04d;\n", onfail);
        write_characters(gen->stream, ' ', indent);
        fputs_e("ctx->cur++;\n", gen->stream);
        return CODE_REACH__BOTH;
    }
}

code_reach_t generate_matching_utf8_charclass_code(generate_t *gen, const char *value, int onfail, size_t indent, bool_t bare) {
    const size_t n = (value != NULL) ? strlen(value) : 0;
    if (value == NULL || n > 0) {
        const bool_t a = (n > 0 && value[0] == '^') ? TRUE : FALSE;
        size_t i = a ? 1 : 0;
        if (!bare) {
            write_characters(gen->stream, ' ', indent);
            fputs_e("{\n", gen->stream);
            indent += 4;
        }
        write_characters(gen->stream, ' ', indent);
        fputs_e("int u;\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fputs_e("const size_t n = pcc_get_char_as_utf32(ctx, &u);\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fprintf_e(gen->stream, "if (n == 0) goto L%04d;\n", onfail);
        if (value != NULL && !(a && n == 1)) { /* not '.' or '[^]' */
            int u0 = 0;
            bool_t r = FALSE;
            write_characters(gen->stream, ' ', indent);
            fputs_e(a ? "if (\n" : "if (!(\n", gen->stream);
            while (i < n) {
                int u = 0;
                if (value[i] == '\\' && i + 1 < n) i++;
                i += utf8_to_utf32(value + i, &u);
                if (r) { /* character range */
                    write_characters(gen->stream, ' ', indent + 4);
                    fprintf_e(gen->stream, "(u >= 0x%06x && u <= 0x%06x)%s\n", u0, u, (i < n) ? " ||" : "");
                    u0 = 0;
                    r = FALSE;
                }
                else if (value[i] != '-') { /* single character */
                    write_characters(gen->stream, ' ', indent + 4);
                    fprintf_e(gen->stream, "u == 0x%06x%s\n", u, (i < n) ? " ||" : "");
                    u0 = 0;
                    r = FALSE;
                }
                else {
                    assert(value[i] == '-');
                    i++;
                    u0 = u;
                    r = TRUE;
                }
            }
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, a ? ") goto L%04d;\n" : ")) goto L%04d;\n", onfail);
        }
        write_characters(gen->stream, ' ', indent);
        fputs_e("ctx->cur += n;\n", gen->stream);
        if (!bare) {
            indent -= 4;
            write_characters(gen->stream, ' ', indent);
            fputs_e("}\n", gen->stream);
        }
        return CODE_REACH__BOTH;
    }
    else {
        write_characters(gen->stream, ' ', indent);
        fprintf_e(gen->stream, "goto L%04d;\n", onfail);
        return CODE_REACH__ALWAYS_FAIL;
    }
}

code_reach_t generate_quantifying_code(generate_t *gen, const node_t *expr, int min, int max, int onfail, size_t indent, bool_t bare) {
    if (max > 1 || max < 0) {
        code_reach_t r;
        if (!bare) {
            write_characters(gen->stream, ' ', indent);
            fputs_e("{\n", gen->stream);
            indent += 4;
        }
        if (min > 0) {
            write_characters(gen->stream, ' ', indent);
            fputs_e("const size_t p0 = ctx->cur;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("const size_t n0 = chunk->thunks.len;\n", gen->stream);
        }
        write_characters(gen->stream, ' ', indent);
        fputs_e("int i;\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        if (max < 0)
            fputs_e("for (i = 0;; i++) {\n", gen->stream);
        else
            fprintf_e(gen->stream, "for (i = 0; i < %d; i++) {\n", max);
        write_characters(gen->stream, ' ', indent + 4);
        fputs_e("const size_t p = ctx->cur;\n", gen->stream);
        write_characters(gen->stream, ' ', indent + 4);
        fputs_e("const size_t n = chunk->thunks.len;\n", gen->stream);
        {
            const int l = ++gen->label;
            r = generate_code(gen, expr, l, indent + 4, FALSE);
            write_characters(gen->stream, ' ', indent + 4);
            fputs_e("if (ctx->cur == p) break;\n", gen->stream);
            if (r != CODE_REACH__ALWAYS_SUCCEED) {
                write_characters(gen->stream, ' ', indent + 4);
                fputs_e("continue;\n", gen->stream);
                write_characters(gen->stream, ' ', indent);
                fprintf_e(gen->stream, "L%04d:;\n", l);
                write_characters(gen->stream, ' ', indent + 4);
                fputs_e("ctx->cur = p;\n", gen->stream);
                write_characters(gen->stream, ' ', indent + 4);
                fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
                write_characters(gen->stream, ' ', indent + 4);
                fputs_e("break;\n", gen->stream);
            }
        }
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
        if (min > 0) {
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "if (i < %d) {\n", min);
            write_characters(gen->stream, ' ', indent + 4);
            fputs_e("ctx->cur = p0;\n", gen->stream);
            write_characters(gen->stream, ' ', indent + 4);
            fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n0);\n", gen->stream);
            write_characters(gen->stream, ' ', indent + 4);
            fprintf_e(gen->stream, "goto L%04d;\n", onfail);
            write_characters(gen->stream, ' ', indent);
            fputs_e("}\n", gen->stream);
        }
        if (!bare) {
            indent -= 4;
            write_characters(gen->stream, ' ', indent);
            fputs_e("}\n", gen->stream);
        }
        return (min > 0) ? ((r == CODE_REACH__ALWAYS_FAIL) ? CODE_REACH__ALWAYS_FAIL : CODE_REACH__BOTH) : CODE_REACH__ALWAYS_SUCCEED;
    }
    else if (max == 1) {
        if (min > 0) {
            return generate_code(gen, expr, onfail, indent, bare);
        }
        else {
            if (!bare) {
                write_characters(gen->stream, ' ', indent);
                fputs_e("{\n", gen->stream);
                indent += 4;
            }
            write_characters(gen->stream, ' ', indent);
            fputs_e("const size_t p = ctx->cur;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("const size_t n = chunk->thunks.len;\n", gen->stream);
            {
                const int l = ++gen->label;
                if (generate_code(gen, expr, l, indent, FALSE) != CODE_REACH__ALWAYS_SUCCEED) {
                    const int m = ++gen->label;
                    write_characters(gen->stream, ' ', indent);
                    fprintf_e(gen->stream, "goto L%04d;\n", m);
                    if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
                    fprintf_e(gen->stream, "L%04d:;\n", l);
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("ctx->cur = p;\n", gen->stream);
                    write_characters(gen->stream, ' ', indent);
                    fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
                    if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
                    fprintf_e(gen->stream, "L%04d:;\n", m);
                }
            }
            if (!bare) {
                indent -= 4;
                write_characters(gen->stream, ' ', indent);
                fputs_e("}\n", gen->stream);
            }
            return CODE_REACH__ALWAYS_SUCCEED;
        }
    }
    else {
        /* no code to generate */
        return CODE_REACH__ALWAYS_SUCCEED;
    }
}

code_reach_t generate_predicating_code(generate_t *gen, const node_t *expr, bool_t neg, int onfail, size_t indent, bool_t bare) {
    code_reach_t r;
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    write_characters(gen->stream, ' ', indent);
    fputs_e("const size_t p = ctx->cur;\n", gen->stream);
    write_characters(gen->stream, ' ', indent);
    fputs_e("const size_t n = chunk->thunks.len;\n", gen->stream);
    if (neg) {
        const int l = ++gen->label;
        r = generate_code(gen, expr, l, indent, FALSE);
        if (r != CODE_REACH__ALWAYS_FAIL) {
            write_characters(gen->stream, ' ', indent);
            fputs_e("ctx->cur = p;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", onfail);
        }
        if (r != CODE_REACH__ALWAYS_SUCCEED) {
            if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
            fprintf_e(gen->stream, "L%04d:;\n", l);
            write_characters(gen->stream, ' ', indent);
            fputs_e("ctx->cur = p;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
        }
        switch (r) {
        case CODE_REACH__ALWAYS_SUCCEED: r = CODE_REACH__ALWAYS_FAIL; break;
        case CODE_REACH__ALWAYS_FAIL: r = CODE_REACH__ALWAYS_SUCCEED; break;
        case CODE_REACH__BOTH: break;
        }
    }
    else {
        const int l = ++gen->label;
        const int m = ++gen->label;
        r = generate_code(gen, expr, l, indent, FALSE);
        if (r != CODE_REACH__ALWAYS_FAIL) {
            write_characters(gen->stream, ' ', indent);
            fputs_e("ctx->cur = p;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
        }
        if (r == CODE_REACH__BOTH) {
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", m);
        }
        if (r != CODE_REACH__ALWAYS_SUCCEED) {
            if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
            fprintf_e(gen->stream, "L%04d:;\n", l);
            write_characters(gen->stream, ' ', indent);
            fputs_e("ctx->cur = p;\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", onfail);
        }
        if (r == CODE_REACH__BOTH) {
            if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
            fprintf_e(gen->stream, "L%04d:;\n", m);
        }
    }
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return r;
}

code_reach_t generate_sequential_code(generate_t *gen, const node_array_t *nodes, int onfail, size_t indent, bool_t bare) {
    bool_t b = FALSE;
    size_t i;
    for (i = 0; i < nodes->len; i++) {
        switch (generate_code(gen, nodes->buf[i], onfail, indent, FALSE)) {
        case CODE_REACH__ALWAYS_FAIL:
            if (i + 1 < nodes->len) {
                write_characters(gen->stream, ' ', indent);
                fputs_e("/* unreachable codes omitted */\n", gen->stream);
            }
            return CODE_REACH__ALWAYS_FAIL;
        case CODE_REACH__ALWAYS_SUCCEED:
            break;
        default:
            b = TRUE;
        }
    }
    return b ? CODE_REACH__BOTH : CODE_REACH__ALWAYS_SUCCEED;
}

code_reach_t generate_alternative_code(generate_t *gen, const node_array_t *nodes, int onfail, size_t indent, bool_t bare) {
    bool_t b = FALSE;
    int m = ++gen->label;
    size_t i;
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    write_characters(gen->stream, ' ', indent);
    fputs_e("const size_t p = ctx->cur;\n", gen->stream);
    write_characters(gen->stream, ' ', indent);
    fputs_e("const size_t n = chunk->thunks.len;\n", gen->stream);
    for (i = 0; i < nodes->len; i++) {
        const bool_t c = (i + 1 < nodes->len) ? TRUE : FALSE;
        const int l = ++gen->label;
        switch (generate_code(gen, nodes->buf[i], l, indent, FALSE)) {
        case CODE_REACH__ALWAYS_SUCCEED:
            if (c) {
                write_characters(gen->stream, ' ', indent);
                fputs_e("/* unreachable codes omitted */\n", gen->stream);
            }
            if (b) {
                if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
                fprintf_e(gen->stream, "L%04d:;\n", m);
            }
            if (!bare) {
                indent -= 4;
                write_characters(gen->stream, ' ', indent);
                fputs_e("}\n", gen->stream);
            }
            return CODE_REACH__ALWAYS_SUCCEED;
        case CODE_REACH__ALWAYS_FAIL:
            break;
        default:
            b = TRUE;
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", m);
        }
        if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
        fprintf_e(gen->stream, "L%04d:;\n", l);
        write_characters(gen->stream, ' ', indent);
        fputs_e("ctx->cur = p;\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fputs_e("pcc_thunk_array__revert(ctx->auxil, &chunk->thunks, n);\n", gen->stream);
        if (!c) {
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "goto L%04d;\n", onfail);
        }
    }
    if (b) {
        if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
        fprintf_e(gen->stream, "L%04d:;\n", m);
    }
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return b ? CODE_REACH__BOTH : CODE_REACH__ALWAYS_FAIL;
}

code_reach_t generate_capturing_code(generate_t *gen, const node_t *expr, size_t index, int onfail, size_t indent, bool_t bare) {
    code_reach_t r;
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    write_characters(gen->stream, ' ', indent);
    fputs_e("const size_t p = ctx->cur;\n", gen->stream);
    write_characters(gen->stream, ' ', indent);
    fputs_e("size_t q;\n", gen->stream);
    r = generate_code(gen, expr, onfail, indent, FALSE);
    write_characters(gen->stream, ' ', indent);
    fputs_e("q = ctx->cur;\n", gen->stream);
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "chunk->capts.buf[" FMT_LU "].range.start = p;\n", (ulong_t)index);
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "chunk->capts.buf[" FMT_LU "].range.end = q;\n", (ulong_t)index);
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return r;
}

code_reach_t generate_expanding_code(generate_t *gen, size_t index, int onfail, size_t indent, bool_t bare) {
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "const size_t n = chunk->capts.buf[" FMT_LU "].range.end - chunk->capts.buf[" FMT_LU "].range.start;\n", (ulong_t)index, (ulong_t)index);
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "if (pcc_refill_buffer(ctx, n) < n) goto L%04d;\n", onfail);
    write_characters(gen->stream, ' ', indent);
    fputs_e("if (n > 0) {\n", gen->stream);
    write_characters(gen->stream, ' ', indent + 4);
    fputs_e("const char *const p = ctx->buffer.buf + ctx->cur;\n", gen->stream);
    write_characters(gen->stream, ' ', indent + 4);
    fprintf_e(gen->stream, "const char *const q = ctx->buffer.buf + chunk->capts.buf[" FMT_LU "].range.start;\n", (ulong_t)index);
    write_characters(gen->stream, ' ', indent + 4);
    fputs_e("size_t i;\n", gen->stream);
    write_characters(gen->stream, ' ', indent + 4);
    fputs_e("for (i = 0; i < n; i++) {\n", gen->stream);
    write_characters(gen->stream, ' ', indent + 8);
    fprintf_e(gen->stream, "if (p[i] != q[i]) goto L%04d;\n", onfail);
    write_characters(gen->stream, ' ', indent + 4);
    fputs_e("}\n", gen->stream);
    write_characters(gen->stream, ' ', indent + 4);
    fputs_e("ctx->cur += n;\n", gen->stream);
    write_characters(gen->stream, ' ', indent);
    fputs_e("}\n", gen->stream);
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return CODE_REACH__BOTH;
}

code_reach_t generate_thunking_action_code(
    generate_t *gen, size_t index, const node_const_array_t *vars, const node_const_array_t *capts, bool_t error, int onfail, size_t indent, bool_t bare
) {
    assert(gen->rule->type == NODE_RULE);
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    if (error) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("pcc_value_t null;\n", gen->stream);
    }
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "pcc_thunk_t *const thunk = pcc_thunk__create_leaf(ctx->auxil, pcc_action_%s_" FMT_LU ", " FMT_LU ", " FMT_LU ");\n",
        gen->rule->data.rule.name, (ulong_t)index, (ulong_t)gen->rule->data.rule.vars.len, (ulong_t)gen->rule->data.rule.capts.len);
    {
        size_t i;
        for (i = 0; i < vars->len; i++) {
            assert(vars->buf[i]->type == NODE_REFERENCE);
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "thunk->data.leaf.values.buf[" FMT_LU "] = &(chunk->values.buf[" FMT_LU "]);\n",
                (ulong_t)vars->buf[i]->data.reference.index, (ulong_t)vars->buf[i]->data.reference.index);
        }
        for (i = 0; i < capts->len; i++) {
            assert(capts->buf[i]->type == NODE_CAPTURE);
            write_characters(gen->stream, ' ', indent);
            fprintf_e(gen->stream, "thunk->data.leaf.capts.buf[" FMT_LU "] = &(chunk->capts.buf[" FMT_LU "]);\n",
                (ulong_t)capts->buf[i]->data.capture.index, (ulong_t)capts->buf[i]->data.capture.index);
        }
        write_characters(gen->stream, ' ', indent);
        fputs_e("thunk->data.leaf.capt0.range.start = chunk->pos;\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fputs_e("thunk->data.leaf.capt0.range.end = ctx->cur;\n", gen->stream);
    }
    if (error) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("memset(&null, 0, sizeof(pcc_value_t)); /* in case */\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fputs_e("thunk->data.leaf.action(ctx, thunk, &null);\n", gen->stream);
        write_characters(gen->stream, ' ', indent);
        fputs_e("pcc_thunk__destroy(ctx->auxil, thunk);\n", gen->stream);
    }
    else {
        write_characters(gen->stream, ' ', indent);
        fputs_e("pcc_thunk_array__add(ctx->auxil, &chunk->thunks, thunk);\n", gen->stream);
    }
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return CODE_REACH__ALWAYS_SUCCEED;
}

code_reach_t generate_thunking_error_code(
    generate_t *gen, const node_t *expr, size_t index, const node_const_array_t *vars, const node_const_array_t *capts, int onfail, size_t indent, bool_t bare
) {
    code_reach_t r;
    const int l = ++gen->label;
    const int m = ++gen->label;
    assert(gen->rule->type == NODE_RULE);
    if (!bare) {
        write_characters(gen->stream, ' ', indent);
        fputs_e("{\n", gen->stream);
        indent += 4;
    }
    r = generate_code(gen, expr, l, indent, TRUE);
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "goto L%04d;\n", m);
    if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
    fprintf_e(gen->stream, "L%04d:;\n", l);
    generate_thunking_action_code(gen, index, vars, capts, TRUE, l, indent, FALSE);
    write_characters(gen->stream, ' ', indent);
    fprintf_e(gen->stream, "goto L%04d;\n", onfail);
    if (indent > 4) write_characters(gen->stream, ' ', indent - 4);
    fprintf_e(gen->stream, "L%04d:;\n", m);
    if (!bare) {
        indent -= 4;
        write_characters(gen->stream, ' ', indent);
        fputs_e("}\n", gen->stream);
    }
    return r;
}

code_reach_t generate_code(generate_t *gen, const node_t *node, int onfail, size_t indent, bool_t bare) {
    if (node == NULL) {
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
    switch (node->type) {
    case NODE_RULE:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    case NODE_REFERENCE:
        write_characters(gen->stream, ' ', indent);
        if (node->data.reference.index != VOID_VALUE) {
            fprintf_e(gen->stream, "if (!pcc_apply_rule(ctx, pcc_evaluate_rule_%s, &chunk->thunks, &(chunk->values.buf[" FMT_LU "]))) goto L%04d;\n",
                node->data.reference.name, (ulong_t)node->data.reference.index, onfail);
        }
        else {
            fprintf_e(gen->stream, "if (!pcc_apply_rule(ctx, pcc_evaluate_rule_%s, &chunk->thunks, NULL)) goto L%04d;\n",
                node->data.reference.name, onfail);
        }
        return CODE_REACH__BOTH;
    case NODE_STRING:
        return generate_matching_string_code(gen, node->data.string.value, onfail, indent, bare);
    case NODE_CHARCLASS:
        return gen->ascii ?
               generate_matching_charclass_code(gen, node->data.charclass.value, onfail, indent, bare) :
               generate_matching_utf8_charclass_code(gen, node->data.charclass.value, onfail, indent, bare);
    case NODE_QUANTITY:
        return generate_quantifying_code(gen, node->data.quantity.expr, node->data.quantity.min, node->data.quantity.max, onfail, indent, bare);
    case NODE_PREDICATE:
        return generate_predicating_code(gen, node->data.predicate.expr, node->data.predicate.neg, onfail, indent, bare);
    case NODE_SEQUENCE:
        return generate_sequential_code(gen, &node->data.sequence.nodes, onfail, indent, bare);
    case NODE_ALTERNATE:
        return generate_alternative_code(gen, &node->data.alternate.nodes, onfail, indent, bare);
    case NODE_CAPTURE:
        return generate_capturing_code(gen, node->data.capture.expr, node->data.capture.index, onfail, indent, bare);
    case NODE_EXPAND:
        return generate_expanding_code(gen, node->data.expand.index, onfail, indent, bare);
    case NODE_ACTION:
        return generate_thunking_action_code(
            gen, node->data.action.index, &node->data.action.vars, &node->data.action.capts, FALSE, onfail, indent, bare
        );
    case NODE_ERROR:
        return generate_thunking_error_code(
            gen, node->data.error.expr, node->data.error.index, &node->data.error.vars, &node->data.error.capts, onfail, indent, bare
        );
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
}

bool_t generate(context_t *ctx) {
    const char *const vt = get_value_type(ctx);
    const char *const at = get_auxil_type(ctx);
    const bool_t vp = is_pointer_type(vt);
    const bool_t ap = is_pointer_type(at);
    FILE *const sstream = fopen_wt_e(ctx->sname);
    FILE *const hstream = fopen_wt_e(ctx->hname);
    fprintf_e(sstream, "/* A packrat parser generated by PackCC %s */\n\n", VERSION);
    fprintf_e(hstream, "/* A packrat parser generated by PackCC %s */\n\n", VERSION);
    {
        write_code_block(hstream, ctx->eheader.buf, ctx->eheader.len, 0);
        fprintf_e(
            hstream,
            "#ifndef PCC_INCLUDED_%s\n"
            "#define PCC_INCLUDED_%s\n"
            "\n",
            ctx->hid, ctx->hid
        );
        write_code_block(hstream, ctx->header.buf, ctx->header.len, 0);
    }
    {
        write_code_block(sstream, ctx->esource.buf, ctx->esource.len, 0);
        fputs_e(
            "#ifdef _MSC_VER\n"
            "#undef _CRT_SECURE_NO_WARNINGS\n"
            "#define _CRT_SECURE_NO_WARNINGS\n"
            "#endif /* _MSC_VER */\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "#include <string.h>\n"
            "\n"
            "#ifndef _MSC_VER\n"
            "#if defined __GNUC__ && defined _WIN32 /* MinGW */\n"
            "#ifndef PCC_USE_SYSTEM_STRNLEN\n"
            "#define strnlen(str, maxlen) pcc_strnlen(str, maxlen)\n"
            "static size_t pcc_strnlen(const char *str, size_t maxlen) {\n"
            "    size_t i;\n"
            "    for (i = 0; i < maxlen && str[i]; i++);\n"
            "    return i;\n"
            "}\n"
            "#endif /* !PCC_USE_SYSTEM_STRNLEN */\n"
            "#endif /* defined __GNUC__ && defined _WIN32 */\n"
            "#endif /* !_MSC_VER */\n"
            "\n"
            "#define PCC_DBG_EVALUATE 0\n"
            "#define PCC_DBG_MATCH    1\n"
            "#define PCC_DBG_NOMATCH  2\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "#include \"%s\"\n"
            "\n",
            ctx->hname
        );
        write_code_block(sstream, ctx->source.buf, ctx->source.len, 0);
    }
    {
        fputs_e(
            "#ifndef PCC_BUFFERSIZE\n"
            "#define PCC_BUFFERSIZE 256\n"
            "#endif /* !PCC_BUFFERSIZE */\n"
            "\n"
            "#ifndef PCC_ARRAYSIZE\n"
            "#define PCC_ARRAYSIZE 2\n"
            "#endif /* !PCC_ARRAYSIZE */\n"
            "\n"
            "#define VOID_VALUE (~(size_t)0)\n"
            "\n"
            "typedef enum pcc_bool_tag {\n"
            "    PCC_FALSE = 0,\n"
            "    PCC_TRUE\n"
            "} pcc_bool_t;\n"
            "\n"
            "typedef struct pcc_char_array_tag {\n"
            "    char *buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_char_array_t;\n"
            "\n"
            "typedef struct pcc_range_tag {\n"
            "    size_t start;\n"
            "    size_t end;\n"
            "} pcc_range_t;\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "typedef %s%spcc_value_t;\n"
            "\n",
            vt, vp ? "" : " "
        );
        fprintf_e(
            sstream,
            "typedef %s%spcc_auxil_t;\n"
            "\n",
            at, ap ? "" : " "
        );
        fputs_e(
            "typedef struct pcc_value_table_tag {\n"
            "    pcc_value_t *buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_value_table_t;\n"
            "\n"
            "typedef struct pcc_value_refer_table_tag {\n"
            "    pcc_value_t **buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_value_refer_table_t;\n"
            "\n"
            "typedef struct pcc_capture_tag {\n"
            "    pcc_range_t range;\n"
            "    char *string; /* mutable */\n"
            "} pcc_capture_t;\n"
            "\n"
            "typedef struct pcc_capture_table_tag {\n"
            "    pcc_capture_t *buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_capture_table_t;\n"
            "\n"
            "typedef struct pcc_capture_const_table_tag {\n"
            "    const pcc_capture_t **buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_capture_const_table_t;\n"
            "\n"
            "typedef struct pcc_thunk_tag pcc_thunk_t;\n"
            "typedef struct pcc_thunk_array_tag pcc_thunk_array_t;\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "typedef void (*pcc_action_t)(%s_context_t *, pcc_thunk_t *, pcc_value_t *);\n"
            "\n",
            get_prefix(ctx)
        );
        fputs_e(
            "typedef enum pcc_thunk_type_tag {\n"
            "    PCC_THUNK_LEAF,\n"
            "    PCC_THUNK_NODE\n"
            "} pcc_thunk_type_t;\n"
            "\n"
            "typedef struct pcc_thunk_leaf_tag {\n"
            "    pcc_value_refer_table_t values;\n"
            "    pcc_capture_const_table_t capts;\n"
            "    pcc_capture_t capt0;\n"
            "    pcc_action_t action;\n"
            "} pcc_thunk_leaf_t;\n"
            "\n"
            "typedef struct pcc_thunk_node_tag {\n"
            "    const pcc_thunk_array_t *thunks; /* just a reference */\n"
            "    pcc_value_t *value; /* just a reference */\n"
            "} pcc_thunk_node_t;\n"
            "\n"
            "typedef union pcc_thunk_data_tag {\n"
            "    pcc_thunk_leaf_t leaf;\n"
            "    pcc_thunk_node_t node;\n"
            "} pcc_thunk_data_t;\n"
            "\n"
            "struct pcc_thunk_tag {\n"
            "    pcc_thunk_type_t type;\n"
            "    pcc_thunk_data_t data;\n"
            "};\n"
            "\n"
            "struct pcc_thunk_array_tag {\n"
            "    pcc_thunk_t **buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "};\n"
            "\n"
            "typedef struct pcc_thunk_chunk_tag {\n"
            "    pcc_value_table_t values;\n"
            "    pcc_capture_table_t capts;\n"
            "    pcc_thunk_array_t thunks;\n"
            "    size_t pos; /* the starting position in the character buffer */\n"
            "} pcc_thunk_chunk_t;\n"
            "\n"
            "typedef struct pcc_lr_entry_tag pcc_lr_entry_t;\n"
            "\n"
            "typedef enum pcc_lr_answer_type_tag {\n"
            "    PCC_LR_ANSWER_LR,\n"
            "    PCC_LR_ANSWER_CHUNK\n"
            "} pcc_lr_answer_type_t;\n"
            "\n"
            "typedef union pcc_lr_answer_data_tag {\n"
            "    pcc_lr_entry_t *lr;\n"
            "    pcc_thunk_chunk_t *chunk;\n"
            "} pcc_lr_answer_data_t;\n"
            "\n"
            "typedef struct pcc_lr_answer_tag pcc_lr_answer_t;\n"
            "\n"
            "struct pcc_lr_answer_tag {\n"
            "    pcc_lr_answer_type_t type;\n"
            "    pcc_lr_answer_data_t data;\n"
            "    size_t pos; /* the absolute position in the input */\n"
            "    pcc_lr_answer_t *hold;\n"
            "};\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "typedef pcc_thunk_chunk_t *(*pcc_rule_t)(%s_context_t *);\n"
            "\n",
            get_prefix(ctx)
        );
        fputs_e(
            "typedef struct pcc_rule_set_tag {\n"
            "    pcc_rule_t *buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_rule_set_t;\n"
            "\n"
            "typedef struct pcc_lr_head_tag pcc_lr_head_t;\n"
            "\n"
            "struct pcc_lr_head_tag {\n"
            "    pcc_rule_t rule;\n"
            "    pcc_rule_set_t invol;\n"
            "    pcc_rule_set_t eval;\n"
            "    pcc_lr_head_t *hold;\n"
            "};\n"
            "\n"
            "typedef struct pcc_lr_memo_tag {\n"
            "    pcc_rule_t rule;\n"
            "    pcc_lr_answer_t *answer;\n"
            "} pcc_lr_memo_t;\n"
            "\n"
            "typedef struct pcc_lr_memo_map_tag {\n"
            "    pcc_lr_memo_t *buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_lr_memo_map_t;\n"
            "\n"
            "typedef struct pcc_lr_table_entry_tag {\n"
            "    pcc_lr_head_t *head; /* just a reference */\n"
            "    pcc_lr_memo_map_t memos;\n"
            "    pcc_lr_answer_t *hold_a;\n"
            "    pcc_lr_head_t *hold_h;\n"
            "} pcc_lr_table_entry_t;\n"
            "\n"
            "typedef struct pcc_lr_table_tag {\n"
            "    pcc_lr_table_entry_t **buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_lr_table_t;\n"
            "\n"
            "struct pcc_lr_entry_tag {\n"
            "    pcc_rule_t rule;\n"
            "    pcc_thunk_chunk_t *seed; /* just a reference */\n"
            "    pcc_lr_head_t *head; /* just a reference */\n"
            "};\n"
            "\n"
            "typedef struct pcc_lr_stack_tag {\n"
            "    pcc_lr_entry_t **buf;\n"
            "    size_t max;\n"
            "    size_t len;\n"
            "} pcc_lr_stack_t;\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "struct %s_context_tag {\n"
            "    size_t pos; /* the position in the input of the first character currently buffered */\n"
            "    size_t cur; /* the current parsing position in the character buffer */\n"
            "    size_t level;\n"
            "    pcc_char_array_t buffer;\n"
            "    pcc_lr_table_t lrtable;\n"
            "    pcc_lr_stack_t lrstack;\n"
            "    pcc_auxil_t auxil;\n"
            "};\n"
            "\n",
            get_prefix(ctx)
        );
        fputs_e(
            "#ifndef PCC_ERROR\n"
            "#define PCC_ERROR(auxil) pcc_error()\n"
            "static void pcc_error(void) {\n"
            "    fprintf(stderr, \"Syntax error\\n\");\n"
            "    exit(1);\n"
            "}\n"
            "#endif /* !PCC_ERROR */\n"
            "\n"
            "#ifndef PCC_GETCHAR\n"
            "#define PCC_GETCHAR(auxil) getchar()\n"
            "#endif /* !PCC_GETCHAR */\n"
            "\n"
            "#ifndef PCC_MALLOC\n"
            "#define PCC_MALLOC(auxil, size) pcc_malloc_e(size)\n"
            "static void *pcc_malloc_e(size_t size) {\n"
            "    void *const p = malloc(size);\n"
            "    if (p == NULL) {\n"
            "        fprintf(stderr, \"Out of memory\\n\");\n"
            "        exit(1);\n"
            "    }\n"
            "    return p;\n"
            "}\n"
            "#endif /* !PCC_MALLOC */\n"
            "\n"
            "#ifndef PCC_REALLOC\n"
            "#define PCC_REALLOC(auxil, ptr, size) pcc_realloc_e(ptr, size)\n"
            "static void *pcc_realloc_e(void *ptr, size_t size) {\n"
            "    void *const p = realloc(ptr, size);\n"
            "    if (p == NULL) {\n"
            "        fprintf(stderr, \"Out of memory\\n\");\n"
            "        exit(1);\n"
            "    }\n"
            "    return p;\n"
            "}\n"
            "#endif /* !PCC_REALLOC */\n"
            "\n"
            "#ifndef PCC_FREE\n"
            "#define PCC_FREE(auxil, ptr) free(ptr)\n"
            "#endif /* !PCC_FREE */\n"
            "\n"
            "#ifndef PCC_DEBUG\n"
            "#define PCC_DEBUG(event, rule, level, pos, buffer, length) ((void)0)\n"
            "#endif /* !PCC_DEBUG */\n"
            "\n"
            /* not used
            "static char *pcc_strdup_e(pcc_auxil_t auxil, const char *str) {\n"
            "    const size_t m = strlen(str);\n"
            "    char *const s = (char *)PCC_MALLOC(auxil, m + 1);\n"
            "    memcpy(s, str, m);\n"
            "    s[m] = '\\0';\n"
            "    return s;\n"
            "}\n"
            "\n"
            */
            "static char *pcc_strndup_e(pcc_auxil_t auxil, const char *str, size_t len) {\n"
            "    const size_t m = strnlen(str, len);\n"
            "    char *const s = (char *)PCC_MALLOC(auxil, m + 1);\n"
            "    memcpy(s, str, m);\n"
            "    s[m] = '\\0';\n"
            "    return s;\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_char_array__init(pcc_auxil_t auxil, pcc_char_array_t *array, size_t max) {\n"
            "    array->len = 0;\n"
            "    array->max = max;\n"
            "    array->buf = (char *)PCC_MALLOC(auxil, array->max);\n"
            "}\n"
            "\n"
            "static void pcc_char_array__add(pcc_auxil_t auxil, pcc_char_array_t *array, char ch) {\n"
            "    if (array->max <= array->len) {\n"
            "        const size_t n = array->len + 1;\n"
            "        size_t m = array->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < n && m != 0) m <<= 1;\n"
            "        if (m == 0) m = n;\n"
            "        array->buf = (char *)PCC_REALLOC(auxil, array->buf, m);\n"
            "        array->max = m;\n"
            "    }\n"
            "    array->buf[array->len++] = ch;\n"
            "}\n"
            "\n"
            "static void pcc_char_array__term(pcc_auxil_t auxil, pcc_char_array_t *array) {\n"
            "    PCC_FREE(auxil, array->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_value_table__init(pcc_auxil_t auxil, pcc_value_table_t *table, size_t max) {\n"
            "    table->len = 0;\n"
            "    table->max = max;\n"
            "    table->buf = (pcc_value_t *)PCC_MALLOC(auxil, sizeof(pcc_value_t) * table->max);\n"
            "}\n"
            "\n"
            "static void pcc_value_table__resize(pcc_auxil_t auxil, pcc_value_table_t *table, size_t len) {\n"
            "    if (table->max < len) {\n"
            "        size_t m = table->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < len && m != 0) m <<= 1;\n"
            "        if (m == 0) m = len;\n"
            "        table->buf = (pcc_value_t *)PCC_REALLOC(auxil, table->buf, sizeof(pcc_value_t) * m);\n"
            "        table->max = m;\n"
            "    }\n"
            "    table->len = len;\n"
            "}\n"
            "\n"
            "static void pcc_value_table__term(pcc_auxil_t auxil, pcc_value_table_t *table) {\n"
            "    PCC_FREE(auxil, table->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_value_refer_table__init(pcc_auxil_t auxil, pcc_value_refer_table_t *table, size_t max) {\n"
            "    table->len = 0;\n"
            "    table->max = max;\n"
            "    table->buf = (pcc_value_t **)PCC_MALLOC(auxil, sizeof(pcc_value_t *) * table->max);\n"
            "}\n"
            "\n"
            "static void pcc_value_refer_table__resize(pcc_auxil_t auxil, pcc_value_refer_table_t *table, size_t len) {\n"
            "    size_t i;\n"
            "    if (table->max < len) {\n"
            "        size_t m = table->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < len && m != 0) m <<= 1;\n"
            "        if (m == 0) m = len;\n"
            "        table->buf = (pcc_value_t **)PCC_REALLOC(auxil, table->buf, sizeof(pcc_value_t *) * m);\n"
            "        table->max = m;\n"
            "    }\n"
            "    for (i = table->len; i < len; i++) table->buf[i] = NULL;\n"
            "    table->len = len;\n"
            "}\n"
            "\n"
            "static void pcc_value_refer_table__term(pcc_auxil_t auxil, pcc_value_refer_table_t *table) {\n"
            "    PCC_FREE(auxil, table->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_capture_table__init(pcc_auxil_t auxil, pcc_capture_table_t *table, size_t max) {\n"
            "    table->len = 0;\n"
            "    table->max = max;\n"
            "    table->buf = (pcc_capture_t *)PCC_MALLOC(auxil, sizeof(pcc_capture_t) * table->max);\n"
            "}\n"
            "\n"
            "static void pcc_capture_table__resize(pcc_auxil_t auxil, pcc_capture_table_t *table, size_t len) {\n"
            "    size_t i;\n"
            "    for (i = len; i < table->len; i++) PCC_FREE(auxil, table->buf[i].string);\n"
            "    if (table->max < len) {\n"
            "        size_t m = table->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < len && m != 0) m <<= 1;\n"
            "        if (m == 0) m = len;\n"
            "        table->buf = (pcc_capture_t *)PCC_REALLOC(auxil, table->buf, sizeof(pcc_capture_t) * m);\n"
            "        table->max = m;\n"
            "    }\n"
            "    for (i = table->len; i < len; i++) {\n"
            "        table->buf[i].range.start = 0;\n"
            "        table->buf[i].range.end = 0;\n"
            "        table->buf[i].string = NULL;\n"
            "    }\n"
            "    table->len = len;\n"
            "}\n"
            "\n"
            "static void pcc_capture_table__term(pcc_auxil_t auxil, pcc_capture_table_t *table) {\n"
            "    while (table->len > 0) {\n"
            "        table->len--;\n"
            "        PCC_FREE(auxil, table->buf[table->len].string);\n"
            "    }\n"
            "    PCC_FREE(auxil, table->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_capture_const_table__init(pcc_auxil_t auxil, pcc_capture_const_table_t *table, size_t max) {\n"
            "    table->len = 0;\n"
            "    table->max = max;\n"
            "    table->buf = (const pcc_capture_t **)PCC_MALLOC(auxil, sizeof(const pcc_capture_t *) * table->max);\n"
            "}\n"
            "\n"
            "static void pcc_capture_const_table__resize(pcc_auxil_t auxil, pcc_capture_const_table_t *table, size_t len) {\n"
            "    size_t i;\n"
            "    if (table->max < len) {\n"
            "        size_t m = table->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < len && m != 0) m <<= 1;\n"
            "        if (m == 0) m = len;\n"
            "        table->buf = (const pcc_capture_t **)PCC_REALLOC(auxil, (pcc_capture_t **)table->buf, sizeof(const pcc_capture_t *) * m);\n"
            "        table->max = m;\n"
            "    }\n"
            "    for (i = table->len; i < len; i++) table->buf[i] = NULL;\n"
            "    table->len = len;\n"
            "}\n"
            "\n"
            "static void pcc_capture_const_table__term(pcc_auxil_t auxil, pcc_capture_const_table_t *table) {\n"
            "    PCC_FREE(auxil, table->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static pcc_thunk_t *pcc_thunk__create_leaf(pcc_auxil_t auxil, pcc_action_t action, size_t valuec, size_t captc) {\n"
            "    pcc_thunk_t *const thunk = (pcc_thunk_t *)PCC_MALLOC(auxil, sizeof(pcc_thunk_t));\n"
            "    thunk->type = PCC_THUNK_LEAF;\n"
            "    pcc_value_refer_table__init(auxil, &thunk->data.leaf.values, valuec);\n"
            "    pcc_value_refer_table__resize(auxil, &thunk->data.leaf.values, valuec);\n"
            "    pcc_capture_const_table__init(auxil, &thunk->data.leaf.capts, captc);\n"
            "    pcc_capture_const_table__resize(auxil, &thunk->data.leaf.capts, captc);\n"
            "    thunk->data.leaf.capt0.range.start = 0;\n"
            "    thunk->data.leaf.capt0.range.end = 0;\n"
            "    thunk->data.leaf.capt0.string = NULL;\n"
            "    thunk->data.leaf.action = action;\n"
            "    return thunk;\n"
            "}\n"
            "\n"
            "static pcc_thunk_t *pcc_thunk__create_node(pcc_auxil_t auxil, const pcc_thunk_array_t *thunks, pcc_value_t *value) {\n"
            "    pcc_thunk_t *const thunk = (pcc_thunk_t *)PCC_MALLOC(auxil, sizeof(pcc_thunk_t));\n"
            "    thunk->type = PCC_THUNK_NODE;\n"
            "    thunk->data.node.thunks = thunks;\n"
            "    thunk->data.node.value = value;\n"
            "    return thunk;\n"
            "}\n"
            "\n"
            "static void pcc_thunk__destroy(pcc_auxil_t auxil, pcc_thunk_t *thunk) {\n"
            "    if (thunk == NULL) return;\n"
            "    switch (thunk->type) {\n"
            "    case PCC_THUNK_LEAF:\n"
            "        PCC_FREE(auxil, thunk->data.leaf.capt0.string);\n"
            "        pcc_capture_const_table__term(auxil, &thunk->data.leaf.capts);\n"
            "        pcc_value_refer_table__term(auxil, &thunk->data.leaf.values);\n"
            "        break;\n"
            "    case PCC_THUNK_NODE:\n"
            "        break;\n"
            "    default: /* unknown */\n"
            "        break;\n"
            "    }\n"
            "    PCC_FREE(auxil, thunk);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_thunk_array__init(pcc_auxil_t auxil, pcc_thunk_array_t *array, size_t max) {\n"
            "    array->len = 0;\n"
            "    array->max = max;\n"
            "    array->buf = (pcc_thunk_t **)PCC_MALLOC(auxil, sizeof(pcc_thunk_t *) * array->max);\n"
            "}\n"
            "\n"
            "static void pcc_thunk_array__add(pcc_auxil_t auxil, pcc_thunk_array_t *array, pcc_thunk_t *thunk) {\n"
            "    if (array->max <= array->len) {\n"
            "        const size_t n = array->len + 1;\n"
            "        size_t m = array->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < n && m != 0) m <<= 1;\n"
            "        if (m == 0) m = n;\n"
            "        array->buf = (pcc_thunk_t **)PCC_REALLOC(auxil, array->buf, sizeof(pcc_thunk_t *) * m);\n"
            "        array->max = m;\n"
            "    }\n"
            "    array->buf[array->len++] = thunk;\n"
            "}\n"
            "\n"
            "static void pcc_thunk_array__revert(pcc_auxil_t auxil, pcc_thunk_array_t *array, size_t len) {\n"
            "    while (array->len > len) {\n"
            "        array->len--;\n"
            "        pcc_thunk__destroy(auxil, array->buf[array->len]);\n"
            "    }\n"
            "}\n"
            "\n"
            "static void pcc_thunk_array__term(pcc_auxil_t auxil, pcc_thunk_array_t *array) {\n"
            "    while (array->len > 0) {\n"
            "        array->len--;\n"
            "        pcc_thunk__destroy(auxil, array->buf[array->len]);\n"
            "    }\n"
            "    PCC_FREE(auxil, array->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static pcc_thunk_chunk_t *pcc_thunk_chunk__create(pcc_auxil_t auxil) {\n"
            "    pcc_thunk_chunk_t *const chunk = (pcc_thunk_chunk_t *)PCC_MALLOC(auxil, sizeof(pcc_thunk_chunk_t));\n"
            "    pcc_value_table__init(auxil, &chunk->values, PCC_ARRAYSIZE);\n"
            "    pcc_capture_table__init(auxil, &chunk->capts, PCC_ARRAYSIZE);\n"
            "    pcc_thunk_array__init(auxil, &chunk->thunks, PCC_ARRAYSIZE);\n"
            "    chunk->pos = 0;\n"
            "    return chunk;\n"
            "}\n"
            "\n"
            "static void pcc_thunk_chunk__destroy(pcc_auxil_t auxil, pcc_thunk_chunk_t *chunk) {\n"
            "    if (chunk == NULL) return;\n"
            "    pcc_thunk_array__term(auxil, &chunk->thunks);\n"
            "    pcc_capture_table__term(auxil, &chunk->capts);\n"
            "    pcc_value_table__term(auxil, &chunk->values);\n"
            "    PCC_FREE(auxil, chunk);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_rule_set__init(pcc_auxil_t auxil, pcc_rule_set_t *set, size_t max) {\n"
            "    set->len = 0;\n"
            "    set->max = max;\n"
            "    set->buf = (pcc_rule_t *)PCC_MALLOC(auxil, sizeof(pcc_rule_t) * set->max);\n"
            "}\n"
            "\n"
            "static size_t pcc_rule_set__index(pcc_auxil_t auxil, const pcc_rule_set_t *set, pcc_rule_t rule) {\n"
            "    size_t i;\n"
            "    for (i = 0; i < set->len; i++) {\n"
            "        if (set->buf[i] == rule) return i;\n"
            "    }\n"
            "    return VOID_VALUE;\n"
            "}\n"
            "\n"
            "static pcc_bool_t pcc_rule_set__add(pcc_auxil_t auxil, pcc_rule_set_t *set, pcc_rule_t rule) {\n"
            "    const size_t i = pcc_rule_set__index(auxil, set, rule);\n"
            "    if (i != VOID_VALUE) return PCC_FALSE;\n"
            "    if (set->max <= set->len) {\n"
            "        const size_t n = set->len + 1;\n"
            "        size_t m = set->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < n && m != 0) m <<= 1;\n"
            "        if (m == 0) m = n;\n"
            "        set->buf = (pcc_rule_t *)PCC_REALLOC(auxil, set->buf, sizeof(pcc_rule_t) * m);\n"
            "        set->max = m;\n"
            "    }\n"
            "    set->buf[set->len++] = rule;\n"
            "    return PCC_TRUE;\n"
            "}\n"
            "\n"
            "static pcc_bool_t pcc_rule_set__remove(pcc_auxil_t auxil, pcc_rule_set_t *set, pcc_rule_t rule) {\n"
            "    const size_t i = pcc_rule_set__index(auxil, set, rule);\n"
            "    if (i == VOID_VALUE) return PCC_FALSE;\n"
            "    memmove(set->buf + i, set->buf + (i + 1), sizeof(pcc_rule_t) * (set->len - (i + 1)));\n"
            "    return PCC_TRUE;\n"
            "}\n"
            "\n"
            "static void pcc_rule_set__clear(pcc_auxil_t auxil, pcc_rule_set_t *set) {\n"
            "    set->len = 0;\n"
            "}\n"
            "\n"
            "static void pcc_rule_set__copy(pcc_auxil_t auxil, pcc_rule_set_t *set, const pcc_rule_set_t *src) {\n"
            "    size_t i;\n"
            "    pcc_rule_set__clear(auxil, set);\n"
            "    for (i = 0; i < src->len; i++) {\n"
            "        pcc_rule_set__add(auxil, set, src->buf[i]);\n"
            "    }\n"
            "}\n"
            "\n"
            "static void pcc_rule_set__term(pcc_auxil_t auxil, pcc_rule_set_t *set) {\n"
            "    PCC_FREE(auxil, set->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static pcc_lr_head_t *pcc_lr_head__create(pcc_auxil_t auxil, pcc_rule_t rule) {\n"
            "    pcc_lr_head_t *const head = (pcc_lr_head_t *)PCC_MALLOC(auxil, sizeof(pcc_lr_head_t));\n"
            "    head->rule = rule;\n"
            "    pcc_rule_set__init(auxil, &head->invol, PCC_ARRAYSIZE);\n"
            "    pcc_rule_set__init(auxil, &head->eval, PCC_ARRAYSIZE);\n"
            "    head->hold = NULL;\n"
            "    return head;\n"
            "}\n"
            "\n"
            "static void pcc_lr_head__destroy(pcc_auxil_t auxil, pcc_lr_head_t *head) {\n"
            "    if (head == NULL) return;\n"
            "    pcc_lr_head__destroy(auxil, head->hold);\n"
            "    pcc_rule_set__term(auxil, &head->eval);\n"
            "    pcc_rule_set__term(auxil, &head->invol);\n"
            "    PCC_FREE(auxil, head);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_lr_entry__destroy(pcc_auxil_t auxil, pcc_lr_entry_t *lr);\n"
            "\n"
            "static pcc_lr_answer_t *pcc_lr_answer__create(pcc_auxil_t auxil, pcc_lr_answer_type_t type, size_t pos) {\n"
            "    pcc_lr_answer_t *answer = (pcc_lr_answer_t *)PCC_MALLOC(auxil, sizeof(pcc_lr_answer_t));\n"
            "    answer->type = type;\n"
            "    answer->pos = pos;\n"
            "    answer->hold = NULL;\n"
            "    switch (answer->type) {\n"
            "    case PCC_LR_ANSWER_LR:\n"
            "        answer->data.lr = NULL;\n"
            "        break;\n"
            "    case PCC_LR_ANSWER_CHUNK:\n"
            "        answer->data.chunk = NULL;\n"
            "        break;\n"
            "    default: /* unknown */\n"
            "        PCC_FREE(auxil, answer);\n"
            "        answer = NULL;\n"
            "    }\n"
            "    return answer;\n"
            "}\n"
            "\n"
            "static void pcc_lr_answer__set_chunk(pcc_auxil_t auxil, pcc_lr_answer_t *answer, pcc_thunk_chunk_t *chunk) {\n"
            "    pcc_lr_answer_t *const a = pcc_lr_answer__create(auxil, answer->type, answer->pos);\n"
            "    switch (answer->type) {\n"
            "    case PCC_LR_ANSWER_LR:\n"
            "        a->data.lr = answer->data.lr;\n"
            "        break;\n"
            "    case PCC_LR_ANSWER_CHUNK:\n"
            "        a->data.chunk = answer->data.chunk;\n"
            "        break;\n"
            "    default: /* unknown */\n"
            "        break;\n"
            "    }\n"
            "    a->hold = answer->hold;\n"
            "    answer->hold = a;\n"
            "    answer->type = PCC_LR_ANSWER_CHUNK;\n"
            "    answer->data.chunk = chunk;\n"
            "}\n"
            "\n"
            "static void pcc_lr_answer__destroy(pcc_auxil_t auxil, pcc_lr_answer_t *answer) {\n"
            "    if (answer == NULL) return;\n"
            "    pcc_lr_answer__destroy(auxil, answer->hold);\n"
            "    switch (answer->type) {\n"
            "    case PCC_LR_ANSWER_LR:\n"
            "        pcc_lr_entry__destroy(auxil, answer->data.lr);\n"
            "        break;\n"
            "    case PCC_LR_ANSWER_CHUNK:\n"
            "        pcc_thunk_chunk__destroy(auxil, answer->data.chunk);\n"
            "        break;\n"
            "    default: /* unknown */\n"
            "        break;\n"
            "    }\n"
            "    PCC_FREE(auxil, answer);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_lr_memo_map__init(pcc_auxil_t auxil, pcc_lr_memo_map_t *map, size_t max) {\n"
            "    map->len = 0;\n"
            "    map->max = max;\n"
            "    map->buf = (pcc_lr_memo_t *)PCC_MALLOC(auxil, sizeof(pcc_lr_memo_t) * map->max);\n"
            "}\n"
            "\n"
            "static size_t pcc_lr_memo_map__index(pcc_auxil_t auxil, pcc_lr_memo_map_t *map, pcc_rule_t rule) {\n"
            "    size_t i;\n"
            "    for (i = 0; i < map->len; i++) {\n"
            "        if (map->buf[i].rule == rule) return i;\n"
            "    }\n"
            "    return VOID_VALUE;\n"
            "}\n"
            "\n"
            "static void pcc_lr_memo_map__put(pcc_auxil_t auxil, pcc_lr_memo_map_t *map, pcc_rule_t rule, pcc_lr_answer_t *answer) {\n"
            "    const size_t i = pcc_lr_memo_map__index(auxil, map, rule);\n"
            "    if (i != VOID_VALUE) {\n"
            "        pcc_lr_answer__destroy(auxil, map->buf[i].answer);\n"
            "        map->buf[i].answer = answer;\n"
            "    }\n"
            "    else {\n"
            "        if (map->max <= map->len) {\n"
            "            const size_t n = map->len + 1;\n"
            "            size_t m = map->max;\n"
            "            if (m == 0) m = 1;\n"
            "            while (m < n && m != 0) m <<= 1;\n"
            "            if (m == 0) m = n;\n"
            "            map->buf = (pcc_lr_memo_t *)PCC_REALLOC(auxil, map->buf, sizeof(pcc_lr_memo_t) * m);\n"
            "            map->max = m;\n"
            "        }\n"
            "        map->buf[map->len].rule = rule;\n"
            "        map->buf[map->len].answer = answer;\n"
            "        map->len++;\n"
            "    }\n"
            "}\n"
            "\n"
            "static pcc_lr_answer_t *pcc_lr_memo_map__get(pcc_auxil_t auxil, pcc_lr_memo_map_t *map, pcc_rule_t rule) {\n"
            "    const size_t i = pcc_lr_memo_map__index(auxil, map, rule);\n"
            "    return (i != VOID_VALUE) ? map->buf[i].answer : NULL;\n"
            "}\n"
            "\n"
            "static void pcc_lr_memo_map__term(pcc_auxil_t auxil, pcc_lr_memo_map_t *map) {\n"
            "    while (map->len > 0) {\n"
            "        map->len--;\n"
            "        pcc_lr_answer__destroy(auxil, map->buf[map->len].answer);\n"
            "    }\n"
            "    PCC_FREE(auxil, map->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static pcc_lr_table_entry_t *pcc_lr_table_entry__create(pcc_auxil_t auxil) {\n"
            "    pcc_lr_table_entry_t *const entry = (pcc_lr_table_entry_t *)PCC_MALLOC(auxil, sizeof(pcc_lr_table_entry_t));\n"
            "    entry->head = NULL;\n"
            "    pcc_lr_memo_map__init(auxil, &entry->memos, PCC_ARRAYSIZE);\n"
            "    entry->hold_a = NULL;\n"
            "    entry->hold_h = NULL;\n"
            "    return entry;\n"
            "}\n"
            "\n"
            "static void pcc_lr_table_entry__destroy(pcc_auxil_t auxil, pcc_lr_table_entry_t *entry) {\n"
            "    if (entry == NULL) return;\n"
            "    pcc_lr_head__destroy(auxil, entry->hold_h);\n"
            "    pcc_lr_answer__destroy(auxil, entry->hold_a);\n"
            "    pcc_lr_memo_map__term(auxil, &entry->memos);\n"
            "    PCC_FREE(auxil, entry);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_lr_table__init(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t max) {\n"
            "    table->len = 0;\n"
            "    table->max = max;\n"
            "    table->buf = (pcc_lr_table_entry_t **)PCC_MALLOC(auxil, sizeof(pcc_lr_table_entry_t *) * table->max);\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__resize(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t len) {\n"
            "    size_t i;\n"
            "    for (i = len; i < table->len; i++) pcc_lr_table_entry__destroy(auxil, table->buf[i]);\n"
            "    if (table->max < len) {\n"
            "        size_t m = table->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < len && m != 0) m <<= 1;\n"
            "        if (m == 0) m = len;\n"
            "        table->buf = (pcc_lr_table_entry_t **)PCC_REALLOC(auxil, table->buf, sizeof(pcc_lr_table_entry_t *) * m);\n"
            "        table->max = m;\n"
            "    }\n"
            "    for (i = table->len; i < len; i++) table->buf[i] = NULL;\n"
            "    table->len = len;\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__set_head(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index, pcc_lr_head_t *head) {\n"
            "    if (index >= table->len) pcc_lr_table__resize(auxil, table, index + 1);\n"
            "    if (table->buf[index] == NULL) table->buf[index] = pcc_lr_table_entry__create(auxil);\n"
            "    table->buf[index]->head = head;\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__hold_head(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index, pcc_lr_head_t *head) {\n"
            "    if (index >= table->len) pcc_lr_table__resize(auxil, table, index + 1);\n"
            "    if (table->buf[index] == NULL) table->buf[index] = pcc_lr_table_entry__create(auxil);\n"
            "    head->hold = table->buf[index]->hold_h;\n"
            "    table->buf[index]->hold_h = head;\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__set_answer(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index, pcc_rule_t rule, pcc_lr_answer_t *answer) {\n"
            "    if (index >= table->len) pcc_lr_table__resize(auxil, table, index + 1);\n"
            "    if (table->buf[index] == NULL) table->buf[index] = pcc_lr_table_entry__create(auxil);\n"
            "    pcc_lr_memo_map__put(auxil, &table->buf[index]->memos, rule, answer);\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__hold_answer(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index, pcc_lr_answer_t *answer) {\n"
            "    if (index >= table->len) pcc_lr_table__resize(auxil, table, index + 1);\n"
            "    if (table->buf[index] == NULL) table->buf[index] = pcc_lr_table_entry__create(auxil);\n"
            "    answer->hold = table->buf[index]->hold_a;\n"
            "    table->buf[index]->hold_a = answer;\n"
            "}\n"
            "\n"
            "static pcc_lr_head_t *pcc_lr_table__get_head(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index) {\n"
            "    if (index >= table->len || table->buf[index] == NULL) return NULL;\n"
            "    return table->buf[index]->head;\n"
            "}\n"
            "\n"
            "static pcc_lr_answer_t *pcc_lr_table__get_answer(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t index, pcc_rule_t rule) {\n"
            "    if (index >= table->len || table->buf[index] == NULL) return NULL;\n"
            "    return pcc_lr_memo_map__get(auxil, &table->buf[index]->memos, rule);\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__shift(pcc_auxil_t auxil, pcc_lr_table_t *table, size_t count) {\n"
            "    size_t i;\n"
            "    if (count > table->len) count = table->len;\n"
            "    for (i = 0; i < count; i++) pcc_lr_table_entry__destroy(auxil, table->buf[i]);\n"
            "    memmove(table->buf, table->buf + count, sizeof(pcc_lr_table_entry_t *) * (table->len - count));\n"
            "    table->len -= count;\n"
            "}\n"
            "\n"
            "static void pcc_lr_table__term(pcc_auxil_t auxil, pcc_lr_table_t *table) {\n"
            "    while (table->len > 0) {\n"
            "        table->len--;\n"
            "        pcc_lr_table_entry__destroy(auxil, table->buf[table->len]);\n"
            "    }\n"
            "    PCC_FREE(auxil, table->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static pcc_lr_entry_t *pcc_lr_entry__create(pcc_auxil_t auxil, pcc_rule_t rule) {\n"
            "    pcc_lr_entry_t *const lr = (pcc_lr_entry_t *)PCC_MALLOC(auxil, sizeof(pcc_lr_entry_t));\n"
            "    lr->rule = rule;\n"
            "    lr->seed = NULL;\n"
            "    lr->head = NULL;\n"
            "    return lr;\n"
            "}\n"
            "\n"
            "static void pcc_lr_entry__destroy(pcc_auxil_t auxil, pcc_lr_entry_t *lr) {\n"
            "    PCC_FREE(auxil, lr);\n"
            "}\n"
            "\n",
            sstream
        );
        fputs_e(
            "static void pcc_lr_stack__init(pcc_auxil_t auxil, pcc_lr_stack_t *stack, size_t max) {\n"
            "    stack->len = 0;\n"
            "    stack->max = max;\n"
            "    stack->buf = (pcc_lr_entry_t **)PCC_MALLOC(auxil, sizeof(pcc_lr_entry_t *) * stack->max);\n"
            "}\n"
            "\n"
            "static void pcc_lr_stack__push(pcc_auxil_t auxil, pcc_lr_stack_t *stack, pcc_lr_entry_t *lr) {\n"
            "    if (stack->max <= stack->len) {\n"
            "        const size_t n = stack->len + 1;\n"
            "        size_t m = stack->max;\n"
            "        if (m == 0) m = 1;\n"
            "        while (m < n && m != 0) m <<= 1;\n"
            "        if (m == 0) m = n;\n"
            "        stack->buf = (pcc_lr_entry_t **)PCC_REALLOC(auxil, stack->buf, sizeof(pcc_lr_entry_t *) * m);\n"
            "        stack->max = m;\n"
            "    }\n"
            "    stack->buf[stack->len++] = lr;\n"
            "}\n"
            "\n"
            "static pcc_lr_entry_t *pcc_lr_stack__pop(pcc_auxil_t auxil, pcc_lr_stack_t *stack) {\n"
            "    return stack->buf[--stack->len];\n"
            "}\n"
            "\n"
            "static void pcc_lr_stack__term(pcc_auxil_t auxil, pcc_lr_stack_t *stack) {\n"
            "    PCC_FREE(auxil, stack->buf);\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static %s_context_t *pcc_context__create(pcc_auxil_t auxil) {\n"
            "    %s_context_t *const ctx = (%s_context_t *)PCC_MALLOC(auxil, sizeof(%s_context_t));\n",
            get_prefix(ctx), get_prefix(ctx), get_prefix(ctx), get_prefix(ctx)
        );
        fputs_e(
            "    ctx->pos = 0;\n"
            "    ctx->cur = 0;\n"
            "    ctx->level = 0;\n"
            "    pcc_char_array__init(auxil, &ctx->buffer, PCC_BUFFERSIZE);\n"
            "    pcc_lr_table__init(auxil, &ctx->lrtable, PCC_BUFFERSIZE);\n"
            "    pcc_lr_stack__init(auxil, &ctx->lrstack, PCC_ARRAYSIZE);\n"
            "    ctx->auxil = auxil;\n"
            "    return ctx;\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static void pcc_context__destroy(%s_context_t *ctx) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    if (ctx == NULL) return;\n"
            "    pcc_lr_stack__term(ctx->auxil, &ctx->lrstack);\n"
            "    pcc_lr_table__term(ctx->auxil, &ctx->lrtable);\n"
            "    pcc_char_array__term(ctx->auxil, &ctx->buffer);\n"
            "    PCC_FREE(ctx->auxil, ctx);\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static size_t pcc_refill_buffer(%s_context_t *ctx, size_t num) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    if (ctx->buffer.len >= ctx->cur + num) return ctx->buffer.len - ctx->cur;\n"
            "    while (ctx->buffer.len < ctx->cur + num) {\n"
            "        const int c = PCC_GETCHAR(ctx->auxil);\n"
            "        if (c == EOF) break;\n"
            "        pcc_char_array__add(ctx->auxil, &ctx->buffer, (char)c);\n"
            "    }\n"
            "    return ctx->buffer.len - ctx->cur;\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static void pcc_commit_buffer(%s_context_t *ctx) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    memmove(ctx->buffer.buf, ctx->buffer.buf + ctx->cur, ctx->buffer.len - ctx->cur);\n"
            "    ctx->buffer.len -= ctx->cur;\n"
            "    ctx->pos += ctx->cur;\n"
            "    pcc_lr_table__shift(ctx->auxil, &ctx->lrtable, ctx->cur);\n"
            "    ctx->cur = 0;\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static const char *pcc_get_capture_string(%s_context_t *ctx, const pcc_capture_t *capt) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    if (capt->string == NULL)\n"
            "        ((pcc_capture_t *)capt)->string =\n"
            "            pcc_strndup_e(ctx->auxil, ctx->buffer.buf + capt->range.start, capt->range.end - capt->range.start);\n"
            "    return capt->string;\n"
            "}\n"
            "\n",
            sstream
        );
        if (ctx->flags & CODE_FLAG__UTF8_CHARCLASS_USED) {
            fprintf_e(
                sstream,
                "static size_t pcc_get_char_as_utf32(%s_context_t *ctx, int *out) { /* with checking UTF-8 validity */\n",
                get_prefix(ctx)
            );
            fputs_e(
                "    int c, u;\n"
                "    size_t n;\n"
                "    if (pcc_refill_buffer(ctx, 1) < 1) return 0;\n"
                "    c = (int)(unsigned char)ctx->buffer.buf[ctx->cur];\n"
                "    n = (c < 0x80) ? 1 :\n"
                "        ((c & 0xe0) == 0xc0) ? 2 :\n"
                "        ((c & 0xf0) == 0xe0) ? 3 :\n"
                "        ((c & 0xf8) == 0xf0) ? 4 : 0;\n"
                "    if (n < 1) return 0;\n"
                "    if (pcc_refill_buffer(ctx, n) < n) return 0;\n"
                "    switch (n) {\n"
                "    case 1:\n"
                "        u = c;\n"
                "        break;\n"
                "    case 2:\n"
                "        u = c & 0x1f;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 1];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        if (u < 0x80) return 0;\n"
                "        break;\n"
                "    case 3:\n"
                "        u = c & 0x0f;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 1];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 2];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        if (u < 0x800) return 0;\n"
                "        break;\n"
                "    case 4:\n"
                "        u = c & 0x07;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 1];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 2];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        c = (int)(unsigned char)ctx->buffer.buf[ctx->cur + 3];\n"
                "        if ((c & 0xc0) != 0x80) return 0;\n"
                "        u <<= 6; u |= c & 0x3f;\n"
                "        if (u < 0x10000 || u > 0x10ffff) return 0;\n"
                "        break;\n"
                "    default:\n"
                "        return 0;\n"
                "    }\n"
                "    if (out) *out = u;\n"
                "    return n;\n"
                "}\n"
                "\n",
                sstream
            );
        }
        fprintf_e(
            sstream,
            "static pcc_bool_t pcc_apply_rule(%s_context_t *ctx, pcc_rule_t rule, pcc_thunk_array_t *thunks, pcc_value_t *value) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    static pcc_value_t null;\n"
            "    pcc_thunk_chunk_t *c = NULL;\n"
            "    const size_t p = ctx->pos + ctx->cur;\n"
            "    pcc_bool_t b = PCC_TRUE;\n"
            "    pcc_lr_answer_t *a = pcc_lr_table__get_answer(ctx->auxil, &ctx->lrtable, p, rule);\n"
            "    pcc_lr_head_t *h = pcc_lr_table__get_head(ctx->auxil, &ctx->lrtable, p);\n"
            "    if (h != NULL) {\n"
            "        if (a == NULL && rule != h->rule && pcc_rule_set__index(ctx->auxil, &h->invol, rule) == VOID_VALUE) {\n"
            "            b = PCC_FALSE;\n"
            "            c = NULL;\n"
            "        }\n"
            "        else if (pcc_rule_set__remove(ctx->auxil, &h->eval, rule)) {\n"
            "            b = PCC_FALSE;\n"
            "            c = rule(ctx);\n"
            "            a = pcc_lr_answer__create(ctx->auxil, PCC_LR_ANSWER_CHUNK, ctx->pos + ctx->cur);\n"
            "            a->data.chunk = c;\n"
            "            pcc_lr_table__hold_answer(ctx->auxil, &ctx->lrtable, p, a);\n"
            "        }\n"
            "    }\n"
            "    if (b) {\n"
            "        if (a != NULL) {\n"
            "            ctx->cur = a->pos - ctx->pos;\n"
            "            switch (a->type) {\n"
            "            case PCC_LR_ANSWER_LR:\n"
            "                if (a->data.lr->head == NULL) {\n"
            "                    a->data.lr->head = pcc_lr_head__create(ctx->auxil, rule);\n"
            "                    pcc_lr_table__hold_head(ctx->auxil, &ctx->lrtable, p, a->data.lr->head);\n"
            "                }\n"
            "                {\n"
            "                    size_t i = ctx->lrstack.len;\n"
            "                    while (i > 0) {\n"
            "                        i--;\n"
            "                        if (ctx->lrstack.buf[i]->head == a->data.lr->head) break;\n"
            "                        ctx->lrstack.buf[i]->head = a->data.lr->head;\n"
            "                        pcc_rule_set__add(ctx->auxil, &a->data.lr->head->invol, ctx->lrstack.buf[i]->rule);\n"
            "                    }\n"
            "                }\n"
            "                c = a->data.lr->seed;\n"
            "                break;\n"
            "            case PCC_LR_ANSWER_CHUNK:\n"
            "                c = a->data.chunk;\n"
            "                break;\n"
            "            default: /* unknown */\n"
            "                break;\n"
            "            }\n"
            "        }\n"
            "        else {\n"
            "            pcc_lr_entry_t *const e = pcc_lr_entry__create(ctx->auxil, rule);\n"
            "            pcc_lr_stack__push(ctx->auxil, &ctx->lrstack, e);\n"
            "            a = pcc_lr_answer__create(ctx->auxil, PCC_LR_ANSWER_LR, p);\n"
            "            a->data.lr = e;\n"
            "            pcc_lr_table__set_answer(ctx->auxil, &ctx->lrtable, p, rule, a);\n"
            "            c = rule(ctx);\n"
            "            pcc_lr_stack__pop(ctx->auxil, &ctx->lrstack);\n"
            "            a->pos = ctx->pos + ctx->cur;\n"
            "            if (e->head == NULL) {\n"
            "                pcc_lr_answer__set_chunk(ctx->auxil, a, c);\n"
            "            }\n"
            "            else {\n"
            "                e->seed = c;\n"
            "                h = a->data.lr->head;\n"
            "                if (h->rule != rule) {\n"
            "                    c = a->data.lr->seed;\n"
            "                    a = pcc_lr_answer__create(ctx->auxil, PCC_LR_ANSWER_CHUNK, ctx->pos + ctx->cur);\n"
            "                    a->data.chunk = c;\n"
            "                    pcc_lr_table__hold_answer(ctx->auxil, &ctx->lrtable, p, a);\n"
            "                }\n"
            "                else {\n"
            "                    pcc_lr_answer__set_chunk(ctx->auxil, a, a->data.lr->seed);\n"
            "                    if (a->data.chunk == NULL) {\n"
            "                        c = NULL;\n"
            "                    }\n"
            "                    else {\n"
            "                        pcc_lr_table__set_head(ctx->auxil, &ctx->lrtable, p, h);\n"
            "                        for (;;) {\n"
            "                            ctx->cur = p - ctx->pos;\n"
            "                            pcc_rule_set__copy(ctx->auxil, &h->eval, &h->invol);\n"
            "                            c = rule(ctx);\n"
            "                            if (c == NULL || ctx->pos + ctx->cur <= a->pos) break;\n"
            "                            pcc_lr_answer__set_chunk(ctx->auxil, a, c);\n"
            "                            a->pos = ctx->pos + ctx->cur;\n"
            "                        }\n"
            "                        pcc_thunk_chunk__destroy(ctx->auxil, c);\n"
            "                        pcc_lr_table__set_head(ctx->auxil, &ctx->lrtable, p, NULL);\n"
            "                        ctx->cur = a->pos - ctx->pos;\n"
            "                        c = a->data.chunk;\n"
            "                    }\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "    if (c == NULL) return PCC_FALSE;\n"
            "    if (value == NULL) value = &null;\n"
            "    memset(value, 0, sizeof(pcc_value_t)); /* in case */\n"
            "    pcc_thunk_array__add(ctx->auxil, thunks, pcc_thunk__create_node(ctx->auxil, &c->thunks, value));\n"
            "    return PCC_TRUE;\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "static void pcc_do_action(%s_context_t *ctx, const pcc_thunk_array_t *thunks, pcc_value_t *value) {\n",
            get_prefix(ctx)
        );
        fputs_e(
            "    size_t i;\n"
            "    for (i = 0; i < thunks->len; i++) {\n"
            "        pcc_thunk_t *const thunk = thunks->buf[i];\n"
            "        switch (thunk->type) {\n"
            "        case PCC_THUNK_LEAF:\n"
            "            thunk->data.leaf.action(ctx, thunk, value);\n"
            "            break;\n"
            "        case PCC_THUNK_NODE:\n"
            "            pcc_do_action(ctx, thunk->data.node.thunks, thunk->data.node.value);\n"
            "            break;\n"
            "        default: /* unknown */\n"
            "            break;\n"
            "        }\n"
            "    }\n"
            "}\n"
            "\n",
            sstream
        );
        {
            size_t i, j, k;
            for (i = 0; i < ctx->rules.len; i++) {
                const node_rule_t *const r = &ctx->rules.buf[i]->data.rule;
                for (j = 0; j < r->codes.len; j++) {
                    const char *s;
                    size_t d;
                    const node_const_array_t *v, *c;
                    switch (r->codes.buf[j]->type) {
                    case NODE_ACTION:
                        s = r->codes.buf[j]->data.action.value;
                        d = r->codes.buf[j]->data.action.index;
                        v = &r->codes.buf[j]->data.action.vars;
                        c = &r->codes.buf[j]->data.action.capts;
                        break;
                    case NODE_ERROR:
                        s = r->codes.buf[j]->data.error.value;
                        d = r->codes.buf[j]->data.error.index;
                        v = &r->codes.buf[j]->data.error.vars;
                        c = &r->codes.buf[j]->data.error.capts;
                        break;
                    default:
                        print_error("Internal error [%d]\n", __LINE__);
                        exit(-1);
                    }
                    fprintf_e(
                        sstream,
                        "static void pcc_action_%s_%d(%s_context_t *__pcc_ctx, pcc_thunk_t *__pcc_in, pcc_value_t *__pcc_out) {\n",
                        r->name, d, get_prefix(ctx)
                    );
                    fputs_e(
                        "#define auxil (__pcc_ctx->auxil)\n"
                        "#define __ (*__pcc_out)\n",
                        sstream
                    );
                    k = 0;
                    while (k < v->len) {
                        assert(v->buf[k]->type == NODE_REFERENCE);
                        fprintf_e(
                            sstream,
                            "#define %s (*__pcc_in->data.leaf.values.buf[" FMT_LU "])\n",
                            v->buf[k]->data.reference.var, (ulong_t)v->buf[k]->data.reference.index
                        );
                        k++;
                    }
                    fputs_e(
                        "#define _0 pcc_get_capture_string(__pcc_ctx, &__pcc_in->data.leaf.capt0)\n"
                        "#define _0s ((const size_t)__pcc_in->data.leaf.capt0.range.start)\n"
                        "#define _0e ((const size_t)__pcc_in->data.leaf.capt0.range.end)\n",
                        sstream
                    );
                    k = 0;
                    while (k < c->len) {
                        assert(c->buf[k]->type == NODE_CAPTURE);
                        fprintf_e(
                            sstream,
                            "#define _" FMT_LU " pcc_get_capture_string(__pcc_ctx, __pcc_in->data.leaf.capts.buf[" FMT_LU "])\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1), (ulong_t)c->buf[k]->data.capture.index
                        );
                        fprintf_e(
                            sstream,
                            "#define _" FMT_LU "s ((const size_t)(__pcc_ctx->pos + __pcc_in->data.leaf.capts.buf[" FMT_LU "]->range.start))\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1), (ulong_t)c->buf[k]->data.capture.index
                        );
                        fprintf_e(
                            sstream,
                            "#define _" FMT_LU "e ((const size_t)(__pcc_ctx->pos + __pcc_in->data.leaf.capts.buf[" FMT_LU "]->range.end))\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1), (ulong_t)c->buf[k]->data.capture.index
                        );
                        k++;
                    }
                    write_code_block(sstream, s, strlen(s), 4);
                    k = c->len;
                    while (k > 0) {
                        k--;
                        assert(c->buf[k]->type == NODE_CAPTURE);
                        fprintf_e(
                            sstream,
                            "#undef _" FMT_LU "e\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1)
                        );
                        fprintf_e(
                            sstream,
                            "#undef _" FMT_LU "s\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1)
                        );
                        fprintf_e(
                            sstream,
                            "#undef _" FMT_LU "\n",
                            (ulong_t)(c->buf[k]->data.capture.index + 1)
                        );
                    }
                    fputs_e(
                        "#undef _0e\n"
                        "#undef _0s\n"
                        "#undef _0\n",
                        sstream
                    );
                    k = v->len;
                    while (k > 0) {
                        k--;
                        assert(v->buf[k]->type == NODE_REFERENCE);
                        fprintf_e(
                            sstream,
                            "#undef %s\n",
                            v->buf[k]->data.reference.var
                        );
                    }
                    fputs_e(
                        "#undef __\n"
                        "#undef auxil\n",
                        sstream
                    );
                    fputs_e(
                        "}\n"
                        "\n",
                        sstream
                    );
                }
            }
        }
        {
            size_t i;
            for (i = 0; i < ctx->rules.len; i++) {
                fprintf_e(
                    sstream,
                    "static pcc_thunk_chunk_t *pcc_evaluate_rule_%s(%s_context_t *ctx);\n",
                    ctx->rules.buf[i]->data.rule.name, get_prefix(ctx)
                );
            }
            fputs_e(
                "\n",
                sstream
            );
            for (i = 0; i < ctx->rules.len; i++) {
                code_reach_t r;
                generate_t g;
                g.stream = sstream;
                g.rule = ctx->rules.buf[i];
                g.label = 0;
                g.ascii = ctx->ascii;
                fprintf_e(
                    sstream,
                    "static pcc_thunk_chunk_t *pcc_evaluate_rule_%s(%s_context_t *ctx) {\n",
                    ctx->rules.buf[i]->data.rule.name, get_prefix(ctx)
                );
                fprintf_e(
                    sstream,
                    "    pcc_thunk_chunk_t *const chunk = pcc_thunk_chunk__create(ctx->auxil);\n"
                    "    chunk->pos = ctx->cur;\n"
                    "    PCC_DEBUG(PCC_DBG_EVALUATE, \"%s\", ctx->level, chunk->pos, (ctx->buffer.buf + chunk->pos), (ctx->buffer.len - chunk->pos));\n"
                    "    ctx->level++;\n",
                    ctx->rules.buf[i]->data.rule.name
                );
                fprintf_e(
                    sstream,
                    "    pcc_value_table__resize(ctx->auxil, &chunk->values, " FMT_LU ");\n",
                    (ulong_t)ctx->rules.buf[i]->data.rule.vars.len
                );
                fprintf_e(
                    sstream,
                    "    pcc_capture_table__resize(ctx->auxil, &chunk->capts, " FMT_LU ");\n",
                    (ulong_t)ctx->rules.buf[i]->data.rule.capts.len
                );
                r = generate_code(&g, ctx->rules.buf[i]->data.rule.expr, 0, 4, FALSE);
                fprintf_e(
                    sstream,
                    "    ctx->level--;\n"
                    "    PCC_DEBUG(PCC_DBG_MATCH, \"%s\", ctx->level, chunk->pos, (ctx->buffer.buf + chunk->pos), (ctx->cur - chunk->pos));\n"
                    "    return chunk;\n",
                    ctx->rules.buf[i]->data.rule.name
                );
                if (r != CODE_REACH__ALWAYS_SUCCEED) {
                    fprintf_e(
                        sstream,
                        "L0000:;\n"
                        "    ctx->level--;\n"
                        "    PCC_DEBUG(PCC_DBG_NOMATCH, \"%s\", ctx->level, chunk->pos, (ctx->buffer.buf + chunk->pos), (ctx->cur - chunk->pos));\n"
                        "    pcc_thunk_chunk__destroy(ctx->auxil, chunk);\n"
                        "    return NULL;\n",
                        ctx->rules.buf[i]->data.rule.name
                    );
                }
                fputs_e(
                    "}\n"
                    "\n",
                    sstream
                );
            }
        }
        fprintf_e(
            sstream,
            "%s_context_t *%s_create(%s%sauxil) {\n",
            get_prefix(ctx), get_prefix(ctx),
            at, ap ? "" : " "
        );
        fputs_e(
            "    return pcc_context__create(auxil);\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "int %s_parse(%s_context_t *ctx, %s%s*ret) {\n",
            get_prefix(ctx), get_prefix(ctx),
            vt, vp ? "" : " "
        );
        fputs_e(
            "    pcc_thunk_array_t thunks;\n"
            "    pcc_thunk_array__init(ctx->auxil, &thunks, PCC_ARRAYSIZE);\n",
            sstream
        );
        if (ctx->rules.len > 0) {
            fprintf_e(
                sstream,
                "    if (pcc_apply_rule(ctx, pcc_evaluate_rule_%s, &thunks, ret))\n",
                ctx->rules.buf[0]->data.rule.name
            );
            fputs_e(
                "        pcc_do_action(ctx, &thunks, ret);\n"
                "    else\n"
                "        PCC_ERROR(ctx->auxil);\n"
                "    pcc_commit_buffer(ctx);\n",
                sstream
            );
        }
        fputs_e(
            "    pcc_thunk_array__term(ctx->auxil, &thunks);\n"
            "    return pcc_refill_buffer(ctx, 1) >= 1;\n"
            "}\n"
            "\n",
            sstream
        );
        fprintf_e(
            sstream,
            "void %s_destroy(%s_context_t *ctx) {\n",
            get_prefix(ctx), get_prefix(ctx)
        );
        fputs_e(
            "    pcc_context__destroy(ctx);\n"
            "}\n",
            sstream
        );
    }
    {
        fputs_e(
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n"
            "\n",
            hstream
        );
        fprintf_e(
            hstream,
            "typedef struct %s_context_tag %s_context_t;\n"
            "\n",
            get_prefix(ctx), get_prefix(ctx)
        );
        fprintf_e(
            hstream,
            "%s_context_t *%s_create(%s%sauxil);\n",
            get_prefix(ctx), get_prefix(ctx),
            at, ap ? "" : " "
        );
        fprintf_e(
            hstream,
            "int %s_parse(%s_context_t *ctx, %s%s*ret);\n",
            get_prefix(ctx), get_prefix(ctx),
            vt, vp ? "" : " "
        );
        fprintf_e(
            hstream,
            "void %s_destroy(%s_context_t *ctx);\n",
            get_prefix(ctx), get_prefix(ctx)
        );
        fputs_e(
            "\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n",
            hstream
        );
        fprintf_e(
            hstream,
            "\n"
            "#endif /* !PCC_INCLUDED_%s */\n",
            ctx->hid
        );
    }
    {
        match_eol(ctx);
        if (!match_eof(ctx)) fputc_e('\n', sstream);
        commit_buffer(ctx);
        while (refill_buffer(ctx, ctx->buffer.max) > 0) {
            const size_t n = (ctx->buffer.len > 0 && ctx->buffer.buf[ctx->buffer.len - 1] == '\r') ? ctx->buffer.len - 1 : ctx->buffer.len;
            write_text(sstream, ctx->buffer.buf, n);
            ctx->bufcur = n;
            commit_buffer(ctx);
        }
    }
    fclose_e(hstream);
    fclose_e(sstream);
    if (ctx->errnum) {
        unlink(ctx->hname);
        unlink(ctx->sname);
        return FALSE;
    }
    return TRUE;
}