#include "chararray.h"

void char_array__init(char_array_t *array, size_t max) {
    array->len = 0;
    array->max = max;
    array->buf = (char *)malloc_e(array->max);
}

void char_array__add(char_array_t *array, char ch) {
    if (array->max <= array->len) {
        const size_t n = array->len + 1;
        size_t m = array->max;
        if (m == 0) m = 1;
        while (m < n && m != 0) m <<= 1;
        if (m == 0) m = n; /* in case of shift overflow */
        array->buf = (char *)realloc_e(array->buf, m);
        array->max = m;
    }
    array->buf[array->len++] = ch;
}

void char_array__append(char_array_t *array, const char *str, size_t len) {
    if (array->max < array->len + len) {
        const size_t n = array->len + len;
        size_t m = array->max;
        if (m == 0) m = 1;
        while (m < n && m != 0) m <<= 1;
        if (m == 0) m = n; /* in case of shift overflow */
        array->buf = (char *)realloc_e(array->buf, m);
        array->max = m;
    }
    memcpy(array->buf + array->len, str, len);
    array->len += len;
}

void char_array__term(char_array_t *array) {
    free(array->buf);
}