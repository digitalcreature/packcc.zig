#ifndef __UTIL_H
#define __UTIL_H

#include "common.h"

size_t string_to_size_t(const char *str);
size_t find_first_trailing_space(const char *str, size_t start, size_t end, size_t *next);
size_t count_indent_spaces(const char *str, size_t start, size_t end, size_t *next);
bool_t is_filled_string(const char *str);
bool_t is_identifier_string(const char *str);
bool_t is_pointer_type(const char *str);
bool_t is_valid_utf8_string(const char *str);
size_t utf8_to_utf32(const char *seq, int *out);
bool_t unescape_string(char *str, bool_t cls);
const char *escape_character(char ch, char (*buf)[5]);
void remove_heading_blank(char *str);
void remove_trailing_blank(char *str);
void make_header_identifier(char *str);
void write_characters(FILE *stream, char ch, size_t len);
void write_text(FILE *stream, const char *ptr, size_t len);
void write_code_block(FILE *stream, const char *ptr, size_t len, size_t indent);
const char *extract_filename(const char *path);
const char *extract_fileext(const char *path);
char *replace_fileext(const char *path, const char *ext);
char *add_fileext(const char *path, const char *ext);
size_t hash_string(const char *str);
size_t populate_bits(size_t x);

void dump_escaped(const char *s);
void dump_void_value(size_t value);

#endif