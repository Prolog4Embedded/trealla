#include "memory.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double foo(double x, int64_t y)
{
    return pow(x, (double)y);
}

int bar(double x, int64_t y, double *result)
{
    *result = pow(x, (double)y);
    return 0;
}

char *baz(const char *x, const char *y)
{
    char *s = PL4BM_MALLOC_IMPL(strlen(x) + strlen(y) + 1);
    strcpy(s, x);
    strcat(s, y);
    return s;
}
