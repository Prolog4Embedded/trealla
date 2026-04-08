#include "panic.h"

#include <stdlib.h>
#include <stdio.h>

void panic(const char *msg) {
    printf("[FATAL] %s", msg);

    exit(1);
}

void not_implemented(const char *name) {
    char buf[128];
    snprintf(buf, sizeof(buf), "An empty function named \"%s\" was called", name);
    panic(buf);
}
