#include "common.h"

const char *g_cmdname = "packcc"; /* replaced later with actual one */

int print_error(const char *format, ...) __attribute__((format(printf, 1, 2))) {
    int n;
    va_list a;
    va_start(a, format);
    n = fprintf(stderr, "%s: ", g_cmdname);
    if (n >= 0) {
        const int k = vfprintf(stderr, format, a);
        if (k < 0) n = k; else n += k;
    }
    va_end(a);
    return n;
}

FILE *fopen_rb_e(const char *path) {
    FILE *const f = fopen(path, "rb");
    if (f == NULL) {
        print_error("Cannot open file '%s' to read\n", path);
        exit(2);
    }
    return f;
}

FILE *fopen_wt_e(const char *path) {
    FILE *const f = fopen(path, "wt");
    if (f == NULL) {
        print_error("Cannot open file '%s' to write\n", path);
        exit(2);
    }
    return f;
}

int fclose_e(FILE *stream) {
    const int r = fclose(stream);
    if (r == EOF) {
        print_error("File closing error\n");
        exit(2);
    }
    return r;
}

int fgetc_e(FILE *stream) {
    const int c = fgetc(stream);
    if (c == EOF && ferror(stream)) {
        print_error("File read error\n");
        exit(2);
    }
    return c;
}

int fputc_e(int c, FILE *stream) {
    const int r = fputc(c, stream);
    if (r == EOF) {
        print_error("File write error\n");
        exit(2);
    }
    return r;
}

int fputs_e(const char *s, FILE *stream) {
    const int r = fputs(s, stream);
    if (r == EOF) {
        print_error("File write error\n");
        exit(2);
    }
    return r;
}

int fprintf_e(FILE *stream, const char *format, ...) __attribute__((format(printf, 2, 3))) {
    int n;
    va_list a;
    va_start(a, format);
    n = vfprintf(stream, format, a);
    va_end(a);
    if (n < 0) {
        print_error("File write error\n");
        exit(2);
    }
    return n;
}

void *malloc_e(size_t size) {
    void *const p = malloc(size);
    if (p == NULL) {
        print_error("Out of memory\n");
        exit(3);
    }
    return p;
}

void *realloc_e(void *ptr, size_t size) {
    void *const p = realloc(ptr, size);
    if (p == NULL) {
        print_error("Out of memory\n");
        exit(3);
    }
    return p;
}

char *strdup_e(const char *str) {
    const size_t m = strlen(str);
    char *const s = (char *)malloc_e(m + 1);
    memcpy(s, str, m);
    s[m] = '\0';
    return s;
}

char *strndup_e(const char *str, size_t len) {
    const size_t m = strnlen(str, len);
    char *const s = (char *)malloc_e(m + 1);
    memcpy(s, str, m);
    s[m] = '\0';
    return s;
}