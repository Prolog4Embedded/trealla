#include <stdint.h>

#include "baremetal/hal_utils.h"

#define TIMER1_BASE UINT32_C(0x40001000)

typedef struct {
	volatile uint32_t VALUE;
	volatile uint32_t RELOAD;
	volatile uint32_t CTRL;
	volatile uint32_t INTSTATUS;
} cmsdk_timer_t;

#define TIMER1 ((cmsdk_timer_t *)TIMER1_BASE)

#define TIMER_CTRL_ENABLE BIT32(0)
#define TIMER_CTRL_INTEN  BIT32(3)

void time_init();
