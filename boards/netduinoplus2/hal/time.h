#include <stdint.h>

#include "baremetal/hal_utils.h"

#define TIM2_INPUT_HZ UINT32_C(16000000)
#define TIM2_TICK_HZ UINT32_C(1000000)

#define RCC_BASE UINT32_C(0x40023800)
#define RCC_APB1ENR   (*(volatile uint32_t *)(RCC_BASE + UINT32_C(0x40)))

#define RCC_APB1ENR_TIM2EN BIT32(0)

#define TIM2_BASE UINT32_C(0x40000000)

// p. 630ff
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SMCR;  /* 0x08 */
    volatile uint32_t DIER;  /* 0x0C */
    volatile uint32_t SR;    /* 0x10 */
    volatile uint32_t EGR;   /* 0x14 */
    volatile uint32_t CCMR1; /* 0x18 */
    volatile uint32_t CCMR2; /* 0x1C */
    volatile uint32_t CCER;  /* 0x20 */
    volatile uint32_t CNT;   /* 0x24 */
    volatile uint32_t PSC;   /* 0x28 */
    volatile uint32_t ARR;   /* 0x2C */
} tim_t;

#define TIM2 ((tim_t *)TIM2_BASE)
#define TIM_CR1_CEN BIT32(0)
#define TIM_EGR_UG BIT32(0)

void time_init();
