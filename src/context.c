#include "context.h"
#include "util.h"

context_t *create_context(const char *iname, const char *oname, bool_t ascii, bool_t debug) {
    context_t *const ctx = (context_t *)malloc_e(sizeof(context_t));
    ctx->iname = strdup_e((iname && iname[0]) ? iname : "-");
    ctx->sname = (oname && oname[0]) ? add_fileext(oname, "c") : replace_fileext(ctx->iname, "c");
    ctx->hname = (oname && oname[0]) ? add_fileext(oname, "h") : replace_fileext(ctx->iname, "h");
    ctx->ifile = (iname && iname[0]) ? fopen_rb_e(ctx->iname) : stdin;
    ctx->hid = strdup_e(ctx->hname); make_header_identifier(ctx->hid);
    ctx->vtype = NULL;
    ctx->atype = NULL;
    ctx->prefix = NULL;
    ctx->ascii = ascii;
    ctx->debug = debug;
    ctx->flags = CODE_FLAG__NONE;
    ctx->errnum = 0;
    ctx->linenum = 0;
    ctx->linepos = 0;
    ctx->bufpos = 0;
    ctx->bufcur = 0;
    char_array__init(&ctx->buffer, BUFFER_INIT_SIZE);
    node_array__init(&ctx->rules, ARRAY_INIT_SIZE);
    ctx->rulehash.mod = 0;
    ctx->rulehash.max = 0;
    ctx->rulehash.buf = NULL;
    char_array__init(&ctx->esource, BUFFER_INIT_SIZE);
    char_array__init(&ctx->eheader, BUFFER_INIT_SIZE);
    char_array__init(&ctx->source, BUFFER_INIT_SIZE);
    char_array__init(&ctx->header, BUFFER_INIT_SIZE);
    return ctx;
}

void destroy_context(context_t *ctx) {
    if (ctx == NULL) return;
    char_array__term(&ctx->header);
    char_array__term(&ctx->source);
    char_array__term(&ctx->eheader);
    char_array__term(&ctx->esource);
    free((node_t **)ctx->rulehash.buf);
    node_array__term(&ctx->rules);
    char_array__term(&ctx->buffer);
    free(ctx->prefix);
    free(ctx->atype);
    free(ctx->vtype);
    free(ctx->hid);
    fclose_e(ctx->ifile);
    free(ctx->hname);
    free(ctx->sname);
    free(ctx->iname);
    free(ctx);
}

size_t column_number(const context_t *ctx) { /* 0-based */
    assert(ctx->bufpos + ctx->bufcur >= ctx->linepos);
    return ctx->bufpos + ctx->bufcur - ctx->linepos;
}

void make_rulehash(context_t *ctx) {
    size_t i, j;
    ctx->rulehash.mod = populate_bits(ctx->rules.len * 4);
    ctx->rulehash.max = ctx->rulehash.mod + 1;
    ctx->rulehash.buf = (const node_t **)realloc_e((node_t **)ctx->rulehash.buf, sizeof(const node_t *) * ctx->rulehash.max);
    for (i = 0; i < ctx->rulehash.max; i++) {
        ctx->rulehash.buf[i] = NULL;
    }
    for (i = 0; i < ctx->rules.len; i++) {
        assert(ctx->rules.buf[i]->type == NODE_RULE);
        j = hash_string(ctx->rules.buf[i]->data.rule.name) & ctx->rulehash.mod;
        while (ctx->rulehash.buf[j] != NULL) {
            if (strcmp(ctx->rules.buf[i]->data.rule.name, ctx->rulehash.buf[j]->data.rule.name) == 0) {
                assert(ctx->rules.buf[i]->data.rule.ref == 0);
                assert(ctx->rulehash.buf[j]->data.rule.ref == 0);
                ctx->rules.buf[i]->data.rule.ref = -1;
                goto EXCEPTION;
            }
            j = (j + 1) & ctx->rulehash.mod;
        }
        ctx->rulehash.buf[j] = ctx->rules.buf[i];

    EXCEPTION:;
    }
}

const node_t *lookup_rulehash(const context_t *ctx, const char *name) {
    size_t j = hash_string(name) & ctx->rulehash.mod;
    while (ctx->rulehash.buf[j] != NULL && strcmp(name, ctx->rulehash.buf[j]->data.rule.name) != 0) {
        j = (j + 1) & ctx->rulehash.mod;
    }
    return (ctx->rulehash.buf[j] != NULL) ? ctx->rulehash.buf[j] : NULL;
}

