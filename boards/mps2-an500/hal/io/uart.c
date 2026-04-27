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
