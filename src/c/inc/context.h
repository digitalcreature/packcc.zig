#ifndef __CONTEXT_H
#define __CONTEXT_H

#include "common.h"
#include "node.h"
#include "chararray.h"

typedef enum code_flag_tag {
    CODE_FLAG__NONE = 0,
    CODE_FLAG__UTF8_CHARCLASS_USED = 1
} code_flag_t;

typedef struct context_tag {
    char *iname;  /* the path name of the PEG file being parsed */
    char *sname;  /* the path name of the C source file being generated */
    char *hname;  /* the path name of the C header file being generated */
    FILE *ifile;  /* the input stream of the PEG file */
    char *hid;    /* the macro name for the include guard of the C header file */
    char *vtype;  /* the type name of the data output by the parsing API function (NULL means the default) */
    char *atype;  /* the type name of the user-defined data passed to the parser creation API function (NULL means the default) */
    char *prefix; /* the prefix of the API function names (NULL means the default) */
    bool_t ascii; /* UTF-8 support disabled if true  */
    bool_t debug; /* debug information is output if true */
    code_flag_t flags;   /* bitwise flags to control code generation; updated during PEG parsing */
    size_t errnum;       /* the current number of PEG parsing errors */
    size_t linenum;      /* the current line number (0-based) */
    size_t linepos;      /* the beginning position in the PEG file of the current line */
    size_t bufpos;       /* the position in the PEG file of the first character currently buffered */
    size_t bufcur;       /* the current parsing position in the character buffer */
    char_array_t buffer; /* the character buffer */
    node_array_t rules;  /* the PEG rules */
    node_hash_table_t rulehash; /* the hash table to accelerate access of desired PEG rules */
    char_array_t esource; /* the code blocks from %earlysource and %earlycommon directives to be added into the generated source file */
    char_array_t eheader; /* the code blocks from %earlyheader and %earlycommon directives to be added into the generated header file */
    char_array_t source;  /* the code blocks from %source and %common directives to be added into the generated source file */
    char_array_t header;  /* the code blocks from %header and %common directives to be added into the generated header file */
} context_t;

typedef enum string_flag_tag {
    STRING_FLAG__NONE = 0,
    STRING_FLAG__NOTEMPTY = 1,
    STRING_FLAG__NOTVOID = 2,
    STRING_FLAG__IDENTIFIER = 4
} string_flag_t;


context_t *create_context(const char *iname, const char *oname, bool_t ascii, bool_t debug);
void destroy_context(context_t *ctx);
size_t column_number(const context_t *ctx);
void make_rulehash(context_t *ctx);
const node_t *lookup_rulehash(const context_t *ctx, const char *name);
void link_references(context_t *ctx, node_t *node);
void verify_variables(context_t *ctx, node_t *node, node_const_array_t *vars);
void verify_captures(context_t *ctx, node_t *node, node_const_array_t *capts);
void dump_node(context_t *ctx, const node_t *node, const int indent);
size_t refill_buffer(context_t *ctx, size_t num);
void commit_buffer(context_t *ctx);
const char *get_value_type(context_t *ctx);
const char *get_auxil_type(context_t *ctx);
const char *get_prefix(context_t *ctx);
void dump_options(context_t *ctx);

#endif