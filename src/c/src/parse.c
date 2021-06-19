#include "parse.h"
#include "util.h"

node_t *parse_primary(context_t *ctx, node_t *rule) {
    const size_t p = ctx->bufcur;
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    node_t *n_p = NULL;
    if (match_identifier(ctx)) {
        const size_t q = ctx->bufcur;
        size_t r = VOID_VALUE, s = VOID_VALUE;
        match_spaces(ctx);
        if (match_character(ctx, ':')) {
            match_spaces(ctx);
            r = ctx->bufcur;
            if (!match_identifier(ctx)) goto EXCEPTION;
            s = ctx->bufcur;
            match_spaces(ctx);
        }
        if (match_string(ctx, "<-")) goto EXCEPTION;
        n_p = create_node(NODE_REFERENCE);
        if (r == VOID_VALUE) {
            assert(q >= p);
            n_p->data.reference.var = NULL;
            n_p->data.reference.index = VOID_VALUE;
            n_p->data.reference.name = strndup_e(ctx->buffer.buf + p, q - p);
        }
        else {
            assert(s != VOID_VALUE); /* s should have a valid value when r has a valid value */
            assert(q >= p);
            n_p->data.reference.var = strndup_e(ctx->buffer.buf + p, q - p);
            if (n_p->data.reference.var[0] == '_') {
                print_error("%s:" FMT_LU ":" FMT_LU ": Leading underscore in variable name '%s'\n",
                    ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), n_p->data.reference.var);
                ctx->errnum++;
            }
            {
                size_t i;
                for (i = 0; i < rule->data.rule.vars.len; i++) {
                    assert(rule->data.rule.vars.buf[i]->type == NODE_REFERENCE);
                    if (strcmp(n_p->data.reference.var, rule->data.rule.vars.buf[i]->data.reference.var) == 0) break;
                }
                if (i == rule->data.rule.vars.len) node_const_array__add(&rule->data.rule.vars, n_p);
                n_p->data.reference.index = i;
            }
            assert(s >= r);
            n_p->data.reference.name = strndup_e(ctx->buffer.buf + r, s - r);
        }
        n_p->data.reference.line = l;
        n_p->data.reference.col = m;
    }
    else if (match_character(ctx, '(')) {
        match_spaces(ctx);
        n_p = parse_expression(ctx, rule);
        if (n_p == NULL) goto EXCEPTION;
        if (!match_character(ctx, ')')) goto EXCEPTION;
        match_spaces(ctx);
    }
    else if (match_character(ctx, '<')) {
        match_spaces(ctx);
        n_p = create_node(NODE_CAPTURE);
        n_p->data.capture.index = rule->data.rule.capts.len;
        node_const_array__add(&rule->data.rule.capts, n_p);
        n_p->data.capture.expr = parse_expression(ctx, rule);
        if (n_p->data.capture.expr == NULL || !match_character(ctx, '>')) {
            rule->data.rule.capts.len = n_p->data.capture.index;
            goto EXCEPTION;
        }
        match_spaces(ctx);
    }
    else if (match_character(ctx, '$')) {
        size_t p;
        match_spaces(ctx);
        p = ctx->bufcur;
        if (match_number(ctx)) {
            const size_t q = ctx->bufcur;
            char *s;
            match_spaces(ctx);
            n_p = create_node(NODE_EXPAND);
            assert(q >= p);
            s = strndup_e(ctx->buffer.buf + p, q - p);
            n_p->data.expand.index = string_to_size_t(s);
            if (n_p->data.expand.index == VOID_VALUE) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Invalid unsigned number '%s'\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), s);
                ctx->errnum++;
            }
            else if (n_p->data.expand.index == 0) {
                print_error("%s:" FMT_LU ":" FMT_LU ": 0 not allowed\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
                ctx->errnum++;
            }
            else if (s[0] == '0') {
                print_error("%s:" FMT_LU ":" FMT_LU ": 0-prefixed number not allowed\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
                ctx->errnum++;
                n_p->data.expand.index = 0;
            }
            free(s);
            if (n_p->data.expand.index > 0 && n_p->data.expand.index != VOID_VALUE) {
                n_p->data.expand.index--;
                n_p->data.expand.line = l;
                n_p->data.expand.col = m;
            }
        }
        else {
            goto EXCEPTION;
        }
    }
    else if (match_character(ctx, '.')) {
        match_spaces(ctx);
        n_p = create_node(NODE_CHARCLASS);
        n_p->data.charclass.value = NULL;
        if (!ctx->ascii) {
            ctx->flags |= CODE_FLAG__UTF8_CHARCLASS_USED;
        }
    }
    else if (match_character_class(ctx)) {
        const size_t q = ctx->bufcur;
        match_spaces(ctx);
        n_p = create_node(NODE_CHARCLASS);
        n_p->data.charclass.value = strndup_e(ctx->buffer.buf + p + 1, q - p - 2);
        if (!unescape_string(n_p->data.charclass.value, TRUE)) {
            print_error("%s:" FMT_LU ":" FMT_LU ": Illegal escape sequence\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
            ctx->errnum++;
        }
        if (!ctx->ascii && !is_valid_utf8_string(n_p->data.charclass.value)) {
            print_error("%s:" FMT_LU ":" FMT_LU ": Invalid UTF-8 string\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
            ctx->errnum++;
        }
        if (!ctx->ascii && n_p->data.charclass.value[0] != '\0') {
            ctx->flags |= CODE_FLAG__UTF8_CHARCLASS_USED;
        }
    }
    else if (match_quotation_single(ctx) || match_quotation_double(ctx)) {
        const size_t q = ctx->bufcur;
        match_spaces(ctx);
        n_p = create_node(NODE_STRING);
        n_p->data.string.value = strndup_e(ctx->buffer.buf + p + 1, q - p - 2);
        if (!unescape_string(n_p->data.string.value, FALSE)) {
            print_error("%s:" FMT_LU ":" FMT_LU ": Illegal escape sequence\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
            ctx->errnum++;
        }
        if (!ctx->ascii && !is_valid_utf8_string(n_p->data.string.value)) {
            print_error("%s:" FMT_LU ":" FMT_LU ": Invalid UTF-8 string\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
            ctx->errnum++;
        }
    }
    else if (match_code_block(ctx)) {
        const size_t q = ctx->bufcur;
        match_spaces(ctx);
        n_p = create_node(NODE_ACTION);
        n_p->data.action.value = strndup_e(ctx->buffer.buf + p + 1, q - p - 2);
        n_p->data.action.index = rule->data.rule.codes.len;
        node_const_array__add(&rule->data.rule.codes, n_p);
    }
    else {
        goto EXCEPTION;
    }
    return n_p;

EXCEPTION:;
    destroy_node(n_p);
    ctx->bufcur = p;
    ctx->linenum = l;
    ctx->linepos = ctx->bufpos + p - m;
    return NULL;
}

node_t *parse_term(context_t *ctx, node_t *rule) {
    const size_t p = ctx->bufcur;
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    node_t *n_p = NULL;
    node_t *n_q = NULL;
    node_t *n_r = NULL;
    node_t *n_t = NULL;
    const char t = match_character(ctx, '&') ? '&' : match_character(ctx, '!') ? '!' : '\0';
    if (t) match_spaces(ctx);
    n_p = parse_primary(ctx, rule);
    if (n_p == NULL) goto EXCEPTION;
    if (match_character(ctx, '*')) {
        match_spaces(ctx);
        n_q = create_node(NODE_QUANTITY);
        n_q->data.quantity.min = 0;
        n_q->data.quantity.max = -1;
        n_q->data.quantity.expr = n_p;
    }
    else if (match_character(ctx, '+')) {
        match_spaces(ctx);
        n_q = create_node(NODE_QUANTITY);
        n_q->data.quantity.min = 1;
        n_q->data.quantity.max = -1;
        n_q->data.quantity.expr = n_p;
    }
    else if (match_character(ctx, '?')) {
        match_spaces(ctx);
        n_q = create_node(NODE_QUANTITY);
        n_q->data.quantity.min = 0;
        n_q->data.quantity.max = 1;
        n_q->data.quantity.expr = n_p;
    }
    else {
        n_q = n_p;
    }
    switch (t) {
    case '&':
        n_r = create_node(NODE_PREDICATE);
        n_r->data.predicate.neg = FALSE;
        n_r->data.predicate.expr = n_q;
        break;
    case '!':
        n_r = create_node(NODE_PREDICATE);
        n_r->data.predicate.neg = TRUE;
        n_r->data.predicate.expr = n_q;
        break;
    default:
        n_r = n_q;
    }
    if (match_character(ctx, '~')) {
        size_t p;
        match_spaces(ctx);
        p = ctx->bufcur;
        if (match_code_block(ctx)) {
            const size_t q = ctx->bufcur;
            match_spaces(ctx);
            n_t = create_node(NODE_ERROR);
            n_t->data.error.expr = n_r;
            n_t->data.error.value = strndup_e(ctx->buffer.buf + p + 1, q - p - 2);
            n_t->data.error.index = rule->data.rule.codes.len;
            node_const_array__add(&rule->data.rule.codes, n_t);
        }
        else {
            goto EXCEPTION;
        }
    }
    else {
        n_t = n_r;
    }
    return n_t;

EXCEPTION:;
    destroy_node(n_r);
    ctx->bufcur = p;
    ctx->linenum = l;
    ctx->linepos = ctx->bufpos + p - m;
    return NULL;
}

node_t *parse_sequence(context_t *ctx, node_t *rule) {
    const size_t p = ctx->bufcur;
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    node_array_t *a_t = NULL;
    node_t *n_t = NULL;
    node_t *n_u = NULL;
    node_t *n_s = NULL;
    n_t = parse_term(ctx, rule);
    if (n_t == NULL) goto EXCEPTION;
    n_u = parse_term(ctx, rule);
    if (n_u != NULL) {
        n_s = create_node(NODE_SEQUENCE);
        a_t = &n_s->data.sequence.nodes;
        node_array__add(a_t, n_t);
        node_array__add(a_t, n_u);
        while ((n_t = parse_term(ctx, rule)) != NULL) {
            node_array__add(a_t, n_t);
        }
    }
    else {
        n_s = n_t;
    }
    return n_s;

EXCEPTION:;
    ctx->bufcur = p;
    ctx->linenum = l;
    ctx->linepos = ctx->bufpos + p - m;
    return NULL;
}

node_t *parse_expression(context_t *ctx, node_t *rule) {
    const size_t p = ctx->bufcur;
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    size_t q;
    node_array_t *a_s = NULL;
    node_t *n_s = NULL;
    node_t *n_e = NULL;
    n_s = parse_sequence(ctx, rule);
    if (n_s == NULL) goto EXCEPTION;
    q = ctx->bufcur;
    if (match_character(ctx, '/')) {
        ctx->bufcur = q;
        n_e = create_node(NODE_ALTERNATE);
        a_s = &n_e->data.alternate.nodes;
        node_array__add(a_s, n_s);
        while (match_character(ctx, '/')) {
            match_spaces(ctx);
            n_s = parse_sequence(ctx, rule);
            if (n_s == NULL) goto EXCEPTION;
            node_array__add(a_s, n_s);
        }
    }
    else {
        n_e = n_s;
    }
    return n_e;

EXCEPTION:;
    destroy_node(n_e);
    ctx->bufcur = p;
    ctx->linenum = l;
    ctx->linepos = ctx->bufpos + p - m;
    return NULL;
}

node_t *parse_rule(context_t *ctx) {
    const size_t p = ctx->bufcur;
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    size_t q;
    node_t *n_r = NULL;
    if (!match_identifier(ctx)) goto EXCEPTION;
    q = ctx->bufcur;
    match_spaces(ctx);
    if (!match_string(ctx, "<-")) goto EXCEPTION;
    match_spaces(ctx);
    n_r = create_node(NODE_RULE);
    n_r->data.rule.expr = parse_expression(ctx, n_r);
    if (n_r->data.rule.expr == NULL) goto EXCEPTION;
    assert(q >= p);
    n_r->data.rule.name = strndup_e(ctx->buffer.buf + p, q - p);
    n_r->data.rule.line = l;
    n_r->data.rule.col = m;
    return n_r;

EXCEPTION:;
    destroy_node(n_r);
    ctx->bufcur = p;
    ctx->linenum = l;
    ctx->linepos = ctx->bufpos + p - m;
    return NULL;
}

bool_t parse_directive_include_(context_t *ctx, const char *name, char_array_t *output1, char_array_t *output2) {
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    if (!match_string(ctx, name)) return FALSE;
    match_spaces(ctx);
    {
        const size_t p = ctx->bufcur;
        if (match_code_block(ctx)) {
            const size_t q = ctx->bufcur;
            match_spaces(ctx);
            if (output1 != NULL) {
                char_array__append(output1, ctx->buffer.buf + p + 1, q - p - 2);
                char_array__add(output1, '\n');
            }
            if (output2 != NULL) {
                char_array__append(output2, ctx->buffer.buf + p + 1, q - p - 2);
                char_array__add(output2, '\n');
            }
        }
        else {
            print_error("%s:" FMT_LU ":" FMT_LU ": Illegal %s syntax\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
            ctx->errnum++;
        }
    }
    return TRUE;
}

bool_t parse_directive_string_(context_t *ctx, const char *name, char **output, string_flag_t mode) {
    const size_t l = ctx->linenum;
    const size_t m = column_number(ctx);
    if (!match_string(ctx, name)) return FALSE;
    match_spaces(ctx);
    {
        char *s = NULL;
        const size_t p = ctx->bufcur;
        const size_t lv = ctx->linenum;
        const size_t mv = column_number(ctx);
        size_t q;
        if (match_quotation_single(ctx) || match_quotation_double(ctx)) {
            q = ctx->bufcur;
            match_spaces(ctx);
            s = strndup_e(ctx->buffer.buf + p + 1, q - p - 2);
            if (!unescape_string(s, FALSE)) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Illegal escape sequence\n", ctx->iname, (ulong_t)(lv + 1), (ulong_t)(mv + 1));
                ctx->errnum++;
            }
        }
        else {
            print_error("%s:" FMT_LU ":" FMT_LU ": Illegal %s syntax\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
            ctx->errnum++;
        }
        if (s != NULL) {
            string_flag_t f = STRING_FLAG__NONE;
            bool_t b = TRUE;
            remove_heading_blank(s);
            remove_trailing_blank(s);
            assert((mode & ~7) == 0);
            if ((mode & STRING_FLAG__NOTEMPTY) && !is_filled_string(s)) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Empty string\n", ctx->iname, (ulong_t)(lv + 1), (ulong_t)(mv + 1));
                ctx->errnum++;
                f |= STRING_FLAG__NOTEMPTY;
            }
            if ((mode & STRING_FLAG__NOTVOID) && strcmp(s, "void") == 0) {
                print_error("%s:" FMT_LU ":" FMT_LU ": 'void' not allowed\n", ctx->iname, (ulong_t)(lv + 1), (ulong_t)(mv + 1));
                ctx->errnum++;
                f |= STRING_FLAG__NOTVOID;
            }
            if ((mode & STRING_FLAG__IDENTIFIER) && !is_identifier_string(s)) {
                if (!(f & STRING_FLAG__NOTEMPTY)) {
                    print_error("%s:" FMT_LU ":" FMT_LU ": Invalid identifier\n", ctx->iname, (ulong_t)(lv + 1), (ulong_t)(mv + 1));
                    ctx->errnum++;
                }
                f |= STRING_FLAG__IDENTIFIER;
            }
            if (*output != NULL) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Multiple %s definition\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1), name);
                ctx->errnum++;
                b = FALSE;
            }
            if (f == STRING_FLAG__NONE && b) {
                *output = s;
            }
            else {
                free(s); s = NULL;
            }
        }
    }
    return TRUE;
}

