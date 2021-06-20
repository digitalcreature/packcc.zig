#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#ifndef _MSC_VER
#if defined __GNUC__ && defined _WIN32 /* MinGW */
#ifndef PCC_USE_SYSTEM_STRNLEN
#define strnlen(str, maxlen) strnlen_(str, maxlen)
size_t strnlen_(const char *str, size_t maxlen) {
    size_t i;
    for (i = 0; i < maxlen && str[i]; i++);
    return i;
}
#endif /* !PCC_USE_SYSTEM_STRNLEN */
#endif /* defined __GNUC__ && defined _WIN32 */
#endif /* !_MSC_VER */

#ifdef _MSC_VER
#define snprintf _snprintf
#define unlink _unlink
#else
#include <unistd.h> /* for unlink() */
#endif

#ifndef __attribute__
#define __attribute__(x)
#endif

#undef TRUE  /* to avoid macro definition conflicts with the system header file of IBM AIX */
#undef FALSE

#define VERSION "1.5.0 Z"

#ifndef BUFFER_INIT_SIZE
#define BUFFER_INIT_SIZE 256
#endif
#ifndef ARRAY_INIT_SIZE
#define ARRAY_INIT_SIZE 2
#endif

#define VOID_VALUE (~(size_t)0)

#ifdef _WIN64 /* 64-bit Windows including MSVC and MinGW-w64 */
#define FMT_LU "%llu"
typedef unsigned long long ulong_t;
/* NOTE: "%llu" and "long long" are not C89-compliant, but they are required to deal with a 64-bit integer value in 64-bit Windows. */
#else
#define FMT_LU "%lu"
typedef unsigned long ulong_t;
#endif
/* FMT_LU and ulong_t are used to print size_t values safely (ex. printf(FMT_LU "\n", (ulong_t)value);) */
/* NOTE: Neither "%z" nor <inttypes.h> is used since PackCC complies with the C89 standard as much as possible. */

typedef enum bool_tag {
    FALSE = 0,
    TRUE
} bool_t;


int print_error(const char *format, ...) __attribute__((format(printf, 1, 2)));

FILE *fopen_rb_e(const char *path);
FILE *fopen_wt_e(const char *path);
int fclose_e(FILE *stream);

int fgetc_e(FILE *stream);
int fputc_e(int c, FILE *stream);
int fputs_e(const char *s, FILE *stream);
int fprintf_e(FILE *stream, const char *format, ...) __attribute__((format(printf, 2, 3)));

void *malloc_e(size_t size);
void *realloc_e(void *ptr, size_t size);

char *strdup_e(const char *str);
char *strndup_e(const char *str, size_t len);


#endif