#include <stdint.h>
#include <stdio.h>

extern uint8_t __stack[]; // linker symbol
extern void _start();     // picolibc crt0

static void reset_handler();
static void default_handler();

static inline uint32_t interrupt_number()
{
    uint32_t ispr;
    __asm__ volatile("mrs %0, ipsr" : "=r"(ispr));
    return ispr;
}

/**
 * 82 maskable interrupt channels
 * p. 374 https://www.st.com/resource/en/reference_manual/DM00031020.pdf
 * p. 375 for table
 */
__attribute__((used, section(".text.init.enter"))) const void *const __interrupt_vector[] = {
    // Unmaskable Interrupts
    (void *)&__stack,        // -, stack pointer
    (void *)reset_handler,   // -, Reset
    (void *)default_handler, // -, NMI
    (void *)default_handler, // -, HardFault
    (void *)default_handler, // -, MemManage
    (void *)default_handler, // -, BusFault
    (void *)default_handler, // -, UsageFault
    0,                       // -, Reserved
    (void *)default_handler, // -, SVCall
    (void *)default_handler, // -, Debug Monitor
    0,                       // -, Reserved
    (void *)default_handler, // -, System Tick Timer
    // Maskable Interrupts
    [12 + 0 ... 12 + 81] = (void *)default_handler,
};

static const char *const interrupt_names[] = {
    [2] = "NMI",
	[3] = "HardFault",
	[4] = "MemManage",
	[5] = "BusFault",
	[6] = "UsageFault",
	[7] = "WTF? Reserved",
	[8] = "SVCall",
	[9] = "Debug Monitor",
	[10] = "WTF?? Reserver",
	[11] = "System Tick Timer",
};

static void reset_handler()
{
    _start();
    printf("RETURNED FROM _start\n");
    for (;;) {
        __asm__ volatile("bkpt #0");
    }
}

static void default_handler()
{
    uint32_t interrupt = interrupt_number();

    const char *name = NULL;
    if (interrupt < sizeof(interrupt_names) / sizeof(interrupt_names[0])) {
        name = interrupt_names[interrupt];
    }

    printf("Unhandled interrupt: %u", interrupt);

    if (interrupt >= 16) {
        printf(" IRQ=%u", interrupt - 16);
    }

    if (name) {
        printf(" %s", name);
    }

    printf("\n");

    for (;;) {
        __asm__ volatile("bkpt #0");
    }
}