bool_t parse(context_t *ctx) {
    {
        bool_t b = TRUE;
        match_spaces(ctx);
        for (;;) {
            size_t p, l, m;
            if (match_eof(ctx) || match_footer_start(ctx)) break;
            p = ctx->bufcur;
            l = ctx->linenum;
            m = column_number(ctx);
            if (
                parse_directive_include_(ctx, "%earlysource", &ctx->esource, NULL) ||
                parse_directive_include_(ctx, "%earlyheader", &ctx->eheader, NULL) ||
                parse_directive_include_(ctx, "%earlycommon", &ctx->esource, &ctx->eheader) ||
                parse_directive_include_(ctx, "%source", &ctx->source, NULL) ||
                parse_directive_include_(ctx, "%header", &ctx->header, NULL) ||
                parse_directive_include_(ctx, "%common", &ctx->source, &ctx->header) ||
                parse_directive_string_(ctx, "%value", &ctx->vtype, STRING_FLAG__NOTEMPTY | STRING_FLAG__NOTVOID) ||
                parse_directive_string_(ctx, "%auxil", &ctx->atype, STRING_FLAG__NOTEMPTY | STRING_FLAG__NOTVOID) ||
                parse_directive_string_(ctx, "%prefix", &ctx->prefix, STRING_FLAG__NOTEMPTY | STRING_FLAG__IDENTIFIER)
            ) {
                b = TRUE;
            }
            else if (match_character(ctx, '%')) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Invalid directive\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
                ctx->errnum++;
                match_identifier(ctx);
                match_spaces(ctx);
                b = TRUE;
            }
            else {
                node_t *const n_r = parse_rule(ctx);
                if (n_r == NULL) {
                    if (b) {
                        print_error("%s:" FMT_LU ":" FMT_LU ": Illegal rule syntax\n", ctx->iname, (ulong_t)(l + 1), (ulong_t)(m + 1));
                        ctx->errnum++;
                        b = FALSE;
                    }
                    ctx->linenum = l;
                    ctx->linepos = ctx->bufpos + p - m;
                    if (!match_identifier(ctx) && !match_spaces(ctx)) match_character_any(ctx);
                    continue;
                }
                node_array__add(&ctx->rules, n_r);
                b = TRUE;
            }
            commit_buffer(ctx);
        }
        commit_buffer(ctx);
    }
    {
        size_t i;
        make_rulehash(ctx);
        for (i = 0; i < ctx->rules.len; i++) {
            link_references(ctx, ctx->rules.buf[i]->data.rule.expr);
        }
        for (i = 1; i < ctx->rules.len; i++) {
            if (ctx->rules.buf[i]->data.rule.ref == 0) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Never used rule '%s'\n",
                    ctx->iname,
                    (ulong_t)(ctx->rules.buf[i]->data.rule.line + 1), (ulong_t)(ctx->rules.buf[i]->data.rule.col + 1),
                    ctx->rules.buf[i]->data.rule.name);
                ctx->errnum++;
            }
            else if (ctx->rules.buf[i]->data.rule.ref < 0) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Multiple definition of rule '%s'\n",
                    ctx->iname,
                    (ulong_t)(ctx->rules.buf[i]->data.rule.line + 1), (ulong_t)(ctx->rules.buf[i]->data.rule.col + 1),
                    ctx->rules.buf[i]->data.rule.name);
                ctx->errnum++;
            }
        }
    }
    {
        size_t i;
        for (i = 0; i < ctx->rules.len; i++) {
            verify_variables(ctx, ctx->rules.buf[i]->data.rule.expr, NULL);
            verify_captures(ctx, ctx->rules.buf[i]->data.rule.expr, NULL);
        }
    }
    if (ctx->debug) {
        size_t i;
        for (i = 0; i < ctx->rules.len; i++) {
            dump_node(ctx, ctx->rules.buf[i], 0);
        }
        dump_options(ctx);
    }
    return (ctx->errnum == 0) ? TRUE : FALSE;
}