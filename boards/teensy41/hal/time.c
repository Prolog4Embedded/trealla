#include "time.h"
#include "time_hal.h"

#include <stdint.h>

static uint32_t s_last;
static uint64_t s_high_us;

void time_init(void)
{
    CCM_CCGR1 |= CCM_CCGR1_GPT1;

    GPT1->CR = 0;
    GPT1->PR = 23; // 24 MHz / 24 = 1 MHz (1 us per tick)
    GPT1->CR = GPT_CR_EN_24M | GPT_CR_CLKSRC_24M | GPT_CR_FRR | GPT_CR_ENMOD | GPT_CR_EN;

    s_last    = 0;
    s_high_us = 0;
}

uint64_t pl4bm_monotonic_ns(void)
{
    uint32_t now = GPT1->CNT;

    if (now < s_last) {
        s_high_us += UINT64_C(1) << 32;
    }

    s_last = now;
    uint64_t us = s_high_us + now;
    return us * UINT64_C(1000);
}
