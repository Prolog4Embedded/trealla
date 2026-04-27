#include "time.h"
#include "time_hal.h"

#include <stdint.h>

static uint32_t s_last_cnt;
static uint64_t s_high_us;

void time_init()
{
	TIMER1->RELOAD = UINT32_MAX;
	TIMER1->CTRL = TIMER_CTRL_ENABLE;
}

uint64_t pl4bm_monotonic_ns(void)
{
	// descending by default
    uint32_t now = UINT32_MAX - TIMER1->VALUE;

    if (now < s_last_cnt) {
        s_high_us += UINT64_C(1) << 32;
    }

    s_last_cnt = now;
	// ~40ns per 25MHz tick
    uint64_t us = (s_high_us + now) * 40;
    return us * UINT64_C(1000);
}
