#include "generic.h"
#include "time_hal.h"

#include <stdint.h>

// Example implementation of pl4bm_monotonic_ns() using a memory-mapped
// free-running counter. Adapt GENERIC_COUNTER and GENERIC_COUNTER_HZ in
// generic.h to match your hardware.

static uint64_t read_counter(void)
{
    // Read hi/lo twice to detect a carry between the two 32-bit halves.
    uint32_t hi, lo, hi2;
    do {
        hi = GENERIC_COUNTER->VALUE_HI;
        lo = GENERIC_COUNTER->VALUE_LO;
        hi2 = GENERIC_COUNTER->VALUE_HI;
    } while (hi != hi2);

    return ((uint64_t)hi << 32) | lo;
}

uint64_t pl4bm_monotonic_ns(void)
{
    uint64_t ticks = read_counter();
    uint64_t secs = ticks / GENERIC_COUNTER_HZ;
    uint64_t sub = ticks % GENERIC_COUNTER_HZ;

    return secs * 1000000000ULL + sub * 1000000000ULL / GENERIC_COUNTER_HZ;
}
