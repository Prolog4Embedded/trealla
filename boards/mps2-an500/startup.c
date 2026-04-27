#include <stdint.h>
#include <stdio.h>

extern uint8_t __stack[]; // linker symbol
extern void _start();     // picolibc crt0

static void reset_handler();
static void default_handler();
static void frame_dumping_handler();

static inline uint32_t interrupt_number() {
  uint32_t ispr;
  __asm__ volatile("mrs %0, ipsr" : "=r"(ispr));
  return ispr;
}

__attribute__((used, section(".text.init.enter")))
const void *const __interrupt_vector[] = {
    [0] = (void *)&__stack,
    [1] = (void *)reset_handler,
    [2] = (void *)default_handler, /* NMI */
    [3] = (void *)frame_dumping_handler, /* HardFault */
    [4] = (void *)default_handler, /* MemManage */
    [5] = (void *)default_handler, /* BusFault */
    [6] = (void *)default_handler, /* UsageFault */
    [7] = 0,
    [8] = 0,
    [9] = 0,
    [10] = 0,
    [11] = (void *)default_handler, /* SVCall */
    [12] = (void *)default_handler, /* DebugMonitor */
    [13] = 0,
    [14] = (void *)default_handler, /* PendSV */
    [15] = (void *)default_handler, /* SysTick */

    [16 + 0 ... 16 + 31] = (void *)default_handler,
};

static const char *const interrupt_names[] = {
    [2] = "NMI",           [3] = "HardFault",  [4] = "MemManage",
    [5] = "BusFault",      [6] = "UsageFault", [11] = "SVCall",
    [12] = "DebugMonitor", [14] = "PendSV",    [15] = "SysTick",
};

static void reset_handler() {
  _start();
  printf("RETURNED FROM _start\n");
  for (;;) {
    __asm__ volatile("bkpt #0");
    __asm__ volatile("wfi");
  }
}

static void default_handler(void) {
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