void link_references(context_t *ctx, node_t *node) {
    if (node == NULL) return;
    switch (node->type) {
    case NODE_RULE:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    case NODE_REFERENCE:
        node->data.reference.rule = lookup_rulehash(ctx, node->data.reference.name);
        if (node->data.reference.rule == NULL) {
            print_error("%s:" FMT_LU ":" FMT_LU ": No definition of rule '%s'\n",
                ctx->iname, (ulong_t)(node->data.reference.line + 1), (ulong_t)(node->data.reference.col + 1),
                node->data.reference.name);
            ctx->errnum++;
        }
        else {
            assert(node->data.reference.rule->type == NODE_RULE);
            ((node_t *)node->data.reference.rule)->data.rule.ref++;
        }
        break;
    case NODE_STRING:
        break;
    case NODE_CHARCLASS:
        break;
    case NODE_QUANTITY:
        link_references(ctx, node->data.quantity.expr);
        break;
    case NODE_PREDICATE:
        link_references(ctx, node->data.predicate.expr);
        break;
    case NODE_SEQUENCE:
        {
            size_t i;
            for (i = 0; i < node->data.sequence.nodes.len; i++) {
                link_references(ctx, node->data.sequence.nodes.buf[i]);
            }
        }
        break;
    case NODE_ALTERNATE:
        {
            size_t i;
            for (i = 0; i < node->data.alternate.nodes.len; i++) {
                link_references(ctx, node->data.alternate.nodes.buf[i]);
            }
        }
        break;
    case NODE_CAPTURE:
        link_references(ctx, node->data.capture.expr);
        break;
    case NODE_EXPAND:
        break;
    case NODE_ACTION:
        break;
    case NODE_ERROR:
        link_references(ctx, node->data.error.expr);
        break;
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
}

void verify_variables(context_t *ctx, node_t *node, node_const_array_t *vars) {
    node_const_array_t a;
    const bool_t b = (vars == NULL) ? TRUE : FALSE;
    if (node == NULL) return;
    if (b) {
        node_const_array__init(&a, ARRAY_INIT_SIZE);
        vars = &a;
    }
    switch (node->type) {
    case NODE_RULE:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    case NODE_REFERENCE:
        if (node->data.reference.index != VOID_VALUE) {
            size_t i;
            for (i = 0; i < vars->len; i++) {
                assert(vars->buf[i]->type == NODE_REFERENCE);
                if (node->data.reference.index == vars->buf[i]->data.reference.index) break;
            }
            if (i == vars->len) node_const_array__add(vars, node);
        }
        break;
    case NODE_STRING:
        break;
    case NODE_CHARCLASS:
        break;
    case NODE_QUANTITY:
        verify_variables(ctx, node->data.quantity.expr, vars);
        break;
    case NODE_PREDICATE:
        verify_variables(ctx, node->data.predicate.expr, vars);
        break;
    case NODE_SEQUENCE:
        {
            size_t i;
            for (i = 0; i < node->data.sequence.nodes.len; i++) {
                verify_variables(ctx, node->data.sequence.nodes.buf[i], vars);
            }
        }
        break;
    case NODE_ALTERNATE:
        {
            size_t i, j, k, m = vars->len;
            node_const_array_t v;
            node_const_array__init(&v, ARRAY_INIT_SIZE);
            node_const_array__copy(&v, vars);
            for (i = 0; i < node->data.alternate.nodes.len; i++) {
                v.len = m;
                verify_variables(ctx, node->data.alternate.nodes.buf[i], &v);
                for (j = m; j < v.len; j++) {
                    for (k = m; k < vars->len; k++) {
                        if (v.buf[j]->data.reference.index == vars->buf[k]->data.reference.index) break;
                    }
                    if (k == vars->len) node_const_array__add(vars, v.buf[j]);
                }
            }
            node_const_array__term(&v);
        }
        break;
    case NODE_CAPTURE:
        verify_variables(ctx, node->data.capture.expr, vars);
        break;
    case NODE_EXPAND:
        break;
    case NODE_ACTION:
        node_const_array__copy(&node->data.action.vars, vars);
        break;
    case NODE_ERROR:
        node_const_array__copy(&node->data.error.vars, vars);
        verify_variables(ctx, node->data.error.expr, vars);
        break;
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
    if (b) {
        node_const_array__term(&a);
    }
}

void verify_captures(context_t *ctx, node_t *node, node_const_array_t *capts) {
    node_const_array_t a;
    const bool_t b = (capts == NULL) ? TRUE : FALSE;
    if (node == NULL) return;
    if (b) {
        node_const_array__init(&a, ARRAY_INIT_SIZE);
        capts = &a;
    }
    switch (node->type) {
    case NODE_RULE:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    case NODE_REFERENCE:
        break;
    case NODE_STRING:
        break;
    case NODE_CHARCLASS:
        break;
    case NODE_QUANTITY:
        verify_captures(ctx, node->data.quantity.expr, capts);
        break;
    case NODE_PREDICATE:
        verify_captures(ctx, node->data.predicate.expr, capts);
        break;
    case NODE_SEQUENCE:
        {
            size_t i;
            for (i = 0; i < node->data.sequence.nodes.len; i++) {
                verify_captures(ctx, node->data.sequence.nodes.buf[i], capts);
            }
        }
        break;
    case NODE_ALTERNATE:
        {
            size_t i, j, m = capts->len;
            node_const_array_t v;
            node_const_array__init(&v, ARRAY_INIT_SIZE);
            node_const_array__copy(&v, capts);
            for (i = 0; i < node->data.alternate.nodes.len; i++) {
                v.len = m;
                verify_captures(ctx, node->data.alternate.nodes.buf[i], &v);
                for (j = m; j < v.len; j++) {
                    node_const_array__add(capts, v.buf[j]);
                }
            }
            node_const_array__term(&v);
        }
        break;
    case NODE_CAPTURE:
        verify_captures(ctx, node->data.capture.expr, capts);
        node_const_array__add(capts, node);
        break;
    case NODE_EXPAND:
        {
            size_t i;
            for (i = 0; i < capts->len; i++) {
                assert(capts->buf[i]->type == NODE_CAPTURE);
                if (node->data.expand.index == capts->buf[i]->data.capture.index) break;
            }
            if (i >= capts->len && node->data.expand.index != VOID_VALUE) {
                print_error("%s:" FMT_LU ":" FMT_LU ": Capture %d not available at this position\n",
                    ctx->iname, (ulong_t)(node->data.expand.line + 1), (ulong_t)(node->data.expand.col + 1), node->data.expand.index + 1);
                ctx->errnum++;
            }
        }
        break;
    case NODE_ACTION:
        node_const_array__copy(&node->data.action.capts, capts);
        break;
    case NODE_ERROR:
        node_const_array__copy(&node->data.error.capts, capts);
        verify_captures(ctx, node->data.error.expr, capts);
        break;
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
    if (b) {
        node_const_array__term(&a);
    }
}

void dump_node(context_t *ctx, const node_t *node, const int indent) {
    if (node == NULL) return;
    switch (node->type) {
    case NODE_RULE:
        fprintf(stdout, "%*sRule(name:'%s', ref:%d, vars.len:" FMT_LU ", capts.len:" FMT_LU ", codes.len:" FMT_LU ") {\n",
            indent, "", node->data.rule.name, node->data.rule.ref,
            (ulong_t)node->data.rule.vars.len, (ulong_t)node->data.rule.capts.len, (ulong_t)node->data.rule.codes.len);
        dump_node(ctx, node->data.rule.expr, indent + 2);
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_REFERENCE:
        fprintf(stdout, "%*sReference(var:'%s', index:", indent, "", node->data.reference.var);
        dump_void_value(node->data.reference.index);
        fprintf(stdout, ", name:'%s', rule:'%s')\n", node->data.reference.name,
            (node->data.reference.rule) ? node->data.reference.rule->data.rule.name : NULL);
        break;
    case NODE_STRING:
        fprintf(stdout, "%*sString(value:'", indent, "");
        dump_escaped(node->data.string.value);
        fprintf(stdout, "')\n");
        break;
    case NODE_CHARCLASS:
        fprintf(stdout, "%*sCharclass(value:'", indent, "");
        dump_escaped(node->data.charclass.value);
        fprintf(stdout, "')\n");
        break;
    case NODE_QUANTITY:
        fprintf(stdout, "%*sQuantity(min:%d, max%d) {\n", indent, "", node->data.quantity.min, node->data.quantity.max);
        dump_node(ctx, node->data.quantity.expr, indent + 2);
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_PREDICATE:
        fprintf(stdout, "%*sPredicate(neg:%d) {\n", indent, "", node->data.predicate.neg);
        dump_node(ctx, node->data.predicate.expr, indent + 2);
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_SEQUENCE:
        fprintf(stdout, "%*sSequence(max:" FMT_LU ", len:" FMT_LU ") {\n",
            indent, "", (ulong_t)node->data.sequence.nodes.max, (ulong_t)node->data.sequence.nodes.len);
        {
            size_t i;
            for (i = 0; i < node->data.sequence.nodes.len; i++) {
                dump_node(ctx, node->data.sequence.nodes.buf[i], indent + 2);
            }
        }
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_ALTERNATE:
        fprintf(stdout, "%*sAlternate(max:" FMT_LU ", len:" FMT_LU ") {\n",
            indent, "", (ulong_t)node->data.alternate.nodes.max, (ulong_t)node->data.alternate.nodes.len);
        {
            size_t i;
            for (i = 0; i < node->data.alternate.nodes.len; i++) {
                dump_node(ctx, node->data.alternate.nodes.buf[i], indent + 2);
            }
        }
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_CAPTURE:
        fprintf(stdout, "%*sCapture(index:", indent, "");
        dump_void_value(node->data.capture.index);
        fprintf(stdout, ") {\n");
        dump_node(ctx, node->data.capture.expr, indent + 2);
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    case NODE_EXPAND:
        fprintf(stdout, "%*sExpand(index:", indent, "");
        dump_void_value(node->data.expand.index);
        fprintf(stdout, ")\n");
        break;
    case NODE_ACTION:
        fprintf(stdout, "%*sAction(index:", indent, "");
        dump_void_value(node->data.action.index);
        fprintf(stdout, ", value:{");
        dump_escaped(node->data.action.value);
        fprintf(stdout, "}, vars:");
        if (node->data.action.vars.len + node->data.action.capts.len > 0) {
            size_t i;
            fprintf(stdout, "\n");
            for (i = 0; i < node->data.action.vars.len; i++) {
                fprintf(stdout, "%*s'%s'\n", indent + 2, "", node->data.action.vars.buf[i]->data.reference.var);
            }
            for (i = 0; i < node->data.action.capts.len; i++) {
                fprintf(stdout, "%*s$" FMT_LU "\n", indent + 2, "", (ulong_t)(node->data.action.capts.buf[i]->data.capture.index + 1));
            }
            fprintf(stdout, "%*s)\n", indent, "");
        }
        else {
            fprintf(stdout, "none)\n");
        }
        break;
    case NODE_ERROR:
        fprintf(stdout, "%*sError(index:", indent, "");
        dump_void_value(node->data.error.index);
        fprintf(stdout, ", value:{");
        dump_escaped(node->data.error.value);
        fprintf(stdout, "}, vars:\n");
        {
            size_t i;
            for (i = 0; i < node->data.error.vars.len; i++) {
                fprintf(stdout, "%*s'%s'\n", indent + 2, "", node->data.error.vars.buf[i]->data.reference.var);
            }
            for (i = 0; i < node->data.error.capts.len; i++) {
                fprintf(stdout, "%*s$" FMT_LU "\n", indent + 2, "", (ulong_t)(node->data.error.capts.buf[i]->data.capture.index + 1));
            }
        }
        fprintf(stdout, "%*s) {\n", indent, "");
        dump_node(ctx, node->data.error.expr, indent + 2);
        fprintf(stdout, "%*s}\n", indent, "");
        break;
    default:
        print_error("%*sInternal error [%d]\n", indent, "", __LINE__);
        exit(-1);
    }
}

size_t refill_buffer(context_t *ctx, size_t num) {
    if (ctx->buffer.len >= ctx->bufcur + num) return ctx->buffer.len - ctx->bufcur;
    while (ctx->buffer.len < ctx->bufcur + num) {
        const int c = fgetc_e(ctx->ifile);
        if (c == EOF) break;
        char_array__add(&ctx->buffer, (char)c);
    }
    return ctx->buffer.len - ctx->bufcur;
}

void commit_buffer(context_t *ctx) {
    assert(ctx->buffer.len >= ctx->bufcur);
    memmove(ctx->buffer.buf, ctx->buffer.buf + ctx->bufcur, ctx->buffer.len - ctx->bufcur);
    ctx->buffer.len -= ctx->bufcur;
    ctx->bufpos += ctx->bufcur;
    ctx->bufcur = 0;
}

const char *get_value_type(context_t *ctx) {
    return (ctx->vtype && ctx->vtype[0]) ? ctx->vtype : "int";
}

const char *get_auxil_type(context_t *ctx) {
    return (ctx->atype && ctx->atype[0]) ? ctx->atype : "void *";
}

const char *get_prefix(context_t *ctx) {
    return (ctx->prefix && ctx->prefix[0]) ? ctx->prefix : "pcc";
}

void dump_options(context_t *ctx) {
    fprintf(stdout, "value_type: '%s'\n", get_value_type(ctx));
    fprintf(stdout, "auxil_type: '%s'\n", get_auxil_type(ctx));
    fprintf(stdout, "prefix: '%s'\n", get_prefix(ctx));
}