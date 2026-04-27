#include "usart.h"
#include <stdio.h>

void usart_init()
{
    // Clock needs to be enabled!

    USART1->CR1 = 0;

    // 16 MHz clock -> 115200 baud
    // BRR = fclk / baud c.a. 16000000 / 115200 ≈ 139
    USART1->BRR = 139;

    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	printf("UART ready\n");
}

void serial_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE))
        ;
    USART1->DR = (uint32_t)(uint8_t)c;
}

int serial_getc(void)
{
    while (!(USART1->SR & USART_SR_RXNE))
        ;
    return (int)(USART1->DR & 0xFF);
}
