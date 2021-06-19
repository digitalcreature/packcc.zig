#include "node.h"

void node_array__init(node_array_t *array, size_t max) {
    array->len = 0;
    array->max = max;
    array->buf = (node_t **)malloc_e(sizeof(node_t *) * array->max);
}

void node_array__add(node_array_t *array, node_t *node) {
    if (array->max <= array->len) {
        const size_t n = array->len + 1;
        size_t m = array->max;
        if (m == 0) m = 1;
        while (m < n && m != 0) m <<= 1;
        if (m == 0) m = n; /* in case of shift overflow */
        array->buf = (node_t **)realloc_e(array->buf, sizeof(node_t *) * m);
        array->max = m;
    }
    array->buf[array->len++] = node;
}

void node_array__term(node_array_t *array) {
    while (array->len > 0) {
        array->len--;
        destroy_node(array->buf[array->len]);
    }
    free(array->buf);
}

void node_const_array__init(node_const_array_t *array, size_t max) {
    array->len = 0;
    array->max = max;
    array->buf = (const node_t **)malloc_e(sizeof(const node_t *) * array->max);
}

void node_const_array__add(node_const_array_t *array, const node_t *node) {
    if (array->max <= array->len) {
        const size_t n = array->len + 1;
        size_t m = array->max;
        if (m == 0) m = 1;
        while (m < n && m != 0) m <<= 1;
        if (m == 0) m = n; /* in case of shift overflow */
        array->buf = (const node_t **)realloc_e((node_t **)array->buf, sizeof(const node_t *) * m);
        array->max = m;
    }
    array->buf[array->len++] = node;
}

void node_const_array__clear(node_const_array_t *array) {
    array->len = 0;
}

void node_const_array__copy(node_const_array_t *array, const node_const_array_t *src) {
    size_t i;
    node_const_array__clear(array);
    for (i = 0; i < src->len; i++) {
        node_const_array__add(array, src->buf[i]);
    }
}

void node_const_array__term(node_const_array_t *array) {
    free((node_t **)array->buf);
}

node_t *create_node(node_type_t type) {
    node_t *const node = (node_t *)malloc_e(sizeof(node_t));
    node->type = type;
    switch (node->type) {
    case NODE_RULE:
        node->data.rule.name = NULL;
        node->data.rule.expr = NULL;
        node->data.rule.ref = 0;
        node_const_array__init(&node->data.rule.vars, ARRAY_INIT_SIZE);
        node_const_array__init(&node->data.rule.capts, ARRAY_INIT_SIZE);
        node_const_array__init(&node->data.rule.codes, ARRAY_INIT_SIZE);
        node->data.rule.line = VOID_VALUE;
        node->data.rule.col = VOID_VALUE;
        break;
    case NODE_REFERENCE:
        node->data.reference.var = NULL;
        node->data.reference.index = VOID_VALUE;
        node->data.reference.name = NULL;
        node->data.reference.rule = NULL;
        node->data.reference.line = VOID_VALUE;
        node->data.reference.col = VOID_VALUE;
        break;
    case NODE_STRING:
        node->data.string.value = NULL;
        break;
    case NODE_CHARCLASS:
        node->data.charclass.value = NULL;
        break;
    case NODE_QUANTITY:
        node->data.quantity.min = node->data.quantity.max = 0;
        node->data.quantity.expr = NULL;
        break;
    case NODE_PREDICATE:
        node->data.predicate.neg = FALSE;
        node->data.predicate.expr = NULL;
        break;
    case NODE_SEQUENCE:
        node_array__init(&node->data.sequence.nodes, ARRAY_INIT_SIZE);
        break;
    case NODE_ALTERNATE:
        node_array__init(&node->data.alternate.nodes, ARRAY_INIT_SIZE);
        break;
    case NODE_CAPTURE:
        node->data.capture.expr = NULL;
        node->data.capture.index = VOID_VALUE;
        break;
    case NODE_EXPAND:
        node->data.expand.index = VOID_VALUE;
        node->data.expand.line = VOID_VALUE;
        node->data.expand.col = VOID_VALUE;
        break;
    case NODE_ACTION:
        node->data.action.value = NULL;
        node->data.action.index = VOID_VALUE;
        node_const_array__init(&node->data.action.vars, ARRAY_INIT_SIZE);
        node_const_array__init(&node->data.action.capts, ARRAY_INIT_SIZE);
        break;
    case NODE_ERROR:
        node->data.error.expr = NULL;
        node->data.error.value = NULL;
        node->data.error.index = VOID_VALUE;
        node_const_array__init(&node->data.error.vars, ARRAY_INIT_SIZE);
        node_const_array__init(&node->data.error.capts, ARRAY_INIT_SIZE);
        break;
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
    return node;
}

void destroy_node(node_t *node) {
    if (node == NULL) return;
    switch (node->type) {
    case NODE_RULE:
        node_const_array__term(&node->data.rule.codes);
        node_const_array__term(&node->data.rule.capts);
        node_const_array__term(&node->data.rule.vars);
        destroy_node(node->data.rule.expr);
        free(node->data.rule.name);
        break;
    case NODE_REFERENCE:
        free(node->data.reference.name);
        free(node->data.reference.var);
        break;
    case NODE_STRING:
        free(node->data.string.value);
        break;
    case NODE_CHARCLASS:
        free(node->data.charclass.value);
        break;
    case NODE_QUANTITY:
        destroy_node(node->data.quantity.expr);
        break;
    case NODE_PREDICATE:
        destroy_node(node->data.predicate.expr);
        break;
    case NODE_SEQUENCE:
        node_array__term(&node->data.sequence.nodes);
        break;
    case NODE_ALTERNATE:
        node_array__term(&node->data.alternate.nodes);
        break;
    case NODE_CAPTURE:
        destroy_node(node->data.capture.expr);
        break;
    case NODE_EXPAND:
        break;
    case NODE_ACTION:
        node_const_array__term(&node->data.action.capts);
        node_const_array__term(&node->data.action.vars);
        free(node->data.action.value);
        break;
    case NODE_ERROR:
        node_const_array__term(&node->data.error.capts);
        node_const_array__term(&node->data.error.vars);
        free(node->data.error.value);
        destroy_node(node->data.error.expr);
        break;
    default:
        print_error("Internal error [%d]\n", __LINE__);
        exit(-1);
    }
    free(node);
}