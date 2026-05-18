#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <time.h>

#include "builtins.h"
#include "program.h"
#include "trealla.h"

#if defined(BENCH_MODE_CYCLES)
#include <x86intrin.h>
#endif

#ifndef BENCH_WARMUP_ITERS
#define BENCH_WARMUP_ITERS 100
#endif

#ifndef BENCH_MEASURE_ITERS
#define BENCH_MEASURE_ITERS 1000
#endif

char **g_envp = NULL;

#if defined(BENCH_MODE_CYCLES)
typedef struct {
    const char *name;
    uint8_t expected_answers;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
} bench_result_t;
#endif

void sigfn(int s)
{
    g_tpl_interrupt = s;
    signal(SIGINT, &sigfn);
}

static prolog *init_prolog(void);
static bool run_query(prolog *pl, const char *query, uint8_t expected_answers);

#if defined(BENCH_MODE_CYCLES)
static int run_cycles_benchmark(void);
static bool warmup_all_queries(prolog *pl);
static bool measure_query_cycles(prolog *pl, const tpl_bench_case_t *bench, bench_result_t *result);
static void print_cycle_summary(const bench_result_t *results, size_t len);
static inline uint64_t rdtsc_begin(void);
static inline uint64_t rdtsc_end(void);
#endif

#if defined(BENCH_MODE_RUNTIME) || defined(BENCH_MODE_MEMORY)
static int run_minimal_benchmark(void);
#endif

int main(void)
{
    srand((unsigned)time(NULL));
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");

#if defined(BENCH_MODE_CYCLES)
    return run_cycles_benchmark();
#elif defined(BENCH_MODE_RUNTIME) || defined(BENCH_MODE_MEMORY)
    return run_minimal_benchmark();
#else
#error "Define BENCH_MODE_CYCLES, BENCH_MODE_RUNTIME, or BENCH_MODE_MEMORY"
#endif
}

static prolog *init_prolog(void)
{

    prolog *pl = pl_create();

    if (!pl) {
        fprintf(stderr, "Failed to create prolog: %s\n", strerror(errno));
        return NULL;
    }

    set_quiet(pl);

    if (!pl_consult_text(pl, (const char *)benchmark_program)) {
        fprintf(stderr, "Failed to load embedded prolog program.\n");
        pl_destroy(pl);
        return NULL;
    }

    return pl;
}

static bool run_query(prolog *pl, const char *query, uint8_t expected_answers)
{
    uint8_t answers = 0;
    pl_sub_query *subq = NULL;

    if (pl_query(pl, query, &subq, 0)) {
        answers++;
    } else {
        fprintf(stderr, "query failed or no answers: %s\n", query);
        return false;
    }

    while (pl_redo(subq)) {
        if (did_dump_vars(pl)) {
            if (answers == UINT8_MAX) {
                fprintf(stderr, "Received too many answers for query: %s\n", query);
                return false;
            }

            answers++;
        }
    }

    if (answers != expected_answers) {
        fprintf(stderr, "answer mismatch for query: %s\nexpected: %u\nreceived: %u\n", query,
                expected_answers, answers);
        return false;
    }

    return true;
}

#if defined(BENCH_MODE_CYCLES)

static int run_cycles_benchmark(void)
{
    prolog *pl = init_prolog();
    if (!pl)
        return EXIT_FAILURE;

    if (!warmup_all_queries(pl)) {
        pl_destroy(pl);
        return EXIT_FAILURE;
    }

    bench_result_t results[bench_cases_len];

    for (uint8_t i = 0; i < bench_cases_len; i++) {
        if (!measure_query_cycles(pl, &bench_cases[i], &results[i])) {
            pl_destroy(pl);
            return EXIT_FAILURE;
        }
    }

    pl_destroy(pl);

    print_cycle_summary(results, bench_cases_len);
    return EXIT_SUCCESS;
}

static bool warmup_all_queries(prolog *pl)
{
    for (uint8_t i = 0; i < bench_cases_len; i++) {
        for (uint64_t j = 0; j < BENCH_WARMUP_ITERS; j++) {
            if (!run_query(pl, bench_cases[i].query, bench_cases[i].expected_answers))
                return false;
        }
    }

    return true;
}

static bool measure_query_cycles(prolog *pl, const tpl_bench_case_t *bench, bench_result_t *result)
{
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    uint64_t total_cycles = 0;

    for (uint64_t i = 0; i < BENCH_MEASURE_ITERS; i++) {
        uint64_t start = rdtsc_begin();

        bool ok = run_query(pl, bench->query, bench->expected_answers);

        uint64_t end = rdtsc_end();

        if (!ok)
            return false;

        uint64_t cycles = end - start;

        if (cycles < min_cycles)
            min_cycles = cycles;

        if (cycles > max_cycles)
            max_cycles = cycles;

        total_cycles += cycles;
    }

    *result = (bench_result_t){
        .name = bench->name,
        .expected_answers = bench->expected_answers,
        .min_cycles = min_cycles,
        .max_cycles = max_cycles,
        .total_cycles = total_cycles,
    };

    return true;
}

static void print_cycle_summary(const bench_result_t *results, size_t len)
{
    fprintf(stderr, "\nBenchmark summary\n");
    fprintf(stderr, "=================\n");
    fprintf(stderr, "warmup iterations:  %" PRIu64 "\n", (uint64_t)BENCH_WARMUP_ITERS);
    fprintf(stderr, "measure iterations: %" PRIu64 "\n\n", (uint64_t)BENCH_MEASURE_ITERS);

    fprintf(stderr, "%-24s %12s %12s %12s %12s\n", "name", "answers", "min", "avg", "max");

    for (size_t i = 0; i < len; i++) {
        uint64_t avg = results[i].total_cycles / BENCH_MEASURE_ITERS;

        fprintf(stderr, "%-24s %12u %12" PRIu64 " %12" PRIu64 " %12" PRIu64 "\n", results[i].name,
                results[i].expected_answers, results[i].min_cycles, avg, results[i].max_cycles);
    }
}

static inline uint64_t rdtsc_begin(void)
{
    unsigned aux;
    _mm_lfence();
    return __rdtscp(&aux);
}

static inline uint64_t rdtsc_end(void)
{
    unsigned aux;
    uint64_t t = __rdtscp(&aux);
    _mm_lfence();
    return t;
}

#endif

#if defined(BENCH_MODE_RUNTIME) || defined(BENCH_MODE_MEMORY)

static int run_minimal_benchmark(void)
{
    prolog *pl = init_prolog();
    if (!pl)
        return EXIT_FAILURE;

    for (uint8_t i = 0; i < bench_cases_len; i++) {
        if (!run_query(pl, bench_cases[i].query, bench_cases[i].expected_answers)) {
            pl_destroy(pl);
            return EXIT_FAILURE;
        }
    }

    pl_destroy(pl);
    return EXIT_SUCCESS;
}

#endif
