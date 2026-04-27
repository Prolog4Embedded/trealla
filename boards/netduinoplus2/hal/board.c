#include "board_hal.h"

#include "time.h"
#include "io/usart.h"

void board_init() {
	time_init();
	usart_init();
}
