#include "common.h"
#include "util.h"
#include "context.h"
#include "parse.h"
#include "generate.h"

extern const char *g_cmdname; /* replaced later with actual one */

void print_version(FILE *output) {
    fprintf(output, "%s version %s\n", g_cmdname, VERSION);
    fprintf(output, "Copyright (c) 2014, 2019-2021 Arihiro Yoshida. All rights reserved.\n");
}

void print_usage(FILE *output) {
    fprintf(output, "Usage: %s [OPTIONS] [FILE]\n", g_cmdname);
    fprintf(output, "Generates a packrat parser for C.\n");
    fprintf(output, "\n");
    fprintf(output, "  -o BASENAME    specify a base name of output source and header files\n");
    fprintf(output, "  -a, --ascii    disable UTF-8 support\n");
    fprintf(output, "  -d, --debug    with debug information\n");
    fprintf(output, "  -h, --help     print this help message and exit\n");
    fprintf(output, "  -v, --version  print the version and exit\n");
}

int main(int argc, char **argv) {
    const char *iname = NULL;
    const char *oname = NULL;
    bool_t ascii = FALSE;
    bool_t debug = FALSE;
#ifdef _MSC_VER
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif
#endif
    g_cmdname = extract_filename(argv[0]);
    {
        const char *fname = NULL;
        const char *opt_o = NULL;
        bool_t opt_a = FALSE;
        bool_t opt_d = FALSE;
        bool_t opt_h = FALSE;
        bool_t opt_v = FALSE;
        int i;
        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                break;
            }
            else if (strcmp(argv[i], "--") == 0) {
                i++; break;
            }
            else if (argv[i][1] == 'o') {
                const char *const o = (argv[i][2] != '\0') ? argv[i] + 2 : (++i < argc) ?  argv[i] : NULL;
                if (o == NULL) {
                    print_error("Output base name missing\n");
                    fprintf(stderr, "\n");
                    print_usage(stderr);
                    exit(1);
                }
                if (opt_o != NULL) {
                    print_error("Extra output base name '%s'\n", o);
                    fprintf(stderr, "\n");
                    print_usage(stderr);
                    exit(1);
                }
                opt_o = o;
            }
            else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--ascii") == 0) {
                opt_a = TRUE;
            }
            else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
                opt_d = TRUE;
            }
            else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                opt_h = TRUE;
            }
            else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                opt_v = TRUE;
            }
            else {
                print_error("Invalid option '%s'\n", argv[i]);
                fprintf(stderr, "\n");
                print_usage(stderr);
                exit(1);
            }
        }
        switch (argc - i) {
        case 0:
            break;
        case 1:
            fname = argv[i];
            break;
        default:
            print_error("Multiple input files\n");
            fprintf(stderr, "\n");
            print_usage(stderr);
            exit(1);
        }
        if (opt_h || opt_v) {
            if (opt_v) print_version(stdout);
            if (opt_v && opt_h) fprintf(stdout, "\n");
            if (opt_h) print_usage(stdout);
            exit(0);
        }
        iname = (fname != NULL && fname[0] != '\0') ? fname : NULL;
        oname = (opt_o != NULL && opt_o[0] != '\0') ? opt_o : NULL;
        ascii = opt_a;
        debug = opt_d;
    }
    {
        context_t *const ctx = create_context(iname, oname, ascii, debug);
        const int b = parse(ctx) && generate(ctx);
        destroy_context(ctx);
        if (!b) exit(10);
    }
    return 0;
}
