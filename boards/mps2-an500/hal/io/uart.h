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
