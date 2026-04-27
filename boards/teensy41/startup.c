
#include <stdint.h>

extern char  __stack[]; // linker symbol
extern void  _start(void); // picolibc crt0-minimal entry

static void reset_handler(void);
static void default_handler(void) { for (;;); }

__attribute__((used, section(".text.init.enter")))
const void *const __interrupt_vector[] = {
    // Core exceptions
    (void *)&__stack,         // 0  Initial SP
    (void *)reset_handler,    // 1  Reset
    (void *)default_handler,  // 2  NMI
    (void *)default_handler,  // 3  HardFault
    (void *)default_handler,  // 4  MemManage
    (void *)default_handler,  // 5  BusFault
    (void *)default_handler,  // 6  UsageFault
    0,                        // 7  Reserved
    0,                        // 8  Reserved
    0,                        // 9  Reserved
    0,                        // 10 Reserved
    (void *)default_handler,  // 11 SVCall
    (void *)default_handler,  // 12 DebugMon
    0,                        // 13 Reserved
    (void *)default_handler,  // 14 PendSV
    (void *)default_handler,  // 15 SysTick

	// ---
    [16 ... 175] = (void *)default_handler,
};

// WDOG1 (legacy 16-bit watchdog)
#define WDOG1_WCR (*(volatile uint16_t *)0x400B8000u)

// RTWDOG (32-bit real-time watchdog)
#define RTWDOG_CS    (*(volatile uint32_t *)0x400BC000u)
#define RTWDOG_CNT   (*(volatile uint32_t *)0x400BC004u)
#define RTWDOG_TOVAL (*(volatile uint32_t *)0x400BC008u)
#define RTWDOG_CS_EN     (1u << 7)
#define RTWDOG_CS_UPDATE (1u << 9)

static void watchdog_disable(void)
{
    // Disable WDOG1 by clearing WDE (bit 2)
    WDOG1_WCR &= ~(uint16_t)(1u << 2);

    // Disable RTWDOG if the UPDATE bit is set (reconfiguration allowed)
    if (RTWDOG_CS & RTWDOG_CS_UPDATE) {
        RTWDOG_CNT   = 0xD928C520u; // unlock
        RTWDOG_TOVAL = 0xFFFFu;
        RTWDOG_CS    = RTWDOG_CS_UPDATE; // EN=0, UPDATE=1 (keep update enabled)
    }
}

static void reset_handler(void)
{
    watchdog_disable();
    _start();
    for (;;);
}
