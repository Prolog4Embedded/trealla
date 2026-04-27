#include "uart.h"

#include <stdint.h>

void serial_init(uint32_t baud)
{
    CCM_CCGR3 |= CCM_CCGR3_LPUART6;

    // Pin mux: ALT2 routes the pads to LPUART6
    IOMUXC_SW_MUX_GPIO_AD_B0_02 = 2; // LPUART6_TX
    IOMUXC_SW_MUX_GPIO_AD_B0_03 = 2; // LPUART6_RX
    IOMUXC_LPUART6_RX_SEL_INPUT = 1; // daisy: GPIO_AD_B0_03_ALT2

    // Software reset
    LPUART6->GLOBAL |= LPUART_GLOBAL_RST;
    LPUART6->GLOBAL &= ~LPUART_GLOBAL_RST;

    // OSR=15 → 16x oversampling; SBR = 24 MHz / (16 * baud)
    LPUART6->BAUD = LPUART_BAUD_OSR(15) | (24000000u / (16u * baud));
    LPUART6->CTRL = LPUART_CTRL_TE | LPUART_CTRL_RE;
}

void serial_putc(char c)
{
    while (!(LPUART6->STAT & LPUART_STAT_TDRE))
        ;
    LPUART6->DATA = (uint32_t)(uint8_t)c;
}

int serial_getc(void)
{
    while (!(LPUART6->STAT & LPUART_STAT_RDRF))
        ;
    return (int)(LPUART6->DATA & 0xFF);
}
