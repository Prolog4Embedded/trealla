#pragma once

#include <stdint.h>

#include "baremetal/hal_utils.h"

// i.MX RT1060 GPT1, p. 2999ff
#define GPT1_BASE UINT32_C(0x401EC000)

typedef struct {
    volatile uint32_t CR;   // Control register        (0x00)
    volatile uint32_t PR;   // Prescaler register       (0x04)
    volatile uint32_t SR;   // Status register          (0x08)
    volatile uint32_t IR;   // Interrupt register       (0x0C)
    volatile uint32_t OCR1; // Output compare 1         (0x10)
    volatile uint32_t OCR2; // Output compare 2         (0x14)
    volatile uint32_t OCR3; // Output compare 3         (0x18)
    volatile uint32_t ICR1; // Input capture 1          (0x1C)
    volatile uint32_t ICR2; // Input capture 2          (0x20)
    volatile uint32_t CNT;  // Counter register         (0x24)
} gpt_t;

#define GPT1 ((gpt_t *)GPT1_BASE)

// CR bits
#define GPT_CR_EN           BIT32(0)             // GPT enable
#define GPT_CR_ENMOD        BIT32(1)             // Reset counter on enable
#define GPT_CR_FRR          BIT32(9)             // Free-run (no reset on compare match)
#define GPT_CR_EN_24M       BIT32(10)            // Enable 24 MHz crystal clock input
#define GPT_CR_CLKSRC_24M   (UINT32_C(5) << 6)  // ipg_clk_24M (crystal oscillator)

// CCM: GPT1 bus clock gate — CCM_CCGR1[29:28] = 0b11
#define CCM_BASE         UINT32_C(0x400FC000)
#define CCM_CCGR1        (*(volatile uint32_t *)(CCM_BASE + UINT32_C(0x6C)))
#define CCM_CCGR1_GPT1   (UINT32_C(3) << 28)

void time_init(void);
