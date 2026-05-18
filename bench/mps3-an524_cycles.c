#include "program.h"
#include "trealla.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

char **g_envp = NULL;


void sigfn(int s) {
  printf("sigfn called: %d", s);
}

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018u)

#define SYST_CSR_ENABLE    (1u << 0)
#define SYST_CSR_CLKSOURCE (1u << 2)

#ifndef BENCH_WARMUP_ITERS
#define BENCH_WARMUP_ITERS 10
#endif

#ifndef BENCH_MEASURE_ITERS
#define BENCH_MEASURE_ITERS 100
#endif

#ifndef BENCH_QUERY
#define BENCH_QUERY "call(diagnose(Component, Fault, Action))."
#endif

#ifndef BENCH_EXPECTED_ANSWERS
#define BENCH_EXPECTED_ANSWERS 7
#endif

static void cycles_init(void)
{
    SYST_CSR = 0;
    SYST_RVR = 0x00FFFFFFu;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_CLKSOURCE;

    uint32_t before = SYST_CVR;

    for (volatile uint32_t i = 0; i < 100000; i++) {
        __asm volatile("nop");
    }

    uint32_t after = SYST_CVR;

    printf("SYST_CSR: 0x%08lx\n", (unsigned long)SYST_CSR);
    printf("systick_probe_ticks: %lu\n",
           (unsigned long)((before - after) & 0x00FFFFFFu));
}

static inline void cycles_reset(void)
{
    SYST_CVR = 0;
    __asm volatile("isb");
}

static inline uint32_t cycles_now(void)
{
    return SYST_CVR & 0x00FFFFFFu;
}

static inline uint32_t cycles_elapsed(uint32_t start, uint32_t end)
{
    return (start - end) & 0x00FFFFFFu;
}

static bool run_query(prolog *pl, const char *query, uint32_t expected_answers)
{
    uint32_t answers = 0;
    pl_sub_query *subq = NULL;

    if (pl_query(pl, query, &subq, 0)) {
        answers++;
    } else {
        return false;
    }

    while (pl_redo(subq)) {
        if (did_dump_vars(pl)) {
            answers++;
        }
    }

    return answers == expected_answers;
}

static prolog *init_prolog(void)
{
    prolog *pl = pl_create();

    if (!pl) {
        return NULL;
    }

    set_quiet(pl);

    if (!pl_consult_text(pl, (const char *)benchmark_program)) {
        pl_destroy(pl);
        return NULL;
    }

    return pl;
}

int main(void)
{
    cycles_init();

    prolog *pl = init_prolog();
    if (!pl) {
        printf("ERR init\n");
        return 1;
    }

    for (uint32_t i = 0; i < BENCH_WARMUP_ITERS; i++) {
        if (!run_query(pl, BENCH_QUERY, BENCH_EXPECTED_ANSWERS)) {
            printf("ERR warmup\n");
            pl_destroy(pl);
            return 1;
        }
    }

    uint32_t min_cycles = UINT32_MAX;
    uint32_t max_cycles = 0;
    uint64_t total_cycles = 0;

    for (uint32_t i = 0; i < BENCH_MEASURE_ITERS; i++) {
        cycles_reset();

        uint32_t start = cycles_now();

        bool ok = run_query(pl, BENCH_QUERY, BENCH_EXPECTED_ANSWERS);

        uint32_t end = cycles_now();

        if (!ok) {
            printf("ERR measure\n");
            pl_destroy(pl);
            return 1;
        }

        uint32_t cycles = cycles_elapsed(start, end);

        if (cycles < min_cycles) {
            min_cycles = cycles;
        }

        if (cycles > max_cycles) {
            max_cycles = cycles;
        }

        total_cycles += cycles;
    }

    uint64_t avg_cycles = total_cycles / BENCH_MEASURE_ITERS;

    printf("query: %s\n", BENCH_QUERY);
    printf("expected_answers: %u\n", (unsigned)BENCH_EXPECTED_ANSWERS);
    printf("warmup_iters: %u\n", (unsigned)BENCH_WARMUP_ITERS);
    printf("measure_iters: %u\n", (unsigned)BENCH_MEASURE_ITERS);
    printf("min_ticks: %lu\n", (unsigned long)min_cycles);
    printf("avg_ticks: %lu\n", (unsigned long)avg_cycles);
    printf("max_ticks: %lu\n", (unsigned long)max_cycles);

    pl_destroy(pl);
    return 0;
}
