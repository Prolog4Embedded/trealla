#include "time.h"
#include "time_hal.h"

#include <stdint.h>

static uint32_t s_last_tim2_cnt;
static uint64_t s_high_us;

void time_init()
{
    // enable tim2 on peripheral clock APB1
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0;

    TIM2->PSC = (TIM2_INPUT_HZ / TIM2_TICK_HZ) - UINT32_C(1);

    TIM2->ARR = UINT32_MAX;

    TIM2->CNT = 0;

    TIM2->EGR = TIM_EGR_UG;

    s_last_tim2_cnt = 0;
    s_high_us = 0;

    TIM2->CR1 = TIM_CR1_CEN;
}

uint64_t pl4bm_monotonic_ns(void)
{
    uint32_t now = TIM2->CNT;

    if (now < s_last_tim2_cnt) {
        s_high_us += UINT64_C(1) << 32;
    }

    s_last_tim2_cnt = now;
    uint64_t us = s_high_us + now;
    return us * UINT64_C(1000);
}
