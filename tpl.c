#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "builtins.h"
#include "src/trealla.h"
#include "trealla.h"

#ifdef TPL_HAS_PROGRAM_PL
extern unsigned char program_pl[];
extern unsigned int program_pl_len;
#endif

void sigfn(int s)
{
    g_tpl_interrupt = s;
    printf("sigfn called: %d", s);
    signal(SIGINT, &sigfn);
}

char **g_envp = NULL;

void fucked_up_repl(prolog *pl, char *line, size_t line_size);

int main(void)
{
    printf("Trealla Prolog\n");
    srand((unsigned)time(NULL));
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");

    prolog *pl = pl_create();

    if (!pl) {
        fprintf(stderr, "Failed to create prolog: %s\n", strerror(errno));
        return 1;
    }

    set_autofail(pl);

#ifdef TPL_HAS_PROGRAM_PL
    if (!pl_consult_text(pl, (const char *)program_pl)) {
        fprintf(stderr, "Failed to load embedded program\n");
        pl_destroy(pl);
        return 1;
    }

    printf("Program loaded.\n");
#else
    fprintf(stderr,
            "[FATAL] No prolog program was embedded, dynamic loading is not currently supported!");
    exit(EXIT_FAILURE);
#endif

    char line[256];

    fucked_up_repl(pl, line, sizeof(line));

    int halt_code = get_halt_code(pl);
    pl_destroy(pl);

    return halt_code;
}

void fucked_up_repl(prolog *pl, char *line, size_t line_size)
{
    for (;;) {
        printf("?- ");
        fflush(stdout);

        size_t len = 0;

        for (;;) {
            int c = getchar();

            if (c == EOF) {
                printf("\n");
                return;
            }

            if (c == '\r')
                continue;

            if (c == '\n') {
                putchar('\n');
                fflush(stdout);
                break;
            }

            if (c == '\b' || c == 127) {
                if (len > 0) {
                    len--;
                    printf("\b \b");
                    fflush(stdout);
                }
                continue;
            }

            if (len + 1 < line_size) {
                line[len++] = (char)c;
                putchar(c);
                fflush(stdout);

                if (c == '.') {
                    printf("\nAutomatically continuing on '.'...\n");
                    break;
                }
            }
        }

        line[len] = '\0';

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (len == 0)
            continue;

        if (line[len - 1] != '.') {
            if (len + 1 < line_size) {
                line[len++] = '.';
                line[len] = '\0';
            } else {
                fprintf(stderr, "Input too long\n");
                continue;
            }
        }

        pl_eval(pl, line, true);

        if (get_halt(pl))
            break;

        if (!get_status(pl))
            printf("false.\n");
        else if (!did_dump_vars(pl))
            printf("true.\n");
    }
}
