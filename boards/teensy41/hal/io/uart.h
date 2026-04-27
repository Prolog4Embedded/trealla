#pragma once

#include <stdint.h>

#include "baremetal/hal_utils.h"

// i.MX RT1060 LPUART6, p. 2836ff
#define LPUART6_BASE UINT32_C(0x40198000)

typedef struct {
    volatile uint32_t VERID;
    volatile uint32_t PARAM;
    volatile uint32_t GLOBAL;
    volatile uint32_t PINCFG;
    volatile uint32_t BAUD;
    volatile uint32_t STAT;
    volatile uint32_t CTRL;
    volatile uint32_t DATA;
    volatile uint32_t MATCH;
    volatile uint32_t MODIR;
    volatile uint32_t FIFO;
    volatile uint32_t WATER;
} lpuart_t;

#define LPUART6 ((lpuart_t *)LPUART6_BASE)

// GLOBAL bits
#define LPUART_GLOBAL_RST   BIT32(1)  // software reset

// BAUD register fields
#define LPUART_BAUD_OSR(n)  (((uint32_t)(n)) << 24) // oversampling ratio (write value-1)

// STAT bits
#define LPUART_STAT_TDRE    BIT32(23) // transmit data register empty
#define LPUART_STAT_RDRF    BIT32(21) // receive data register full

// CTRL bits
#define LPUART_CTRL_TE      BIT32(19) // transmitter enable
#define LPUART_CTRL_RE      BIT32(18) // receiver enable

// CCM: LPUART6 clock gate — CCM_CCGR3[5:4] = 0b11
// CCM_BASE defined in time.h; include order must put time.h first, or redefine here
#ifndef CCM_BASE
#define CCM_BASE UINT32_C(0x400FC000)
#endif
#define CCM_CCGR3           (*(volatile uint32_t *)(CCM_BASE + UINT32_C(0x74)))
#define CCM_CCGR3_LPUART6   (UINT32_C(3) << 4)

// IOMUXC: GPIO_AD_B0_02 → LPUART6_TX (ALT2), GPIO_AD_B0_03 → LPUART6_RX (ALT2)
// i.MX RT1060 RM §11.5
#define IOMUXC_BASE                    UINT32_C(0x401F8000)
#define IOMUXC_SW_MUX_GPIO_AD_B0_02   (*(volatile uint32_t *)(IOMUXC_BASE + UINT32_C(0x00F4)))
#define IOMUXC_SW_MUX_GPIO_AD_B0_03   (*(volatile uint32_t *)(IOMUXC_BASE + UINT32_C(0x00F8)))
#define IOMUXC_LPUART6_RX_SEL_INPUT   (*(volatile uint32_t *)(IOMUXC_BASE + UINT32_C(0x049C)))

void serial_init(uint32_t baud);
void serial_putc(char c);
int  serial_getc(void);
