#include "board_hal.h"

#include "time.h"
#include "io/uart.h"

void board_init() {
	time_init();
	uart_init();
}
