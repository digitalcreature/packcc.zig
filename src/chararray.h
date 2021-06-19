
#ifndef __CHARARRAY_H
#define __CHARARRAY_H

#include "common.h"

typedef struct char_array_tag {
    char *buf;
    size_t max;
    size_t len;
} char_array_t;


void char_array__init(char_array_t *array, size_t max);
void char_array__add(char_array_t *array, char ch);
void char_array__append(char_array_t *array, const char *str, size_t len);
void char_array__term(char_array_t *array);

#endif