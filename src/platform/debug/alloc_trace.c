#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PL4BM_ALLOC_TRACK_MAX 2048

typedef struct {
    void *ptr;
    size_t size;
} alloc_entry_t;

static alloc_entry_t g_allocs[PL4BM_ALLOC_TRACK_MAX];

static unsigned g_alloc_trace_depth;

static size_t g_alloc_current;
static size_t g_alloc_peak;
static size_t g_alloc_total;
static size_t g_free_total;

void *__real_malloc(size_t size);
void __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);
void *__real_calloc(size_t nmemb, size_t size);

static void print_size(size_t bytes)
{
    if (bytes >= 1024) {
        printf("%lu.%02lu KiB", (unsigned long)(bytes / 1024),
               (unsigned long)(((bytes % 1024) * 100) / 1024));
    } else {
        printf("%lu B", (unsigned long)bytes);
    }
}

static int track_add(void *ptr, size_t size)
{
    if (!ptr) {
        return 0;
    }

    for (size_t i = 0; i < PL4BM_ALLOC_TRACK_MAX; ++i) {
        if (!g_allocs[i].ptr) {
            g_allocs[i].ptr = ptr;
            g_allocs[i].size = size;

            g_alloc_current += size;
            g_alloc_total += size;

            if (g_alloc_current > g_alloc_peak) {
                g_alloc_peak = g_alloc_current;
            }

            return 1;
        }
    }

    return 0;
}

static size_t track_remove(void *ptr)
{
    if (!ptr) {
        return 0;
    }

    for (size_t i = 0; i < PL4BM_ALLOC_TRACK_MAX; ++i) {
        if (g_allocs[i].ptr == ptr) {
            size_t size = g_allocs[i].size;

            g_allocs[i].ptr = NULL;
            g_allocs[i].size = 0;

            g_alloc_current -= size;
            g_free_total += size;

            return size;
        }
    }

    return 0;
}

static void log_stats(void)
{
    printf(" current=");
    print_size(g_alloc_current);

    printf(" peak=");
    print_size(g_alloc_peak);

    printf(" total=");
    print_size(g_alloc_total);

    printf("\n");
}

void *__wrap_malloc(size_t size)
{
    if (g_alloc_trace_depth > 0) {
        return __real_malloc(size);
    }

    g_alloc_trace_depth++;

    void *caller = __builtin_extract_return_addr(__builtin_return_address(0));
    void *p = __real_malloc(size);

    int tracked = track_add(p, size);

    printf("[PL4BM MEMORY] malloc(%lu)\t = %p caller=%p%s",
           (unsigned long)size,
           p,
           caller,
           tracked ? "" : " TRACK_FULL");

    log_stats();

    g_alloc_trace_depth--;

    return p;
}

void __wrap_free(void *ptr)
{
    void *caller = __builtin_extract_return_addr(__builtin_return_address(0));

    size_t old_size = track_remove(ptr);

    printf("[PL4BM MEMORY] free(%p", ptr);

    if (old_size) {
        printf(", ");
        print_size(old_size);
    } else if (ptr) {
        printf(", UNKNOWN");
    }

    printf(")\t caller=%p", caller);

    __real_free(ptr);

    log_stats();
}

void *__wrap_calloc(size_t nmemb, size_t size)
{
    if (g_alloc_trace_depth > 0) {
        return __real_calloc(nmemb, size);
    }

    g_alloc_trace_depth++;

    void *caller = __builtin_extract_return_addr(__builtin_return_address(0));

    void *p = __real_calloc(nmemb, size);

    size_t total = nmemb * size;
    int tracked = track_add(p, total);

    printf("[PL4BM MEMORY] calloc(%lu, %lu)\t = %p caller=%p%s",
           (unsigned long)nmemb,
           (unsigned long)size,
           p,
           caller,
           tracked ? "" : " TRACK_FULL");

    log_stats();

    g_alloc_trace_depth--;

    return p;
}

void *__wrap_realloc(void *ptr, size_t size)
{
    void *caller = __builtin_extract_return_addr(__builtin_return_address(0));

    size_t old_size = track_remove(ptr);
    void *new_ptr = __real_realloc(ptr, size);
    int tracked = track_add(new_ptr, size);

    printf("[PL4BM] realloc(%p", ptr);

    if (old_size) {
        printf(", old=");
        print_size(old_size);
    } else if (ptr) {
        printf(", old=UNKNOWN");
    }

    printf(", new=%lu)\t = %p caller=%p%s", (unsigned long)size, new_ptr, caller,
           tracked ? "" : " TRACK_FULL");

    log_stats();

    return new_ptr;
}
