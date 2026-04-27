#include "baremetal/api/serial_hal.h"

#include <stdio.h>

static int putc_file(char c, FILE *f)
{
    (void)f;
    serial_putc(c);
    return 0;
}

static int getc_file(FILE *f)
{
    (void)f;
    return serial_getc();
}

static FILE __stdio = FDEV_SETUP_STREAM(putc_file, getc_file, NULL, _FDEV_SETUP_RW);

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x)
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdio
#endif

FILE *const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);
