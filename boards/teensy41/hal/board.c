#include "board_hal.h"

#include "time.h"
#include "io/uart.h"

void board_init(void)
{
    time_init();
    serial_init(115200);
}
