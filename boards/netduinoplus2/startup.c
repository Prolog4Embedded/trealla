#include <stdint.h>
#include <stdio.h>

extern uint8_t __stack[]; // linker symbol
extern void _start();     // picolibc crt0

static void reset_handler();
static void default_handler();
static void frame_dumping_handler(void);

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
    (void *)frame_dumping_handler, // -, HardFault
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
        __asm__ volatile("wfi");
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
        __asm__ volatile("wfi");
    }
}

#define SCB_CFSR  (*(volatile uint32_t *)0xE000ED28)
#define SCB_HFSR  (*(volatile uint32_t *)0xE000ED2C)
#define SCB_MMFAR (*(volatile uint32_t *)0xE000ED34)
#define SCB_BFAR  (*(volatile uint32_t *)0xE000ED38)

void fault_handler_c(uint32_t *frame) {
  uint32_t interrupt = interrupt_number();

  printf("Unhandled interrupt: %lu", (unsigned long)interrupt);

  if (interrupt >= 16) {
    printf(" IRQ=%lu", (unsigned long)(interrupt - 16));
  }

  if (interrupt < sizeof(interrupt_names) / sizeof(interrupt_names[0]) &&
      interrupt_names[interrupt]) {
    printf(" %s", interrupt_names[interrupt]);
  }

  printf("\n");

  printf("r0   = 0x%08lx\n", (unsigned long)frame[0]);
  printf("r1   = 0x%08lx\n", (unsigned long)frame[1]);
  printf("r2   = 0x%08lx\n", (unsigned long)frame[2]);
  printf("r3   = 0x%08lx\n", (unsigned long)frame[3]);
  printf("r12  = 0x%08lx\n", (unsigned long)frame[4]);
  printf("lr   = 0x%08lx\n", (unsigned long)frame[5]);
  printf("pc   = 0x%08lx\n", (unsigned long)frame[6]);
  printf("xpsr = 0x%08lx\n", (unsigned long)frame[7]);

  printf("CFSR = 0x%08lx\n", (unsigned long)SCB_CFSR);
  printf("HFSR = 0x%08lx\n", (unsigned long)SCB_HFSR);
  printf("MMFAR= 0x%08lx\n", (unsigned long)SCB_MMFAR);
  printf("BFAR = 0x%08lx\n", (unsigned long)SCB_BFAR);

  for (;;) {
    __asm__ volatile("wfi");
  }
}

__attribute__((naked))
static void frame_dumping_handler(void) {
  __asm__ volatile(
      "tst lr, #4        \n"
      "ite eq            \n"
      "mrseq r0, msp     \n"
      "mrsne r0, psp     \n"
      "b fault_handler_c \n"
  );
}
