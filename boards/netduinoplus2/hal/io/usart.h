#pragma once

#include <stdint.h>

#include "baremetal/hal_utils.h"

// p. 968, https://www.st.com/resource/en/reference_manual/DM00031020.pdf

// 8 or, 9 bit word length configured in USART_CR1

// lsb out first, characters preceded by start bit

// Interrupt Requests
// Transmit Data Register Empty, TXE, TXEIE
// CTS flag, CTS, CTSIE
// Transmission Complete, TC, TCIE
// Received Data Ready to be Read, RXNE, RXNEIE
// Overrun Error Detected, ORE, RXNEIE
// Idle Line Detected, IDLE, IDLEIE
// Parity Error, PE, PEIE
// Break Flag, LBD, LBDIE
// Noise Flag, Overrun error and Framing Error in multibuffer communication NF or ORE or, FE, EIE

typedef struct {
    volatile uint32_t SR;  // Status register
    volatile uint32_t DR;  // Data register
	volatile uint32_t BRR; // Baud rate register
    volatile uint32_t CR1; // Control register 1
	volatile uint32_t CR2; // Control register 2
	volatile uint32_t CR3; // Control register 3
	volatile uint32_t GTPR; // Guard time and prescaler register
} usart_t;

#define USART1_BASE UINT32_C(0x40011000)
#define USART1 ((usart_t *)USART1_BASE)

// Status bits
#define USART_SR_PE BIT32(0)  // Parity error
#define USART_SR_FE BIT32(1)  // Framing error
#define USART_SR_ORE BIT32(3) // Overrun error
#define USART_SR_RXNE BIT32(5)  // Read data register not empty
#define USART_SR_TC BIT32(6)  // Transmission complete
#define USART_SR_TXE BIT32(7)  // Transmission register empty

// Control register 1 bits, p. 1014f
#define USART_CR1_SBK BIT32(0) // Send break
#define USART_CR1_RWU BIT32(1) // Receiver wake-up
#define USART_CR1_RE BIT32(2) // Receiver enable
#define USART_CR1_TE BIT32(3) // Transmitter enable
#define USART_CR1_IDLEIE BIT32(4) // Idle interrupt enable
#define USART_CR1_RXNEIE BIT32(5) // RXNE interrupt enable
#define USART_CR1_TCEIE BIT32(6) // PE interrupt enable
#define USART_CR1_TXEIE BIT32(7) // TXE interrupt enable
#define USART_CR1_PEIE BIT32(8) // PE interrupt enable
#define USART_CR1_PS BIT32(9) // Parity selection
#define USART_CR1_PCE BIT32(10) // Parity control enable
#define USART_CR1_WAKE BIT32(11) // Wake-up method
#define USART_CR1_M BIT32(12) // Word length
#define USART_CR1_UE BIT32(13) // USART enable
#define USART_CR1_OVER8 BIT32(15) // Oversampling mode

// Control register 2 bits, p. 1016f
#define USART_CR2_ADD_Pos   0 // Address of the USART node
#define USART_CR2_ADD_Msk   (15u << USART_CR2_ADD_Pos)   // 0b1111
#define USART_CR2_LBDL BIT32(5) // LIN break detection length
#define USART_CR2_LBDIE BIT32(6) // LIN break detection interrupt enable
#define USART_CR2_LBCL BIT32(8) // Last bit clock pulse
#define USART_CR2_CPHA BIT32(9) // Clock phase
#define USART_CR2_CPOL BIT32(10) // Clock polarity
#define USART_CR2_CLKEN BIT32(11) // Clock enable
#define USART_CR2_STOP_Pos 12 // STOP bits
#define USART_CR2_STOP_1   (0u << USART_CR2_STOP_Pos)
#define USART_CR2_STOP_2   (2u << USART_CR2_STOP_Pos)
#define USART_CR2_LINEN BIT32(14) // LIN mode enable

// Control register 3, p. 1017f
#define USART_CR3_EIE BIT32(0) // Error interrupt enable
#define USART_CR3_IREN BIT32(1) // IrDA mode enable
#define USART_CR3_IRLP BIT32(2) // IrDA low-power
#define USART_CR3_HDSEL BIT32(3) // Half-duplex selection
#define USART_CR3_NACK BIT32(4) // Smartcard NACK enable
#define USART_CR3_SCEN BIT32(5) // Smartcard mode enable
#define USART_CR3_DMAR BIT32(6) // DMA enable receiver
#define USART_CR3_DMAT BIT32(7) // DMA enable transmitter
#define USART_CR3_RTSE BIT32(8) // RTSE enable
#define USART_CR3_CTSE BIT32(9) // CTS enable
#define USART_CR3_CTSIE BIT32(10) // CTS interrupt enable
#define USART_CR3_ONEBIT BIT32(11) // One sample bit method enable

void usart_init(void);

void serial_putc(char c);
int  serial_getc(void);
