#pragma once

#include <stdint.h>

// Board-specific memory-mapped peripheral base addresses.
// Replace these with the real addresses from your MCU's reference manual.

// Example: a simple 64-bit free-running counter peripheral.
// Access it as: GENERIC_COUNTER->VALUE
#define GENERIC_COUNTER_BASE ((uintptr_t)0x40000000)

typedef struct {
    volatile uint32_t VALUE_LO; // lower 32 bits of counter
    volatile uint32_t VALUE_HI; // upper 32 bits of counter
} generic_counter_t;

#define GENERIC_COUNTER ((generic_counter_t *)GENERIC_COUNTER_BASE)

// Tick frequency of the counter in Hz.
// Replace with your board's actual counter frequency.
#define GENERIC_COUNTER_HZ 1000000ULL
