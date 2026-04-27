You are an AI assistant with access to a complete “context file” containing files in this project.  Each file is included in its entirety, separated by headers of the form:

```
-------- ./path/to/file.ext --------
<file contents>
```

**Guidelines for using the context file:**

1. **Always consult the context first.**

   * Before you generate any solution, recommendation, or code snippet, search the context for relevant definitions, functions, types, and comments.
   * If you find the answer already implemented, quote or reference the exact file and section (using the header marker) rather than reinventing it.

2. **Maintain consistency with existing code.**

   * Match the project’s coding style, naming conventions, and error‐handling patterns.
   * When extending or modifying code, ensure your changes slot cleanly into the existing files.

3. **Handle missing information gracefully.**

   * If the context does not contain the needed logic or data, clearly state that you did not find it in the provided files.
   * Offer to propose a new implementation or request additional context.

4. **Be precise and concise.**

   * Provide minimal explanations that directly address the user’s request.
   * Include exact file paths and line comments when referring back to context snippets.

Whenever you receive a question or task, begin by locating the relevant section in the context file. Only after confirming whether the solution exists should you write or modify code.
-------- boards/mps2-an500/startup.c --------
```c
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
 * 32 external interrupt channels
 * Table 5-1, DAI 0500B (ARM Cortex-M7 SMM on V2M-MPS2+)
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
    [12 + 0 ... 12 + 31] = (void *)default_handler,
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
```
-------- boards/mps2-an500/picolibc-cross.txt --------
```txt
[binaries]
c = 'arm-none-eabi-gcc'
ar = 'arm-none-eabi-ar'
as = 'arm-none-eabi-gcc'
strip = 'arm-none-eabi-strip'
objcopy = 'arm-none-eabi-objcopy'
size = 'arm-none-eabi-size'

[host_machine]
system = 'none'
cpu_family = 'arm'
cpu = 'cortex-m7'
endian = 'little'

[properties]
needs_exe_wrapper = true

[built-in options]
c_args = ['-mcpu=cortex-m7', '-mthumb']
c_link_args = ['-mcpu=cortex-m7', '-mthumb']
```
-------- boards/mps2-an500/CMakePresets.json --------
```json
{
    "version": 7,
    "include": [
        "../../cmake/presets/base.json"
    ],
    "configurePresets": [
        {
            "name": "mps2-an500-picolibc-base",
            "hidden": true,
            "inherits": "base",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE":
                    "${sourceDir}/cmake/toolchains/arm-none-eabi-picolibc.cmake",
                "TPL_BAREMETAL_CPU": "cortex-m7",
                "TPL_BOARD_CMAKE": "${sourceDir}/boards/mps2-an500/board.cmake"
            }
        },
        {
            "name": "mps2-an500-picolibc-debug",
            "displayName": "Netduino Plus 2 picolibc Debug",
            "inherits": [
                "mps2-an500-picolibc-base",
                "base-debug"
            ]
        },
        {
            "name": "mps2-an500-picolibc-release",
            "displayName": "Netduino Plus 2 picolibc Release",
            "inherits": [
                "mps2-an500-picolibc-base",
                "base-release"
            ]
        }
    ],
    "buildPresets": [
        {
            "name": "mps2-an500-picolibc-debug",
            "configurePreset": "mps2-an500-picolibc-debug"
        },
        {
            "name": "mps2-an500-picolibc-release",
            "configurePreset": "mps2-an500-picolibc-release"
        }
    ]
}
```
-------- boards/mps2-an500/hal/io/uart.c --------
```c
#include "uart.h"
#include <stdio.h>

void uart_init()
{
	UART0->CTRL = 0;
	UART0->BAUDDIV = 217;
	UART0->CTRL = UART_CTRL_RXEN | UART_CTRL_TXEN;

	printf("UART ready\n");
}

void serial_putc(char c)
{
    while ((UART0->STATE & UART_STATE_TXBF))
        ;
    UART0->DATA = (uint32_t)c;
}

int serial_getc(void)
{
    while (!(UART0->STATE & UART_STATE_RXBF))
        ;
    return (int)(UART0->DATA & 0xFF);
}
```
-------- boards/mps2-an500/hal/io/uart.h --------
```h
#pragma once

#include <stdint.h>

#include "baremetal/hal_utils.h"

// AI0500B_cortex_m7_on_v2m_mps2.pdf

typedef struct {
	volatile uint32_t DATA;
	volatile uint32_t STATE;
	volatile uint32_t CTRL;
	volatile uint32_t INTSTATUS;
	volatile uint32_t BAUDDIV;
} cmsdk_uart_t;

#define UART0_BASE UINT32_C(0x40004000)
#define UART0 ((cmsdk_uart_t *)UART0_BASE)

#define UART_STATE_TXBF BIT32(0)  /* TX buffer full */
#define UART_STATE_RXBF BIT32(1)  /* RX buffer full */
#define UART_CTRL_TXEN  BIT32(0)  /* TX enable */
#define UART_CTRL_RXEN  BIT32(1)  /* RX enable */

void uart_init(void);

void serial_putc(char c);
int  serial_getc(void);
```
-------- boards/mps2-an500/hal/time.c --------
```c
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
```
-------- boards/mps2-an500/hal/time.h --------
```h
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
```
-------- boards/mps2-an500/hal/board.c --------
```c
#include "board_hal.h"

#include "time.h"
#include "io/uart.h"

void board_init() {
	time_init();
	uart_init();
}
```
-------- boards/mps2-an500/linker_script.ld --------
```ld
__flash       = 0x00000000;
__flash_size  = 0x00400000;

__ram         = 0x20000000;
__ram_size    = 0x00400000;

__stack_size  = 64K;

INCLUDE picolibc.ld
```
-------- boards/mps2-an500/board.cmake --------
```cmake
tpl_board(
  NAME                "mps2-an500"
  DESCRIPTION         "mps2-an500, Arm Cortex M7"
  LINKER_SCRIPT       "${CMAKE_CURRENT_LIST_DIR}/linker_script.ld"
  PICOLIBC_CROSS_FILE "${CMAKE_CURRENT_LIST_DIR}/picolibc-cross.txt"
  USE_SEMIHOST        OFF
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/startup.c"
    "${CMAKE_CURRENT_LIST_DIR}/hal/board.c"
    "${CMAKE_CURRENT_LIST_DIR}/hal/time.c"
    "${CMAKE_CURRENT_LIST_DIR}/hal/io/uart.c"
  INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}"
)
```
