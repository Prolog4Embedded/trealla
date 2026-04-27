#pragma once

#include <stdint.h>

/**
 * Implemented per-board. Returns some increasing number.
 */
uint64_t pl4bm_monotonic_ns(void);
